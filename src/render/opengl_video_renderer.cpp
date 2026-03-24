#include "render/opengl_video_renderer.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#if defined(_WIN32) && !defined(WIN32_LEAN_AND_MEAN)
#define WIN32_LEAN_AND_MEAN
#endif
#if defined(_WIN32) && !defined(NOMINMAX)
#define NOMINMAX
#endif

#if __has_include(<SDL2/SDL.h>)
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#elif __has_include(<SDL.h>)
#include <SDL.h>
#include <SDL_opengl.h>
#else
#error "SDL2 headers not found"
#endif

#if defined(_WIN32)
#include <windows.h>
#include <d3d11.h>
#include <d3d11_1.h>
#include <d3d11_3.h>
#include <d3d11_4.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#endif

extern "C" {
#include <libavutil/frame.h>
#include <libavutil/pixdesc.h>
}

#include "logger.h"
#include "render/d3d11_video_renderer.h"

namespace vp::render {

namespace {

constexpr int kMinWindowWidth = 320;
constexpr int kMinWindowHeight = 180;
constexpr float kVolumeStep = 0.05f;
constexpr double kSeekStepSeconds = 5.0;
constexpr double kSeekStepSecondsCtrl = 30.0;
constexpr double kSpeedStep = 0.1;
constexpr int kControlPanelInset = 12;
constexpr int kControlPanelHeight = 46;
constexpr int kControlPadding = 10;
constexpr int kControlGap = 12;
constexpr int kBarHeight = 8;
constexpr int kVolumeBarWidth = 96;
constexpr int kMinProgressBarWidth = 96;
constexpr uint64_t kOsdHoldMs = 1500;

void updateMaxAtomic(std::atomic<uint64_t>& target, uint64_t value) {
    uint64_t current = target.load();
    while (value > current && !target.compare_exchange_weak(current, value)) {
    }
}

struct WindowSize {
    int width;
    int height;
};

WindowSize computeInitialWindowSize(int video_width, int video_height) {
    const int safe_video_width = std::max(1, video_width);
    const int safe_video_height = std::max(1, video_height);

    WindowSize result{safe_video_width, safe_video_height};
    SDL_Rect usable_bounds{};
    if (SDL_GetDisplayUsableBounds(0, &usable_bounds) != 0 || usable_bounds.w <= 0 || usable_bounds.h <= 0) {
        return result;
    }

    constexpr double kMaxDisplayRatio = 0.9;
    const double max_width = static_cast<double>(usable_bounds.w) * kMaxDisplayRatio;
    const double max_height = static_cast<double>(usable_bounds.h) * kMaxDisplayRatio;
    const double width_scale = max_width / static_cast<double>(safe_video_width);
    const double height_scale = max_height / static_cast<double>(safe_video_height);
    const double scale = std::min(1.0, std::min(width_scale, height_scale));

    result.width = std::max(1, static_cast<int>(std::lround(static_cast<double>(safe_video_width) * scale)));
    result.height = std::max(1, static_cast<int>(std::lround(static_cast<double>(safe_video_height) * scale)));
    return result;
}

void updateWindowSizeFromSdl(SDL_Window* window, std::atomic<int>& width, std::atomic<int>& height) {
    if (!window) {
        return;
    }

    int window_width = width.load();
    int window_height = height.load();
    SDL_GetWindowSize(window, &window_width, &window_height);
    width.store(std::max(1, window_width));
    height.store(std::max(1, window_height));
}

SDL_Rect computeRenderRect(int window_width, int window_height, int frame_width, int frame_height) {
    SDL_Rect dst_rect{0, 0, std::max(1, window_width), std::max(1, window_height)};
    if (window_width <= 0 || window_height <= 0 || frame_width <= 0 || frame_height <= 0) {
        return dst_rect;
    }

    const double src_aspect = static_cast<double>(frame_width) / static_cast<double>(frame_height);
    const double dst_aspect = static_cast<double>(window_width) / static_cast<double>(window_height);
    if (dst_aspect > src_aspect) {
        dst_rect.h = window_height;
        dst_rect.w = std::max(1, static_cast<int>(std::lround(static_cast<double>(window_height) * src_aspect)));
        dst_rect.x = (window_width - dst_rect.w) / 2;
        dst_rect.y = 0;
    } else {
        dst_rect.w = window_width;
        dst_rect.h = std::max(1, static_cast<int>(std::lround(static_cast<double>(window_width) / src_aspect)));
        dst_rect.x = 0;
        dst_rect.y = (window_height - dst_rect.h) / 2;
    }
    return dst_rect;
}

double clampRatio(double value) {
    return std::max(0.0, std::min(1.0, value));
}

float clampVolume(float value) {
    return std::max(0.0f, std::min(1.0f, value));
}

int chromaExtent(int value) {
    return std::max(1, (value + 1) / 2);
}

bool isLimitedRange(AVColorRange range) {
    return range != AVCOL_RANGE_JPEG;
}

bool useBt601Matrix(AVColorSpace space) {
    switch (space) {
    case AVCOL_SPC_BT470BG:
    case AVCOL_SPC_SMPTE170M:
    case AVCOL_SPC_FCC:
        return true;
    default:
        return false;
    }
}

struct ColorCoefficients {
    std::array<float, 4> r{};
    std::array<float, 4> g{};
    std::array<float, 4> b{};
};

ColorCoefficients buildColorCoefficients(AVColorRange range, AVColorSpace space) {
    const bool limited = isLimitedRange(range);
    const bool bt601 = useBt601Matrix(space);
    const float y_bias = limited ? (-16.0f / 255.0f) : 0.0f;
    const float uv_bias = limited ? (-128.0f / 255.0f) : -0.5f;
    const float y_scale = limited ? (255.0f / 219.0f) : 1.0f;
    const float rv = bt601 ? 1.596027f : 1.792741f;
    const float gu = bt601 ? -0.391762f : -0.213249f;
    const float gv = bt601 ? -0.812968f : -0.532909f;
    const float bu = bt601 ? 2.017232f : 2.112402f;

    ColorCoefficients coeffs;
    coeffs.r = {y_scale, 0.0f, rv, y_scale * y_bias + rv * uv_bias};
    coeffs.g = {y_scale, gu, gv, y_scale * y_bias + (gu + gv) * uv_bias};
    coeffs.b = {y_scale, bu, 0.0f, y_scale * y_bias + bu * uv_bias};
    return coeffs;
}

uint64_t monotonicMsNow() {
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}

struct GlFunctions {
    PFNGLCREATESHADERPROC CreateShader{};
    PFNGLSHADERSOURCEPROC ShaderSource{};
    PFNGLCOMPILESHADERPROC CompileShader{};
    PFNGLGETSHADERIVPROC GetShaderiv{};
    PFNGLGETSHADERINFOLOGPROC GetShaderInfoLog{};
    PFNGLDELETESHADERPROC DeleteShader{};
    PFNGLCREATEPROGRAMPROC CreateProgram{};
    PFNGLATTACHSHADERPROC AttachShader{};
    PFNGLLINKPROGRAMPROC LinkProgram{};
    PFNGLGETPROGRAMIVPROC GetProgramiv{};
    PFNGLGETPROGRAMINFOLOGPROC GetProgramInfoLog{};
    PFNGLDELETEPROGRAMPROC DeleteProgram{};
    PFNGLUSEPROGRAMPROC UseProgram{};
    PFNGLGETUNIFORMLOCATIONPROC GetUniformLocation{};
    PFNGLUNIFORM1IPROC Uniform1i{};
    PFNGLUNIFORM4FPROC Uniform4f{};
    PFNGLACTIVETEXTUREPROC ActiveTexture{};
};

GlFunctions g_gl;

template <typename T>
bool loadGlProc(T& target, const char* name) {
    target = reinterpret_cast<T>(SDL_GL_GetProcAddress(name));
    if (!target) {
        LOG_ERROR("OpenGL proc load failed: " << name);
        return false;
    }
    return true;
}

bool loadGlFunctions() {
    return loadGlProc(g_gl.CreateShader, "glCreateShader") &&
           loadGlProc(g_gl.ShaderSource, "glShaderSource") &&
           loadGlProc(g_gl.CompileShader, "glCompileShader") &&
           loadGlProc(g_gl.GetShaderiv, "glGetShaderiv") &&
           loadGlProc(g_gl.GetShaderInfoLog, "glGetShaderInfoLog") &&
           loadGlProc(g_gl.DeleteShader, "glDeleteShader") &&
           loadGlProc(g_gl.CreateProgram, "glCreateProgram") &&
           loadGlProc(g_gl.AttachShader, "glAttachShader") &&
           loadGlProc(g_gl.LinkProgram, "glLinkProgram") &&
           loadGlProc(g_gl.GetProgramiv, "glGetProgramiv") &&
           loadGlProc(g_gl.GetProgramInfoLog, "glGetProgramInfoLog") &&
           loadGlProc(g_gl.DeleteProgram, "glDeleteProgram") &&
           loadGlProc(g_gl.UseProgram, "glUseProgram") &&
           loadGlProc(g_gl.GetUniformLocation, "glGetUniformLocation") &&
           loadGlProc(g_gl.Uniform1i, "glUniform1i") &&
           loadGlProc(g_gl.Uniform4f, "glUniform4f") &&
           loadGlProc(g_gl.ActiveTexture, "glActiveTexture");
}

GLuint compileShader(GLenum type, const char* source, const char* label) {
    const GLuint shader = g_gl.CreateShader(type);
    if (shader == 0) {
        LOG_ERROR("OpenGL CreateShader failed for " << label);
        return 0;
    }

    g_gl.ShaderSource(shader, 1, &source, nullptr);
    g_gl.CompileShader(shader);

    GLint compiled = GL_FALSE;
    g_gl.GetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_TRUE) {
        return shader;
    }

    char info_log[1024]{};
    GLsizei log_length = 0;
    g_gl.GetShaderInfoLog(shader, static_cast<GLsizei>(sizeof(info_log) - 1), &log_length, info_log);
    LOG_ERROR("OpenGL shader compile failed for " << label << ": " << info_log);
    g_gl.DeleteShader(shader);
    return 0;
}

GLuint linkProgram(GLuint vertex_shader, GLuint fragment_shader, const char* label) {
    const GLuint program = g_gl.CreateProgram();
    if (program == 0) {
        LOG_ERROR("OpenGL CreateProgram failed for " << label);
        return 0;
    }

    g_gl.AttachShader(program, vertex_shader);
    g_gl.AttachShader(program, fragment_shader);
    g_gl.LinkProgram(program);

    GLint linked = GL_FALSE;
    g_gl.GetProgramiv(program, GL_LINK_STATUS, &linked);
    if (linked == GL_TRUE) {
        return program;
    }

    char info_log[1024]{};
    GLsizei log_length = 0;
    g_gl.GetProgramInfoLog(program, static_cast<GLsizei>(sizeof(info_log) - 1), &log_length, info_log);
    LOG_ERROR("OpenGL program link failed for " << label << ": " << info_log);
    g_gl.DeleteProgram(program);
    return 0;
}

constexpr const char* kVertexShaderSource = R"(#version 120
varying vec2 v_texcoord;
void main() {
    gl_Position = gl_Vertex;
    v_texcoord = gl_MultiTexCoord0.xy;
}
)";

constexpr const char* kYuv420FragmentShaderSource = R"(#version 120
uniform sampler2D texY;
uniform sampler2D texU;
uniform sampler2D texV;
uniform vec4 coeffR;
uniform vec4 coeffG;
uniform vec4 coeffB;
varying vec2 v_texcoord;
void main() {
    float y = texture2D(texY, v_texcoord).r;
    float u = texture2D(texU, v_texcoord).r;
    float v = texture2D(texV, v_texcoord).r;
    vec4 yuv = vec4(y, u, v, 1.0);
    gl_FragColor = vec4(dot(yuv, coeffR), dot(yuv, coeffG), dot(yuv, coeffB), 1.0);
}
)";

constexpr const char* kNv12FragmentShaderSource = R"(#version 120
uniform sampler2D texY;
uniform sampler2D texUV;
uniform vec4 coeffR;
uniform vec4 coeffG;
uniform vec4 coeffB;
varying vec2 v_texcoord;
void main() {
    float y = texture2D(texY, v_texcoord).r;
    vec4 uv_sample = texture2D(texUV, v_texcoord);
    vec4 yuv = vec4(y, uv_sample.r, uv_sample.a, 1.0);
    gl_FragColor = vec4(dot(yuv, coeffR), dot(yuv, coeffG), dot(yuv, coeffB), 1.0);
}
)";

#if defined(_WIN32)
using Microsoft::WRL::ComPtr;

const char* boolName(bool value) { return value ? "true" : "false"; }

struct ColorMatrixConstants {
    float coeff_r[4];
    float coeff_g[4];
    float coeff_b[4];
    float reserved[4];
};

bool isLimitedRange(const AVFrame* frame) { return !frame || frame->color_range != AVCOL_RANGE_JPEG; }

bool useBt601Matrix(const AVFrame* frame) {
    if (!frame) {
        return false;
    }
    switch (frame->colorspace) {
    case AVCOL_SPC_BT470BG:
    case AVCOL_SPC_SMPTE170M:
    case AVCOL_SPC_FCC:
        return true;
    default:
        return false;
    }
}

ColorMatrixConstants buildNativeColorMatrix(const AVFrame* frame, bool high_bit_depth) {
    const bool limited = isLimitedRange(frame);
    const bool bt601 = useBt601Matrix(frame);
    const float y_bias = limited ? (high_bit_depth ? (-64.0f / 1023.0f) : (-16.0f / 255.0f)) : 0.0f;
    const float uv_bias = limited ? (high_bit_depth ? (-512.0f / 1023.0f) : (-128.0f / 255.0f)) : -0.5f;
    const float y_scale = limited ? (high_bit_depth ? (1023.0f / 876.0f) : (255.0f / 219.0f)) : 1.0f;
    const float rv = bt601 ? 1.596027f : 1.792741f;
    const float gu = bt601 ? -0.391762f : -0.213249f;
    const float gv = bt601 ? -0.812968f : -0.532909f;
    const float bu = bt601 ? 2.017232f : 2.112402f;

    ColorMatrixConstants constants{};
    constants.coeff_r[0] = y_scale;
    constants.coeff_r[2] = rv;
    constants.coeff_r[3] = y_scale * y_bias + rv * uv_bias;
    constants.coeff_g[0] = y_scale;
    constants.coeff_g[1] = gu;
    constants.coeff_g[2] = gv;
    constants.coeff_g[3] = y_scale * y_bias + (gu + gv) * uv_bias;
    constants.coeff_b[0] = y_scale;
    constants.coeff_b[1] = bu;
    constants.coeff_b[3] = y_scale * y_bias + bu * uv_bias;
    return constants;
}

HRESULT compileD3DShader(const char* source, const char* entry, const char* profile, ComPtr<ID3DBlob>& blob) {
    UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(DEBUG_MODE)
    flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
    ComPtr<ID3DBlob> errors;
    const HRESULT hr = D3DCompile(source, std::strlen(source), nullptr, nullptr, nullptr, entry, profile, flags, 0,
                                  blob.GetAddressOf(), errors.GetAddressOf());
    if (FAILED(hr) && errors) {
        LOG_ERROR("OpenGL D3D11 shader compile failed: " << static_cast<const char*>(errors->GetBufferPointer()));
    }
    return hr;
}

std::wstring utf8ToWide(const std::string& text) {
    if (text.empty()) {
        return {};
    }
    auto convert = [&](UINT code_page, DWORD flags) -> std::wstring {
        const int wide_length = MultiByteToWideChar(code_page, flags, text.data(), static_cast<int>(text.size()), nullptr, 0);
        if (wide_length <= 0) {
            return {};
        }
        std::wstring wide(static_cast<size_t>(wide_length), L'\0');
        const int converted = MultiByteToWideChar(code_page, flags, text.data(), static_cast<int>(text.size()), wide.data(), wide_length);
        if (converted != wide_length) {
            return {};
        }
        return wide;
    };
    std::wstring wide = convert(CP_UTF8, MB_ERR_INVALID_CHARS);
    return !wide.empty() ? wide : convert(CP_ACP, 0);
}

int clampSubtitleAlignment(int alignment) { return alignment >= 1 && alignment <= 9 ? alignment : 2; }
int subtitleHorizontalGroup(int alignment) { return (clampSubtitleAlignment(alignment) - 1) % 3; }
int subtitleVerticalGroup(int alignment) { return (clampSubtitleAlignment(alignment) - 1) / 3; }

struct GdiSurface {
    HDC dc{nullptr};
    HBITMAP bitmap{nullptr};
    HGDIOBJ old_bitmap{nullptr};
    void* bits{nullptr};
    int width{0};
    int height{0};
};

bool createGdiSurface(int width, int height, GdiSurface& surface) {
    surface = {};
    surface.width = std::max(1, width);
    surface.height = std::max(1, height);
    surface.dc = CreateCompatibleDC(nullptr);
    if (!surface.dc) {
        return false;
    }
    BITMAPINFO info{};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = surface.width;
    info.bmiHeader.biHeight = -surface.height;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    surface.bitmap = CreateDIBSection(surface.dc, &info, DIB_RGB_COLORS, &surface.bits, nullptr, 0);
    if (!surface.bitmap || !surface.bits) {
        if (surface.bitmap) DeleteObject(surface.bitmap);
        DeleteDC(surface.dc);
        surface = {};
        return false;
    }
    surface.old_bitmap = SelectObject(surface.dc, surface.bitmap);
    SetBkMode(surface.dc, TRANSPARENT);
    return true;
}

void destroyGdiSurface(GdiSurface& surface) {
    if (surface.dc && surface.old_bitmap) SelectObject(surface.dc, surface.old_bitmap);
    if (surface.bitmap) DeleteObject(surface.bitmap);
    if (surface.dc) DeleteDC(surface.dc);
    surface = {};
}

void clearGdiSurface(const GdiSurface& surface) {
    if (surface.bits) {
        std::memset(surface.bits, 0, static_cast<size_t>(surface.width) * static_cast<size_t>(surface.height) * 4u);
    }
}

void blendPixelBGRA(uint8_t* dst, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    const int dst_a = dst[3];
    const int src_a = a;
    const int out_a = src_a + ((dst_a * (255 - src_a) + 127) / 255);
    if (out_a <= 0) {
        dst[0] = dst[1] = dst[2] = dst[3] = 0;
        return;
    }
    const int src_b_premul = b * src_a;
    const int src_g_premul = g * src_a;
    const int src_r_premul = r * src_a;
    const int dst_b_premul = dst[0] * dst_a;
    const int dst_g_premul = dst[1] * dst_a;
    const int dst_r_premul = dst[2] * dst_a;
    const int out_b_premul = src_b_premul + ((dst_b_premul * (255 - src_a) + 127) / 255);
    const int out_g_premul = src_g_premul + ((dst_g_premul * (255 - src_a) + 127) / 255);
    const int out_r_premul = src_r_premul + ((dst_r_premul * (255 - src_a) + 127) / 255);
    dst[0] = static_cast<uint8_t>((out_b_premul + out_a / 2) / out_a);
    dst[1] = static_cast<uint8_t>((out_g_premul + out_a / 2) / out_a);
    dst[2] = static_cast<uint8_t>((out_r_premul + out_a / 2) / out_a);
    dst[3] = static_cast<uint8_t>(out_a);
}

void fillRectBGRA(std::vector<uint8_t>& pixels, int width, int height, const RECT& rect, const subtitle::SubtitleColor& color) {
    const int left = (std::max)(0, static_cast<int>(rect.left));
    const int top = (std::max)(0, static_cast<int>(rect.top));
    const int right = (std::min)(width, static_cast<int>(rect.right));
    const int bottom = (std::min)(height, static_cast<int>(rect.bottom));
    if (left >= right || top >= bottom) {
        return;
    }
    for (int y = top; y < bottom; ++y) {
        uint8_t* row = pixels.data() + (static_cast<size_t>(y) * static_cast<size_t>(width) + static_cast<size_t>(left)) * 4u;
        for (int x = left; x < right; ++x) {
            blendPixelBGRA(row, color.r, color.g, color.b, color.a);
            row += 4;
        }
    }
}

void compositeMaskToPixels(const GdiSurface& surface, std::vector<uint8_t>& pixels, const subtitle::SubtitleColor& color) {
    const auto* mask = static_cast<const uint8_t*>(surface.bits);
    if (!mask) return;
    const size_t pixel_count = static_cast<size_t>(surface.width) * static_cast<size_t>(surface.height);
    for (size_t i = 0; i < pixel_count; ++i) {
        const uint8_t blue = mask[i * 4u + 0u];
        const uint8_t green = mask[i * 4u + 1u];
        const uint8_t red = mask[i * 4u + 2u];
        const uint8_t coverage = static_cast<uint8_t>(std::max({blue, green, red}));
        if (coverage == 0) continue;
        const uint8_t alpha = static_cast<uint8_t>((static_cast<unsigned int>(coverage) * static_cast<unsigned int>(color.a) + 127u) / 255u);
        blendPixelBGRA(pixels.data() + i * 4u, color.r, color.g, color.b, alpha);
    }
}

#ifndef WGL_ACCESS_READ_ONLY_NV
#define WGL_ACCESS_READ_ONLY_NV 0x0000
#endif
using PFNWGLDXOPENDEVICENVPROC = HANDLE(WINAPI*)(void* dx_device);
using PFNWGLDXCLOSEDEVICENVPROC = BOOL(WINAPI*)(HANDLE h_device);
using PFNWGLDXREGISTEROBJECTNVPROC = HANDLE(WINAPI*)(HANDLE h_device, void* dx_object, GLuint name, GLenum type, GLenum access);
using PFNWGLDXUNREGISTEROBJECTNVPROC = BOOL(WINAPI*)(HANDLE h_device, HANDLE h_object);
using PFNWGLDXLOCKOBJECTSNVPROC = BOOL(WINAPI*)(HANDLE h_device, GLint count, HANDLE* h_objects);
using PFNWGLDXUNLOCKOBJECTSNVPROC = BOOL(WINAPI*)(HANDLE h_device, GLint count, HANDLE* h_objects);
#endif

}  // namespace

class OpenGLVideoRenderer::Impl {
public:
    Impl() = default;
    ~Impl() { close(); }

    bool init(const VideoRendererConfig& config);
    void close();
    void renderFrame(const core::VideoFrame& frame);
    void present();
    void clear();
    void handleEvents() {}
    bool shouldQuit() const { return should_quit_.load(); }
    bool consumeTogglePauseRequest();
    bool consumeSeekRequest(double& normalized_position);
    bool consumeSeekDeltaRequest(double& delta_seconds);
    bool consumeVolumeChangeRequest(float& volume);
    bool consumeSpeedChangeRequest(double& speed_delta);
    bool consumeResetSpeedRequest();
    bool consumeToggleSubtitleRequest();
    bool consumeSetABRepeatStartRequest();
    bool consumeSetABRepeatEndRequest();
    bool consumeClearABRepeatRequest();
    bool consumeScreenshotRequest();
    bool consumeStepFrameBackwardRequest();
    bool consumeStepFrameForwardRequest();
    bool consumeSubtitleDelayChangeRequest(double& delta_seconds);
    bool consumeAudioDelayChangeRequest(double& delta_seconds);
    bool consumeNextChapterRequest();
    bool consumePreviousChapterRequest();
    bool consumeNextItemRequest();
    bool consumePreviousItemRequest();
    void setOverlayState(double position, double duration, float volume, bool paused);
    void setSubtitleText(const std::string& text);
    void setSubtitleItems(const std::vector<subtitle::SubtitleItem>& items);
    void setHotkeyManager(const input::HotkeyManager& hotkey_manager);
    RendererDiagnostics getDiagnostics() const;
    void resetDiagnostics();
    bool supportsNativeFrameFormat(AVPixelFormat format) const;
    bool supportsDirectFrameFormat(AVPixelFormat format) const;
    void* nativeDeviceHandle() const;

private:
    struct PendingVideoFrame {
        int width{0};
        int height{0};
        AVPixelFormat format{AV_PIX_FMT_NONE};
        AVColorRange color_range{AVCOL_RANGE_UNSPECIFIED};
        AVColorSpace color_space{AVCOL_SPC_UNSPECIFIED};
        int y_pitch{0};
        int u_pitch{0};
        int v_pitch{0};
        std::vector<uint8_t> y_plane;
        std::vector<uint8_t> u_plane;
        std::vector<uint8_t> v_plane;
        AVFrame* native_frame{nullptr};
        bool valid{false};

        PendingVideoFrame() = default;
        ~PendingVideoFrame() { reset(); }
        PendingVideoFrame(const PendingVideoFrame&) = delete;
        PendingVideoFrame& operator=(const PendingVideoFrame&) = delete;
        PendingVideoFrame(PendingVideoFrame&& other) noexcept { moveFrom(std::move(other)); }
        PendingVideoFrame& operator=(PendingVideoFrame&& other) noexcept {
            if (this != &other) {
                reset();
                moveFrom(std::move(other));
            }
            return *this;
        }
        void reset() {
            if (native_frame) av_frame_free(&native_frame);
            width = height = 0;
            format = AV_PIX_FMT_NONE;
            color_range = AVCOL_RANGE_UNSPECIFIED;
            color_space = AVCOL_SPC_UNSPECIFIED;
            y_pitch = u_pitch = v_pitch = 0;
            y_plane.clear();
            u_plane.clear();
            v_plane.clear();
            valid = false;
        }
        bool isNative() const { return native_frame != nullptr && format == AV_PIX_FMT_D3D11; }
    private:
        void moveFrom(PendingVideoFrame&& other) {
            width = other.width;
            height = other.height;
            format = other.format;
            color_range = other.color_range;
            color_space = other.color_space;
            y_pitch = other.y_pitch;
            u_pitch = other.u_pitch;
            v_pitch = other.v_pitch;
            y_plane = std::move(other.y_plane);
            u_plane = std::move(other.u_plane);
            v_plane = std::move(other.v_plane);
            native_frame = other.native_frame;
            valid = other.valid;
            other.native_frame = nullptr;
            other.width = other.height = 0;
            other.format = AV_PIX_FMT_NONE;
            other.color_range = AVCOL_RANGE_UNSPECIFIED;
            other.color_space = AVCOL_SPC_UNSPECIFIED;
            other.y_pitch = other.u_pitch = other.v_pitch = 0;
            other.valid = false;
        }
    };

    struct ControlRequests {
        bool toggle_pause_requested{false};
        bool seek_requested{false};
        double seek_ratio{0.0};
        bool seek_delta_requested{false};
        double seek_delta_seconds{0.0};
        bool volume_change_requested{false};
        float requested_volume{1.0f};
        bool speed_change_requested{false};
        double speed_delta{0.0};
        bool speed_reset_requested{false};
        bool subtitle_toggle_requested{false};
        bool ab_repeat_start_requested{false};
        bool ab_repeat_end_requested{false};
        bool ab_repeat_clear_requested{false};
        bool screenshot_requested{false};
        bool step_frame_backward_requested{false};
        bool step_frame_forward_requested{false};
        bool subtitle_delay_change_requested{false};
        double subtitle_delay_delta_seconds{0.0};
        bool audio_delay_change_requested{false};
        double audio_delay_delta_seconds{0.0};
        bool next_chapter_requested{false};
        bool previous_chapter_requested{false};
        bool next_item_requested{false};
        bool previous_item_requested{false};
    };

    struct SubtitleSnapshot {
        std::string text;
        subtitle::SubtitleStyle style{};
        int play_res_x{0};
        int play_res_y{0};
    };

    bool copyFrameData(const AVFrame& frame, PendingVideoFrame& out);
    bool refNativeFrame(const AVFrame& frame, PendingVideoFrame& out);
    void renderLoop();
    void pumpEvents();
    void handleKeyDown(int key_code, unsigned short modifiers);
    void applyAction(input::PlayerAction action, unsigned short modifiers);
    void requestFullscreenToggle();
    void requestRedraw();
    void requestRedrawIfPaused();
    void showOsdFor(uint64_t duration_ms = kOsdHoldMs);
    bool createGlContext();
    void destroyGlResources();
    bool ensureGlTextures(const PendingVideoFrame& frame);
    bool uploadFrameTextures(const PendingVideoFrame& frame);
    void renderCurrentFrame(const PendingVideoFrame* frame);
    void renderSoftwareFrame(const PendingVideoFrame& frame, int drawable_width, int drawable_height);
    void renderNativeFrame(const PendingVideoFrame& frame, int drawable_width, int drawable_height);
    void drawAspectQuad(int drawable_width, int drawable_height, int frame_width, int frame_height);
    void drawFilledRect(int drawable_width, int drawable_height, int x, int y, int w, int h, float r, float g, float b, float a);
    void drawSubtitleOverlay(const PendingVideoFrame* frame, int drawable_width, int drawable_height);
    void drawOsdOverlay(const PendingVideoFrame* frame, int drawable_width, int drawable_height);
    bool updateSubtitleTextureIfNeeded(const PendingVideoFrame* frame, int drawable_width, int drawable_height);
    SubtitleSnapshot snapshotSubtitleState() const;
    bool consumeFlagRequest(bool& flag);
    bool consumeDoubleRequest(bool& requested, double& value, double& out);
    bool consumeFloatRequest(bool& requested, float& value, float& out);

#if defined(_WIN32)
    struct NativeVideoVertex { float x; float y; float u; float v; };
    bool initializeNativeInterop();
    void destroyNativeInterop();
    bool loadWglInteropFunctions();
    bool createNativeD3DDevice();
    bool createNativeD3DShaders();
    bool ensureNativeSourceViewsLocked(ID3D11Texture2D* texture, intptr_t index, DXGI_FORMAT format);
    bool ensureNativeInteropTarget(int width, int height);
    void resetNativeInteropTarget();
    bool updateNativeGlTexture(const AVFrame* frame);
    void disableNativeInterop(const char* reason,
                              const D3D11_TEXTURE2D_DESC* texture_desc = nullptr,
                              intptr_t texture_index = 0,
                              HRESULT y_plane_hr = S_OK,
                              HRESULT uv_plane_hr = S_OK);
#endif

    std::string title_;
    SDL_Window* window_{nullptr};
    void* gl_context_{nullptr};
    std::atomic<int> width_{0};
    std::atomic<int> height_{0};
    std::atomic<bool> should_quit_{false};
    std::atomic<bool> fullscreen_{false};
    std::atomic<bool> minimized_{false};
    std::atomic<bool> render_running_{false};
    std::atomic<bool> render_initialized_{false};
    std::atomic<bool> render_init_success_{false};
    std::atomic<bool> fullscreen_toggle_requested_{false};
    std::atomic<bool> clear_requested_{false};
    std::atomic<bool> redraw_requested_{false};
    std::thread render_thread_;
    std::mutex frame_mutex_;
    std::condition_variable frame_cv_;
    PendingVideoFrame pending_frame_;
    bool pending_frame_ready_{false};

    std::mutex request_mutex_;
    ControlRequests requests_{};
    float last_nonzero_volume_{1.0f};

    std::atomic<double> overlay_position_{0.0};
    std::atomic<double> overlay_duration_{0.0};
    std::atomic<float> overlay_volume_{1.0f};
    std::atomic<bool> overlay_paused_{false};
    std::atomic<uint64_t> osd_visible_until_ms_{0};

    mutable std::mutex subtitle_mutex_;
    std::vector<subtitle::SubtitleItem> subtitle_items_;
    std::atomic<uint64_t> subtitle_generation_{1};
    input::HotkeyManager hotkey_manager_{};

    GLuint yuv420_program_{0};
    GLint yuv420_tex_y_location_{-1};
    GLint yuv420_tex_u_location_{-1};
    GLint yuv420_tex_v_location_{-1};
    GLint yuv420_coeff_r_location_{-1};
    GLint yuv420_coeff_g_location_{-1};
    GLint yuv420_coeff_b_location_{-1};
    GLuint nv12_program_{0};
    GLint nv12_tex_y_location_{-1};
    GLint nv12_tex_uv_location_{-1};
    GLint nv12_coeff_r_location_{-1};
    GLint nv12_coeff_g_location_{-1};
    GLint nv12_coeff_b_location_{-1};

    GLuint tex_y_{0};
    GLuint tex_u_{0};
    GLuint tex_v_{0};
    GLuint tex_uv_{0};
    int texture_width_{0};
    int texture_height_{0};
    AVPixelFormat texture_format_{AV_PIX_FMT_NONE};

    GLuint subtitle_texture_{0};
    int subtitle_texture_width_{0};
    int subtitle_texture_height_{0};
    bool subtitle_texture_valid_{false};
    uint64_t rendered_subtitle_generation_{0};
    SDL_Rect subtitle_cached_video_rect_{0, 0, 0, 0};
    int subtitle_cached_drawable_width_{0};
    int subtitle_cached_drawable_height_{0};
    std::vector<uint8_t> subtitle_pixels_;

    std::atomic<uint64_t> frame_copy_frames_{0};
    std::atomic<uint64_t> frame_copy_bytes_{0};
    std::atomic<uint64_t> frame_copy_time_us_total_{0};
    std::atomic<uint64_t> frame_copy_time_us_max_{0};

#if defined(_WIN32)
    ComPtr<ID3D11Device> native_d3d_device_;
    ComPtr<ID3D11DeviceContext> native_d3d_context_;
    ComPtr<ID3D11Device3> native_d3d_device3_;
    ComPtr<ID3D11VertexShader> native_video_vertex_shader_;
    ComPtr<ID3D11PixelShader> native_nv12_pixel_shader_;
    ComPtr<ID3D11InputLayout> native_video_input_layout_;
    ComPtr<ID3D11Buffer> native_video_vertex_buffer_;
    ComPtr<ID3D11Buffer> native_color_matrix_buffer_;
    ComPtr<ID3D11SamplerState> native_sampler_state_;
    ComPtr<ID3D11Texture2D> native_output_texture_;
    ComPtr<ID3D11RenderTargetView> native_output_rtv_;
    ID3D11Texture2D* native_source_texture_ptr_{nullptr};
    intptr_t native_source_index_{0};
    DXGI_FORMAT native_source_format_{DXGI_FORMAT_UNKNOWN};
    ComPtr<ID3D11ShaderResourceView1> native_srv_y_;
    ComPtr<ID3D11ShaderResourceView1> native_srv_uv_;
    PFNWGLDXOPENDEVICENVPROC wgl_dx_open_device_nv_{nullptr};
    PFNWGLDXCLOSEDEVICENVPROC wgl_dx_close_device_nv_{nullptr};
    PFNWGLDXREGISTEROBJECTNVPROC wgl_dx_register_object_nv_{nullptr};
    PFNWGLDXUNREGISTEROBJECTNVPROC wgl_dx_unregister_object_nv_{nullptr};
    PFNWGLDXLOCKOBJECTSNVPROC wgl_dx_lock_objects_nv_{nullptr};
    PFNWGLDXUNLOCKOBJECTSNVPROC wgl_dx_unlock_objects_nv_{nullptr};
    HANDLE interop_device_handle_{nullptr};
    HANDLE interop_object_handle_{nullptr};
    bool interop_object_locked_{false};
    GLuint native_gl_texture_{0};
    int native_output_width_{0};
    int native_output_height_{0};
    std::atomic<bool> native_startup_allowed_{false};
    std::atomic<bool> native_session_disabled_{false};
    D3D11DiagnosticsSnapshot native_diagnostics_{};
#endif
};

bool OpenGLVideoRenderer::Impl::init(const VideoRendererConfig& config) {
    close();
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        LOG_ERROR("SDL init failed for OpenGL renderer: " << SDL_GetError());
        return false;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
#if defined(SDL_GL_CONTEXT_PROFILE_MASK) && defined(SDL_GL_CONTEXT_PROFILE_COMPATIBILITY)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
#endif
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);

    title_ = config.title;
    const WindowSize window_size = computeInitialWindowSize(config.width, config.height);
    width_.store(window_size.width);
    height_.store(window_size.height);
    window_ = SDL_CreateWindow(config.title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                               width_.load(), height_.load(),
                               SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!window_) {
        LOG_ERROR("SDL window creation failed for OpenGL renderer: " << SDL_GetError());
        SDL_Quit();
        return false;
    }
    SDL_SetWindowMinimumSize(window_, kMinWindowWidth, kMinWindowHeight);

    should_quit_.store(false);
    fullscreen_.store(false);
    minimized_.store(false);
    render_running_.store(true);
    render_initialized_.store(false);
    render_init_success_.store(false);
    fullscreen_toggle_requested_.store(false);
    clear_requested_.store(false);
    redraw_requested_.store(false);
    osd_visible_until_ms_.store(0);
    pending_frame_.reset();
    pending_frame_ready_ = false;
    {
        std::lock_guard<std::mutex> lock(request_mutex_);
        requests_ = ControlRequests{};
        last_nonzero_volume_ = 1.0f;
    }
    resetDiagnostics();

    render_thread_ = std::thread(&Impl::renderLoop, this);
    for (int i = 0; i < 200 && !render_initialized_.load(); ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    if (!render_initialized_.load() || !render_init_success_.load()) {
        LOG_ERROR("OpenGL renderer thread initialization failed");
        close();
        return false;
    }

    LOG_INFO("OpenGL renderer initialized: window=" << width_.load() << "x" << height_.load());
    return true;
}

void OpenGLVideoRenderer::Impl::close() {
    render_running_.store(false);
    frame_cv_.notify_all();
    if (render_thread_.joinable()) {
        render_thread_.join();
    }
    {
        std::lock_guard<std::mutex> lock(frame_mutex_);
        pending_frame_.reset();
        pending_frame_ready_ = false;
    }
    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }
    title_.clear();
    width_.store(0);
    height_.store(0);
    should_quit_.store(false);
    fullscreen_.store(false);
    minimized_.store(false);
    render_initialized_.store(false);
    render_init_success_.store(false);
    fullscreen_toggle_requested_.store(false);
    clear_requested_.store(false);
    redraw_requested_.store(false);
    texture_width_ = 0;
    texture_height_ = 0;
    texture_format_ = AV_PIX_FMT_NONE;
    subtitle_texture_width_ = 0;
    subtitle_texture_height_ = 0;
    subtitle_texture_valid_ = false;
    rendered_subtitle_generation_ = 0;
    subtitle_cached_video_rect_ = SDL_Rect{0, 0, 0, 0};
    subtitle_cached_drawable_width_ = 0;
    subtitle_cached_drawable_height_ = 0;
    subtitle_pixels_.clear();
#if defined(_WIN32)
    native_startup_allowed_.store(false);
    native_session_disabled_.store(false);
    native_diagnostics_ = {};
#endif
    SDL_Quit();
}

void OpenGLVideoRenderer::Impl::renderFrame(const core::VideoFrame& frame) {
    if (!frame.valid || !frame.frame || !render_running_.load()) {
        return;
    }

    PendingVideoFrame copy;
    const AVPixelFormat format = static_cast<AVPixelFormat>(frame.frame->format);
    const bool accepted = (format == AV_PIX_FMT_D3D11 && supportsNativeFrameFormat(format))
        ? refNativeFrame(*frame.frame, copy)
        : copyFrameData(*frame.frame, copy);
    if (!accepted) {
        const char* format_name = av_get_pix_fmt_name(format);
        LOG_WARNING("OpenGL renderer rejected frame format: " << (format_name ? format_name : "unknown"));
        return;
    }

    {
        std::lock_guard<std::mutex> lock(frame_mutex_);
        pending_frame_ = std::move(copy);
        pending_frame_ready_ = true;
    }
    redraw_requested_.store(true);
    frame_cv_.notify_one();
}

bool OpenGLVideoRenderer::Impl::copyFrameData(const AVFrame& frame, PendingVideoFrame& out) {
    const AVPixelFormat format = static_cast<AVPixelFormat>(frame.format);
    if ((format != AV_PIX_FMT_YUV420P && format != AV_PIX_FMT_NV12) ||
        frame.width <= 0 || frame.height <= 0 || !frame.data[0] || frame.linesize[0] <= 0) {
        return false;
    }

    const auto copy_start = std::chrono::steady_clock::now();
    out.reset();
    out.width = frame.width;
    out.height = frame.height;
    out.format = format;
    out.color_range = frame.color_range;
    out.color_space = frame.colorspace;
    out.y_pitch = frame.width;
    out.y_plane.resize(static_cast<size_t>(out.y_pitch) * static_cast<size_t>(out.height));
    for (int row = 0; row < frame.height; ++row) {
        std::memcpy(out.y_plane.data() + static_cast<size_t>(row) * static_cast<size_t>(out.y_pitch),
                    frame.data[0] + static_cast<ptrdiff_t>(row) * frame.linesize[0],
                    static_cast<size_t>(out.y_pitch));
    }

    uint64_t copied_bytes = static_cast<uint64_t>(out.y_plane.size());
    const int chroma_width = chromaExtent(frame.width);
    const int chroma_height = chromaExtent(frame.height);
    if (format == AV_PIX_FMT_YUV420P) {
        if (!frame.data[1] || !frame.data[2] || frame.linesize[1] <= 0 || frame.linesize[2] <= 0) {
            out.reset();
            return false;
        }
        out.u_pitch = chroma_width;
        out.v_pitch = chroma_width;
        out.u_plane.resize(static_cast<size_t>(out.u_pitch) * static_cast<size_t>(chroma_height));
        out.v_plane.resize(static_cast<size_t>(out.v_pitch) * static_cast<size_t>(chroma_height));
        for (int row = 0; row < chroma_height; ++row) {
            std::memcpy(out.u_plane.data() + static_cast<size_t>(row) * static_cast<size_t>(out.u_pitch),
                        frame.data[1] + static_cast<ptrdiff_t>(row) * frame.linesize[1],
                        static_cast<size_t>(out.u_pitch));
            std::memcpy(out.v_plane.data() + static_cast<size_t>(row) * static_cast<size_t>(out.v_pitch),
                        frame.data[2] + static_cast<ptrdiff_t>(row) * frame.linesize[2],
                        static_cast<size_t>(out.v_pitch));
        }
        copied_bytes += static_cast<uint64_t>(out.u_plane.size() + out.v_plane.size());
    } else {
        if (!frame.data[1] || frame.linesize[1] <= 0) {
            out.reset();
            return false;
        }
        out.u_pitch = frame.width;
        out.u_plane.resize(static_cast<size_t>(out.u_pitch) * static_cast<size_t>(chroma_height));
        for (int row = 0; row < chroma_height; ++row) {
            std::memcpy(out.u_plane.data() + static_cast<size_t>(row) * static_cast<size_t>(out.u_pitch),
                        frame.data[1] + static_cast<ptrdiff_t>(row) * frame.linesize[1],
                        static_cast<size_t>(out.u_pitch));
        }
        copied_bytes += static_cast<uint64_t>(out.u_plane.size());
    }
    out.valid = true;

    const uint64_t elapsed_us = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - copy_start).count());
    frame_copy_frames_.fetch_add(1);
    frame_copy_bytes_.fetch_add(copied_bytes);
    frame_copy_time_us_total_.fetch_add(elapsed_us);
    updateMaxAtomic(frame_copy_time_us_max_, elapsed_us);
    return true;
}

bool OpenGLVideoRenderer::Impl::refNativeFrame(const AVFrame& frame, PendingVideoFrame& out) {
    if (frame.format != AV_PIX_FMT_D3D11 || frame.width <= 0 || frame.height <= 0 || !frame.data[0]) {
        return false;
    }
    out.reset();
    out.native_frame = av_frame_alloc();
    if (!out.native_frame) {
        return false;
    }
    if (av_frame_ref(out.native_frame, const_cast<AVFrame*>(&frame)) < 0) {
        av_frame_free(&out.native_frame);
        return false;
    }
    out.width = frame.width;
    out.height = frame.height;
    out.format = static_cast<AVPixelFormat>(frame.format);
    out.color_range = frame.color_range;
    out.color_space = frame.colorspace;
    out.valid = true;
    return true;
}

void OpenGLVideoRenderer::Impl::present() {
    redraw_requested_.store(true);
    frame_cv_.notify_one();
}

void OpenGLVideoRenderer::Impl::clear() {
    clear_requested_.store(true);
    frame_cv_.notify_one();
}

void OpenGLVideoRenderer::Impl::renderLoop() {
    PendingVideoFrame last_frame;
    bool has_last_frame = false;

    const bool init_ok = createGlContext();
    render_init_success_.store(init_ok);
    render_initialized_.store(true);
    if (!init_ok) {
        destroyGlResources();
        return;
    }

    while (render_running_.load()) {
        pumpEvents();

        PendingVideoFrame incoming_frame;
        bool have_new_frame = false;
        bool clear_now = clear_requested_.exchange(false);
        bool redraw_now = redraw_requested_.exchange(false);
        bool fullscreen_now = fullscreen_toggle_requested_.exchange(false);

        {
            std::unique_lock<std::mutex> lock(frame_mutex_);
            if (!pending_frame_ready_ && !clear_now && !redraw_now && !fullscreen_now) {
                frame_cv_.wait_for(lock, std::chrono::milliseconds(8), [this] {
                    return !render_running_.load() || pending_frame_ready_ || clear_requested_.load() ||
                           redraw_requested_.load() || fullscreen_toggle_requested_.load();
                });
            }
            if (pending_frame_ready_) {
                incoming_frame = std::move(pending_frame_);
                pending_frame_.reset();
                pending_frame_ready_ = false;
                have_new_frame = incoming_frame.valid;
            }
        }

        clear_now = clear_now || clear_requested_.exchange(false);
        redraw_now = redraw_now || redraw_requested_.exchange(false);
        fullscreen_now = fullscreen_now || fullscreen_toggle_requested_.exchange(false);
        if (!render_running_.load()) {
            break;
        }

        if (fullscreen_now && window_) {
            const bool next_fullscreen = !fullscreen_.load();
            if (SDL_SetWindowFullscreen(window_, next_fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0) == 0) {
                fullscreen_.store(next_fullscreen);
            } else {
                LOG_WARNING("OpenGL fullscreen toggle failed: " << SDL_GetError());
            }
            updateWindowSizeFromSdl(window_, width_, height_);
            minimized_.store(false);
            redraw_now = true;
        }

        if (minimized_.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(8));
            continue;
        }

        if (clear_now) {
            last_frame.reset();
            has_last_frame = false;
            renderCurrentFrame(nullptr);
            continue;
        }

        const PendingVideoFrame* frame_to_render = nullptr;
        if (have_new_frame) {
            frame_to_render = &incoming_frame;
        } else if (redraw_now && has_last_frame) {
            frame_to_render = &last_frame;
        }

        if (frame_to_render) {
            renderCurrentFrame(frame_to_render);
            if (have_new_frame) {
                last_frame = std::move(incoming_frame);
                has_last_frame = last_frame.valid;
            }
        } else if (redraw_now) {
            renderCurrentFrame(nullptr);
        }
    }

    destroyGlResources();
}

void OpenGLVideoRenderer::Impl::requestRedraw() {
    redraw_requested_.store(true);
    frame_cv_.notify_one();
}

void OpenGLVideoRenderer::Impl::requestRedrawIfPaused() {
    if (overlay_paused_.load()) {
        requestRedraw();
    }
}

void OpenGLVideoRenderer::Impl::showOsdFor(uint64_t duration_ms) {
    const uint64_t deadline = monotonicMsNow() + duration_ms;
    uint64_t current = osd_visible_until_ms_.load();
    while (deadline > current && !osd_visible_until_ms_.compare_exchange_weak(current, deadline)) {
    }
}

bool OpenGLVideoRenderer::Impl::createGlContext() {
    if (!window_) {
        LOG_ERROR("OpenGL renderer missing SDL window before context creation");
        return false;
    }
    gl_context_ = SDL_GL_CreateContext(window_);
    if (!gl_context_) {
        LOG_ERROR("SDL_GL_CreateContext failed: " << SDL_GetError());
        return false;
    }
    if (SDL_GL_MakeCurrent(window_, gl_context_) != 0) {
        LOG_ERROR("SDL_GL_MakeCurrent failed: " << SDL_GetError());
        destroyGlResources();
        return false;
    }
    if (!loadGlFunctions()) {
        destroyGlResources();
        return false;
    }
    if (SDL_GL_SetSwapInterval(0) != 0) {
        LOG_WARNING("OpenGL renderer could not disable swap interval: " << SDL_GetError());
    }

    const GLubyte* vendor = glGetString(GL_VENDOR);
    const GLubyte* renderer = glGetString(GL_RENDERER);
    const GLubyte* version = glGetString(GL_VERSION);
    LOG_INFO("OpenGL context: vendor=" << (vendor ? reinterpret_cast<const char*>(vendor) : "unknown")
             << " renderer=" << (renderer ? reinterpret_cast<const char*>(renderer) : "unknown")
             << " version=" << (version ? reinterpret_cast<const char*>(version) : "unknown"));

    const GLuint vertex_shader = compileShader(GL_VERTEX_SHADER, kVertexShaderSource, "opengl-video-vertex");
    const GLuint yuv420_fragment_shader = compileShader(GL_FRAGMENT_SHADER, kYuv420FragmentShaderSource, "opengl-yuv420-fragment");
    const GLuint nv12_fragment_shader = compileShader(GL_FRAGMENT_SHADER, kNv12FragmentShaderSource, "opengl-nv12-fragment");
    if (vertex_shader == 0 || yuv420_fragment_shader == 0 || nv12_fragment_shader == 0) {
        if (vertex_shader != 0) g_gl.DeleteShader(vertex_shader);
        if (yuv420_fragment_shader != 0) g_gl.DeleteShader(yuv420_fragment_shader);
        if (nv12_fragment_shader != 0) g_gl.DeleteShader(nv12_fragment_shader);
        destroyGlResources();
        return false;
    }

    yuv420_program_ = linkProgram(vertex_shader, yuv420_fragment_shader, "opengl-yuv420-program");
    nv12_program_ = linkProgram(vertex_shader, nv12_fragment_shader, "opengl-nv12-program");
    g_gl.DeleteShader(vertex_shader);
    g_gl.DeleteShader(yuv420_fragment_shader);
    g_gl.DeleteShader(nv12_fragment_shader);
    if (yuv420_program_ == 0 || nv12_program_ == 0) {
        destroyGlResources();
        return false;
    }

    yuv420_tex_y_location_ = g_gl.GetUniformLocation(yuv420_program_, "texY");
    yuv420_tex_u_location_ = g_gl.GetUniformLocation(yuv420_program_, "texU");
    yuv420_tex_v_location_ = g_gl.GetUniformLocation(yuv420_program_, "texV");
    yuv420_coeff_r_location_ = g_gl.GetUniformLocation(yuv420_program_, "coeffR");
    yuv420_coeff_g_location_ = g_gl.GetUniformLocation(yuv420_program_, "coeffG");
    yuv420_coeff_b_location_ = g_gl.GetUniformLocation(yuv420_program_, "coeffB");
    nv12_tex_y_location_ = g_gl.GetUniformLocation(nv12_program_, "texY");
    nv12_tex_uv_location_ = g_gl.GetUniformLocation(nv12_program_, "texUV");
    nv12_coeff_r_location_ = g_gl.GetUniformLocation(nv12_program_, "coeffR");
    nv12_coeff_g_location_ = g_gl.GetUniformLocation(nv12_program_, "coeffG");
    nv12_coeff_b_location_ = g_gl.GetUniformLocation(nv12_program_, "coeffB");

    g_gl.UseProgram(yuv420_program_);
    g_gl.Uniform1i(yuv420_tex_y_location_, 0);
    g_gl.Uniform1i(yuv420_tex_u_location_, 1);
    g_gl.Uniform1i(yuv420_tex_v_location_, 2);
    g_gl.UseProgram(nv12_program_);
    g_gl.Uniform1i(nv12_tex_y_location_, 0);
    g_gl.Uniform1i(nv12_tex_uv_location_, 1);
    g_gl.UseProgram(0);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

#if defined(_WIN32)
    if (!initializeNativeInterop()) {
        LOG_INFO("OpenGL native D3D11 interop is not enabled for this session; software/direct upload remains active");
    }
#endif
    return true;
}

void OpenGLVideoRenderer::Impl::destroyGlResources() {
    auto reset_state_only = [this] {
        yuv420_program_ = 0;
        nv12_program_ = 0;
        yuv420_tex_y_location_ = yuv420_tex_u_location_ = yuv420_tex_v_location_ = -1;
        yuv420_coeff_r_location_ = yuv420_coeff_g_location_ = yuv420_coeff_b_location_ = -1;
        nv12_tex_y_location_ = nv12_tex_uv_location_ = -1;
        nv12_coeff_r_location_ = nv12_coeff_g_location_ = nv12_coeff_b_location_ = -1;
        tex_y_ = tex_u_ = tex_v_ = tex_uv_ = 0;
        texture_width_ = texture_height_ = 0;
        texture_format_ = AV_PIX_FMT_NONE;
        subtitle_texture_ = 0;
        subtitle_texture_width_ = subtitle_texture_height_ = 0;
        subtitle_texture_valid_ = false;
    };

    if (!gl_context_) {
        reset_state_only();
        return;
    }
    if (window_) SDL_GL_MakeCurrent(window_, gl_context_);
    if (g_gl.UseProgram) g_gl.UseProgram(0);
#if defined(_WIN32)
    destroyNativeInterop();
#endif
    if (subtitle_texture_ != 0) glDeleteTextures(1, &subtitle_texture_);
    if (tex_y_ != 0) glDeleteTextures(1, &tex_y_);
    if (tex_u_ != 0) glDeleteTextures(1, &tex_u_);
    if (tex_v_ != 0) glDeleteTextures(1, &tex_v_);
    if (tex_uv_ != 0) glDeleteTextures(1, &tex_uv_);
    if (yuv420_program_ != 0 && g_gl.DeleteProgram) g_gl.DeleteProgram(yuv420_program_);
    if (nv12_program_ != 0 && g_gl.DeleteProgram) g_gl.DeleteProgram(nv12_program_);
    SDL_GL_DeleteContext(gl_context_);
    gl_context_ = nullptr;
    reset_state_only();
}

bool OpenGLVideoRenderer::Impl::ensureGlTextures(const PendingVideoFrame& frame) {
    if (frame.width <= 0 || frame.height <= 0) return false;
    if (texture_width_ == frame.width && texture_height_ == frame.height && texture_format_ == frame.format) return true;

    if (tex_y_ != 0) glDeleteTextures(1, &tex_y_);
    if (tex_u_ != 0) glDeleteTextures(1, &tex_u_);
    if (tex_v_ != 0) glDeleteTextures(1, &tex_v_);
    if (tex_uv_ != 0) glDeleteTextures(1, &tex_uv_);
    tex_y_ = tex_u_ = tex_v_ = tex_uv_ = 0;

    const auto create_texture = [](GLuint& texture_id, GLenum internal_format, int width, int height) -> bool {
        glGenTextures(1, &texture_id);
        if (texture_id == 0) return false;
        glBindTexture(GL_TEXTURE_2D, texture_id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(internal_format), width, height, 0, internal_format, GL_UNSIGNED_BYTE, nullptr);
        return glGetError() == GL_NO_ERROR;
    };

    if (!create_texture(tex_y_, GL_LUMINANCE, frame.width, frame.height)) {
        LOG_ERROR("OpenGL texture allocation failed for Y plane");
        return false;
    }
    const int chroma_width = chromaExtent(frame.width);
    const int chroma_height = chromaExtent(frame.height);
    bool ok = false;
    if (frame.format == AV_PIX_FMT_YUV420P) {
        ok = create_texture(tex_u_, GL_LUMINANCE, chroma_width, chroma_height) &&
             create_texture(tex_v_, GL_LUMINANCE, chroma_width, chroma_height);
    } else if (frame.format == AV_PIX_FMT_NV12) {
        ok = create_texture(tex_uv_, GL_LUMINANCE_ALPHA, chroma_width, chroma_height);
    }
    if (!ok) {
        LOG_ERROR("OpenGL texture allocation failed for chroma planes");
        return false;
    }
    texture_width_ = frame.width;
    texture_height_ = frame.height;
    texture_format_ = frame.format;
    return true;
}

bool OpenGLVideoRenderer::Impl::uploadFrameTextures(const PendingVideoFrame& frame) {
    if (!ensureGlTextures(frame)) return false;
    g_gl.ActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex_y_);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, frame.width, frame.height, GL_LUMINANCE, GL_UNSIGNED_BYTE, frame.y_plane.data());
    if (glGetError() != GL_NO_ERROR) return false;

    const int chroma_width = chromaExtent(frame.width);
    const int chroma_height = chromaExtent(frame.height);
    if (frame.format == AV_PIX_FMT_YUV420P) {
        g_gl.ActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, tex_u_);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, chroma_width, chroma_height, GL_LUMINANCE, GL_UNSIGNED_BYTE, frame.u_plane.data());
        if (glGetError() != GL_NO_ERROR) return false;
        g_gl.ActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, tex_v_);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, chroma_width, chroma_height, GL_LUMINANCE, GL_UNSIGNED_BYTE, frame.v_plane.data());
        if (glGetError() != GL_NO_ERROR) return false;
    } else if (frame.format == AV_PIX_FMT_NV12) {
        g_gl.ActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, tex_uv_);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, chroma_width, chroma_height, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, frame.u_plane.data());
        if (glGetError() != GL_NO_ERROR) return false;
    } else {
        return false;
    }
    return true;
}

void OpenGLVideoRenderer::Impl::drawAspectQuad(int drawable_width, int drawable_height, int frame_width, int frame_height) {
    float x_scale = 1.0f;
    float y_scale = 1.0f;
    const double frame_aspect = static_cast<double>(std::max(1, frame_width)) / static_cast<double>(std::max(1, frame_height));
    const double window_aspect = static_cast<double>(std::max(1, drawable_width)) / static_cast<double>(std::max(1, drawable_height));
    if (window_aspect > frame_aspect) x_scale = static_cast<float>(frame_aspect / window_aspect);
    else y_scale = static_cast<float>(window_aspect / frame_aspect);

    glBegin(GL_TRIANGLE_STRIP);
    glTexCoord2f(0.0f, 1.0f); glVertex2f(-x_scale, -y_scale);
    glTexCoord2f(0.0f, 0.0f); glVertex2f(-x_scale, y_scale);
    glTexCoord2f(1.0f, 1.0f); glVertex2f(x_scale, -y_scale);
    glTexCoord2f(1.0f, 0.0f); glVertex2f(x_scale, y_scale);
    glEnd();
}

void OpenGLVideoRenderer::Impl::renderSoftwareFrame(const PendingVideoFrame& frame, int drawable_width, int drawable_height) {
    if (!uploadFrameTextures(frame)) return;
    const ColorCoefficients coeffs = buildColorCoefficients(frame.color_range, frame.color_space);
    const GLuint program = frame.format == AV_PIX_FMT_NV12 ? nv12_program_ : yuv420_program_;
    g_gl.UseProgram(program);
    if (frame.format == AV_PIX_FMT_NV12) {
        g_gl.Uniform4f(nv12_coeff_r_location_, coeffs.r[0], coeffs.r[1], coeffs.r[2], coeffs.r[3]);
        g_gl.Uniform4f(nv12_coeff_g_location_, coeffs.g[0], coeffs.g[1], coeffs.g[2], coeffs.g[3]);
        g_gl.Uniform4f(nv12_coeff_b_location_, coeffs.b[0], coeffs.b[1], coeffs.b[2], coeffs.b[3]);
    } else {
        g_gl.Uniform4f(yuv420_coeff_r_location_, coeffs.r[0], coeffs.r[1], coeffs.r[2], coeffs.r[3]);
        g_gl.Uniform4f(yuv420_coeff_g_location_, coeffs.g[0], coeffs.g[1], coeffs.g[2], coeffs.g[3]);
        g_gl.Uniform4f(yuv420_coeff_b_location_, coeffs.b[0], coeffs.b[1], coeffs.b[2], coeffs.b[3]);
    }
    drawAspectQuad(drawable_width, drawable_height, frame.width, frame.height);
    g_gl.UseProgram(0);
}

void OpenGLVideoRenderer::Impl::drawFilledRect(int drawable_width, int drawable_height, int x, int y, int w, int h, float r, float g, float b, float a) {
    if (w <= 0 || h <= 0) return;
    const float left = (static_cast<float>(x) / static_cast<float>(drawable_width)) * 2.0f - 1.0f;
    const float right = (static_cast<float>(x + w) / static_cast<float>(drawable_width)) * 2.0f - 1.0f;
    const float top = 1.0f - (static_cast<float>(y) / static_cast<float>(drawable_height)) * 2.0f;
    const float bottom = 1.0f - (static_cast<float>(y + h) / static_cast<float>(drawable_height)) * 2.0f;
    glColor4f(r, g, b, a);
    glBegin(GL_TRIANGLE_STRIP);
    glVertex2f(left, top); glVertex2f(left, bottom); glVertex2f(right, top); glVertex2f(right, bottom);
    glEnd();
}

void OpenGLVideoRenderer::Impl::renderNativeFrame(const PendingVideoFrame& frame, int drawable_width, int drawable_height) {
#if defined(_WIN32)
    if (!updateNativeGlTexture(frame.native_frame) || native_gl_texture_ == 0) return;
    g_gl.UseProgram(0);
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glBindTexture(GL_TEXTURE_2D, native_gl_texture_);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    drawAspectQuad(drawable_width, drawable_height, frame.width, frame.height);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
#else
    (void)frame; (void)drawable_width; (void)drawable_height;
#endif
}

void OpenGLVideoRenderer::Impl::renderCurrentFrame(const PendingVideoFrame* frame) {
    if (!window_ || !gl_context_) return;
    SDL_GL_MakeCurrent(window_, gl_context_);

    int drawable_width = 0;
    int drawable_height = 0;
    SDL_GL_GetDrawableSize(window_, &drawable_width, &drawable_height);
    drawable_width = std::max(1, drawable_width);
    drawable_height = std::max(1, drawable_height);
    glViewport(0, 0, drawable_width, drawable_height);
    glClear(GL_COLOR_BUFFER_BIT);

    if (frame && frame->valid) {
        if (frame->isNative()) renderNativeFrame(*frame, drawable_width, drawable_height);
        else renderSoftwareFrame(*frame, drawable_width, drawable_height);
    }
    drawSubtitleOverlay(frame, drawable_width, drawable_height);
    drawOsdOverlay(frame, drawable_width, drawable_height);
    SDL_GL_SwapWindow(window_);
}

OpenGLVideoRenderer::Impl::SubtitleSnapshot OpenGLVideoRenderer::Impl::snapshotSubtitleState() const {
    std::lock_guard<std::mutex> lock(subtitle_mutex_);
    SubtitleSnapshot snapshot;
    snapshot.text = subtitle::flattenSubtitleText(subtitle_items_);
    if (!subtitle_items_.empty()) {
        snapshot.style = subtitle_items_.front().style;
        snapshot.play_res_x = subtitle_items_.front().play_res_x;
        snapshot.play_res_y = subtitle_items_.front().play_res_y;
    }
    return snapshot;
}

bool OpenGLVideoRenderer::Impl::updateSubtitleTextureIfNeeded(const PendingVideoFrame* frame, int drawable_width, int drawable_height) {
    const SDL_Rect video_rect = frame && frame->valid
        ? computeRenderRect(drawable_width, drawable_height, frame->width, frame->height)
        : SDL_Rect{0, 0, drawable_width, drawable_height};
    const uint64_t generation = subtitle_generation_.load();
    if (generation == rendered_subtitle_generation_ &&
        drawable_width == subtitle_cached_drawable_width_ &&
        drawable_height == subtitle_cached_drawable_height_ &&
        subtitle_cached_video_rect_.x == video_rect.x && subtitle_cached_video_rect_.y == video_rect.y &&
        subtitle_cached_video_rect_.w == video_rect.w && subtitle_cached_video_rect_.h == video_rect.h) {
        return true;
    }

    subtitle_cached_drawable_width_ = drawable_width;
    subtitle_cached_drawable_height_ = drawable_height;
    subtitle_cached_video_rect_ = video_rect;
    rendered_subtitle_generation_ = generation;
    subtitle_texture_valid_ = false;

    const SubtitleSnapshot subtitle_state = snapshotSubtitleState();
    if (subtitle_state.text.empty()) return true;

#if defined(_WIN32)
    GdiSurface mask_surface;
    if (!createGdiSurface(drawable_width, drawable_height, mask_surface)) return false;
    subtitle_pixels_.assign(static_cast<size_t>(drawable_width) * static_cast<size_t>(drawable_height) * 4u, 0u);

    const std::wstring wide_text = utf8ToWide(subtitle_state.text);
    if (wide_text.empty()) { destroyGdiSurface(mask_surface); return true; }
    std::wstring wide_font = utf8ToWide(subtitle_state.style.font_family);
    if (wide_font.empty()) wide_font = L"Microsoft YaHei UI";

    float font_size = static_cast<float>(subtitle_state.style.font_size > 0.0 ? subtitle_state.style.font_size : 36.0);
    if (subtitle_state.play_res_y > 0 && subtitle_state.style.font_size > 0.0) {
        font_size = static_cast<float>(subtitle_state.style.font_size * static_cast<double>(video_rect.h) /
                                       static_cast<double>(subtitle_state.play_res_y));
    }
    font_size = std::clamp(font_size, 18.0f, std::max(36.0f, static_cast<float>(video_rect.h) * 0.18f));
    const int logical_dpi = GetDeviceCaps(mask_surface.dc, LOGPIXELSY);
    const int font_height = -MulDiv(static_cast<int>(std::lround(font_size)), logical_dpi, 72);
    HFONT font = CreateFontW(font_height, 0, 0, 0,
                             subtitle_state.style.bold ? FW_BOLD : FW_NORMAL,
                             subtitle_state.style.italic ? TRUE : FALSE,
                             subtitle_state.style.underline ? TRUE : FALSE,
                             subtitle_state.style.strikeout ? TRUE : FALSE,
                             DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                             ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, wide_font.c_str());
    if (!font) font = CreateFontW(font_height, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                  DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                  ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei UI");
    if (!font) { destroyGdiSurface(mask_surface); return false; }
    const HGDIOBJ old_font = SelectObject(mask_surface.dc, font);

    const float fallback_margin_x = std::max(24.0f, static_cast<float>(video_rect.w) * 0.06f);
    const float fallback_margin_y = std::max(18.0f, static_cast<float>(video_rect.h) * 0.05f);
    const float scale_x = subtitle_state.play_res_x > 0 ? static_cast<float>(video_rect.w) / static_cast<float>(subtitle_state.play_res_x) : 1.0f;
    const float scale_y = subtitle_state.play_res_y > 0 ? static_cast<float>(video_rect.h) / static_cast<float>(subtitle_state.play_res_y) : 1.0f;
    const int margin_left = static_cast<int>(std::lround(std::max(fallback_margin_x, static_cast<float>(subtitle_state.style.margin_l) * scale_x)));
    const int margin_right = static_cast<int>(std::lround(std::max(fallback_margin_x, static_cast<float>(subtitle_state.style.margin_r) * scale_x)));
    const int margin_vertical = static_cast<int>(std::lround(std::max(fallback_margin_y, static_cast<float>(subtitle_state.style.margin_v) * scale_y)));
    const int reserved_bottom = kControlPanelHeight + kControlPanelInset + 8;
    const int left_limit = video_rect.x + margin_left;
    const int right_limit = video_rect.x + video_rect.w - margin_right;
    const int top_limit = video_rect.y + margin_vertical;
    const int bottom_limit = video_rect.y + video_rect.h - margin_vertical - reserved_bottom;
    if (right_limit <= left_limit || bottom_limit <= top_limit) {
        SelectObject(mask_surface.dc, old_font); DeleteObject(font); destroyGdiSurface(mask_surface); return true;
    }

    UINT draw_flags = DT_WORDBREAK | DT_NOPREFIX;
    if (subtitleHorizontalGroup(subtitle_state.style.alignment) == 0) draw_flags |= DT_LEFT;
    else if (subtitleHorizontalGroup(subtitle_state.style.alignment) == 2) draw_flags |= DT_RIGHT;
    else draw_flags |= DT_CENTER;

    RECT measure_rect{left_limit, top_limit, right_limit, bottom_limit};
    DrawTextW(mask_surface.dc, wide_text.c_str(), static_cast<int>(wide_text.size()), &measure_rect, draw_flags | DT_CALCRECT);
    const int text_width = (std::max)(1, static_cast<int>(measure_rect.right - measure_rect.left));
    const int text_height = (std::max)(1, static_cast<int>(measure_rect.bottom - measure_rect.top));
    RECT draw_rect{left_limit, top_limit, left_limit + text_width, top_limit + text_height};

    if (subtitle_state.style.has_position && subtitle_state.play_res_x > 0 && subtitle_state.play_res_y > 0) {
        const int anchor_x = video_rect.x + static_cast<int>(std::lround(subtitle_state.style.position_x * scale_x));
        const int anchor_y = video_rect.y + static_cast<int>(std::lround(subtitle_state.style.position_y * scale_y));
        draw_rect.left = anchor_x - text_width / 2; draw_rect.right = draw_rect.left + text_width;
        draw_rect.top = anchor_y - text_height / 2; draw_rect.bottom = draw_rect.top + text_height;
    } else {
        const int h_group = subtitleHorizontalGroup(subtitle_state.style.alignment);
        const int v_group = subtitleVerticalGroup(subtitle_state.style.alignment);
        if (h_group == 0) { draw_rect.left = left_limit; draw_rect.right = (std::min)(right_limit, static_cast<int>(draw_rect.left) + text_width); }
        else if (h_group == 2) { draw_rect.right = right_limit; draw_rect.left = (std::max)(left_limit, static_cast<int>(draw_rect.right) - text_width); }
        else { draw_rect.left = left_limit + ((right_limit - left_limit) - text_width) / 2; draw_rect.right = draw_rect.left + text_width; }
        if (v_group == 2) { draw_rect.top = top_limit; draw_rect.bottom = draw_rect.top + text_height; }
        else if (v_group == 1) { draw_rect.top = top_limit + ((bottom_limit - top_limit) - text_height) / 2; draw_rect.bottom = draw_rect.top + text_height; }
        else { draw_rect.bottom = bottom_limit; draw_rect.top = draw_rect.bottom - text_height; }
    }

    draw_rect.left = (std::max)(0, static_cast<int>(draw_rect.left));
    draw_rect.top = (std::max)(0, static_cast<int>(draw_rect.top));
    draw_rect.right = (std::min)(drawable_width, static_cast<int>(draw_rect.right));
    draw_rect.bottom = (std::min)(drawable_height, static_cast<int>(draw_rect.bottom));
    if (draw_rect.right <= draw_rect.left || draw_rect.bottom <= draw_rect.top) {
        SelectObject(mask_surface.dc, old_font); DeleteObject(font); destroyGdiSurface(mask_surface); return true;
    }

    const int outline_px = (std::max)(1, static_cast<int>(std::lround((std::max)(1.0, subtitle_state.style.outline))));
    const int shadow_px = (std::max)(1, static_cast<int>(std::lround((std::max)(1.0, subtitle_state.style.shadow))));
    if (subtitle_state.style.background_color.a > 0 || subtitle_state.style.border_style == 3) {
        RECT background_rect = draw_rect;
        background_rect.left -= outline_px + 8; background_rect.top -= outline_px + 6;
        background_rect.right += outline_px + 8; background_rect.bottom += outline_px + 6;
        fillRectBGRA(subtitle_pixels_, drawable_width, drawable_height, background_rect, subtitle_state.style.background_color);
    }

    auto draw_masked_text = [&](int offset_x, int offset_y, const subtitle::SubtitleColor& color) {
        clearGdiSurface(mask_surface);
        RECT text_rect = draw_rect;
        OffsetRect(&text_rect, offset_x, offset_y);
        SetTextColor(mask_surface.dc, RGB(255, 255, 255));
        DrawTextW(mask_surface.dc, wide_text.c_str(), static_cast<int>(wide_text.size()), &text_rect, draw_flags);
        compositeMaskToPixels(mask_surface, subtitle_pixels_, color);
    };
    for (int dy = -outline_px; dy <= outline_px; ++dy) {
        for (int dx = -outline_px; dx <= outline_px; ++dx) {
            if (dx == 0 && dy == 0) continue;
            draw_masked_text(dx, dy, subtitle_state.style.outline_color);
        }
    }
    draw_masked_text(shadow_px, shadow_px, subtitle::SubtitleColor(0, 0, 0, std::max<uint8_t>(80, subtitle_state.style.outline_color.a / 2)));
    draw_masked_text(0, 0, subtitle_state.style.primary_color);

    SelectObject(mask_surface.dc, old_font); DeleteObject(font); destroyGdiSurface(mask_surface);

    if (subtitle_texture_ == 0 || subtitle_texture_width_ != drawable_width || subtitle_texture_height_ != drawable_height) {
        if (subtitle_texture_ != 0) glDeleteTextures(1, &subtitle_texture_);
        glGenTextures(1, &subtitle_texture_);
        if (subtitle_texture_ == 0) return false;
        subtitle_texture_width_ = drawable_width;
        subtitle_texture_height_ = drawable_height;
        glBindTexture(GL_TEXTURE_2D, subtitle_texture_);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, drawable_width, drawable_height, 0, GL_BGRA, GL_UNSIGNED_BYTE, subtitle_pixels_.data());
    } else {
        glBindTexture(GL_TEXTURE_2D, subtitle_texture_);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, drawable_width, drawable_height, GL_BGRA, GL_UNSIGNED_BYTE, subtitle_pixels_.data());
    }
    subtitle_texture_valid_ = glGetError() == GL_NO_ERROR;
    glBindTexture(GL_TEXTURE_2D, 0);
    return subtitle_texture_valid_;
#else
    (void)subtitle_state;
    subtitle_texture_valid_ = false;
    return true;
#endif
}

void OpenGLVideoRenderer::Impl::drawSubtitleOverlay(const PendingVideoFrame* frame, int drawable_width, int drawable_height) {
    if (!updateSubtitleTextureIfNeeded(frame, drawable_width, drawable_height) || !subtitle_texture_valid_ || subtitle_texture_ == 0) return;
    g_gl.UseProgram(0);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, subtitle_texture_);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glBegin(GL_TRIANGLE_STRIP);
    glTexCoord2f(0.0f, 1.0f); glVertex2f(-1.0f, -1.0f);
    glTexCoord2f(0.0f, 0.0f); glVertex2f(-1.0f, 1.0f);
    glTexCoord2f(1.0f, 1.0f); glVertex2f(1.0f, -1.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex2f(1.0f, 1.0f);
    glEnd();
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
}

void OpenGLVideoRenderer::Impl::drawOsdOverlay(const PendingVideoFrame* frame, int drawable_width, int drawable_height) {
    const bool paused = overlay_paused_.load();
    const bool show_osd = paused || monotonicMsNow() <= osd_visible_until_ms_.load();
    if (!show_osd) return;

    double progress = 0.0;
    const double duration = overlay_duration_.load();
    if (duration > 0.001) progress = clampRatio(overlay_position_.load() / duration);
    const float volume = clampVolume(overlay_volume_.load());

    g_gl.UseProgram(0);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    const int panel_width = std::max(1, drawable_width - (kControlPanelInset * 2));
    const int panel_x = kControlPanelInset;
    const int panel_y = std::max(0, drawable_height - kControlPanelHeight - kControlPanelInset);
    const int track_y = panel_y + (kControlPanelHeight - kBarHeight) / 2;
    const int volume_width = std::min(kVolumeBarWidth, std::max(56, panel_width / 5));
    const int progress_width = std::max(kMinProgressBarWidth, panel_width - (kControlPadding * 2) - volume_width - kControlGap);
    const int progress_x = panel_x + kControlPadding;
    const int volume_x = panel_x + panel_width - kControlPadding - volume_width;

    drawFilledRect(drawable_width, drawable_height, panel_x, panel_y, panel_width, kControlPanelHeight, 0.02f, 0.02f, 0.02f, 0.58f);
    drawFilledRect(drawable_width, drawable_height, progress_x, track_y, progress_width, kBarHeight, 0.18f, 0.18f, 0.18f, 0.9f);
    drawFilledRect(drawable_width, drawable_height, progress_x, track_y, static_cast<int>(std::lround(progress * progress_width)), kBarHeight, 0.96f, 0.68f, 0.18f, 0.96f);
    drawFilledRect(drawable_width, drawable_height, volume_x, track_y, volume_width, kBarHeight, 0.18f, 0.18f, 0.18f, 0.9f);
    drawFilledRect(drawable_width, drawable_height, volume_x, track_y, static_cast<int>(std::lround(volume * volume_width)), kBarHeight, 0.42f, 0.82f, 0.58f, 0.96f);

    if (paused) {
        const SDL_Rect video_rect = frame && frame->valid
            ? computeRenderRect(drawable_width, drawable_height, frame->width, frame->height)
            : SDL_Rect{0, 0, drawable_width, drawable_height};
        const int badge_width = std::max(42, video_rect.w / 10);
        const int badge_height = std::max(56, video_rect.h / 6);
        const int gap = std::max(10, badge_width / 3);
        const int bar_width = std::max(10, badge_width / 4);
        const int center_x = video_rect.x + video_rect.w / 2;
        const int center_y = video_rect.y + video_rect.h / 2;
        const int top = center_y - badge_height / 2;
        const int left_bar = center_x - gap / 2 - bar_width;
        const int right_bar = center_x + gap / 2;
        drawFilledRect(drawable_width, drawable_height, left_bar, top, bar_width, badge_height, 1.0f, 1.0f, 1.0f, 0.72f);
        drawFilledRect(drawable_width, drawable_height, right_bar, top, bar_width, badge_height, 1.0f, 1.0f, 1.0f, 0.72f);
    }
    glDisable(GL_BLEND);
}

void OpenGLVideoRenderer::Impl::pumpEvents() {
    SDL_Event event{};
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_QUIT: should_quit_.store(true); break;
        case SDL_KEYDOWN: handleKeyDown(static_cast<int>(event.key.keysym.sym), static_cast<unsigned short>(event.key.keysym.mod)); break;
        case SDL_WINDOWEVENT:
            if (event.window.event == SDL_WINDOWEVENT_MINIMIZED || event.window.event == SDL_WINDOWEVENT_HIDDEN) minimized_.store(true);
            else if (event.window.event == SDL_WINDOWEVENT_RESTORED || event.window.event == SDL_WINDOWEVENT_SHOWN ||
                     event.window.event == SDL_WINDOWEVENT_MAXIMIZED || event.window.event == SDL_WINDOWEVENT_RESIZED ||
                     event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                minimized_.store(false);
                requestRedraw();
            }
            if (event.window.event == SDL_WINDOWEVENT_RESIZED || event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED ||
                event.window.event == SDL_WINDOWEVENT_MAXIMIZED || event.window.event == SDL_WINDOWEVENT_RESTORED) {
                updateWindowSizeFromSdl(window_, width_, height_);
                requestRedraw();
            }
            break;
        default: break;
        }
    }
}

void OpenGLVideoRenderer::Impl::handleKeyDown(int key_code, unsigned short modifiers) {
    if (key_code == SDLK_ESCAPE) {
        if (fullscreen_.load()) requestFullscreenToggle();
        else should_quit_.store(true);
        return;
    }
    if (key_code == SDLK_RETURN || key_code == SDLK_KP_ENTER) {
        requestFullscreenToggle();
        return;
    }
    std::optional<input::PlayerAction> action;
    {
        std::lock_guard<std::mutex> lock(request_mutex_);
        action = hotkey_manager_.actionForKey(key_code);
    }
    if (!action && key_code == SDLK_EQUALS) action = input::PlayerAction::VolumeUp;
    else if (!action && key_code == SDLK_MINUS) action = input::PlayerAction::VolumeDown;
    if (action) applyAction(*action, modifiers);
}

void OpenGLVideoRenderer::Impl::applyAction(input::PlayerAction action, unsigned short modifiers) {
    bool should_show_osd = false;
    {
        std::lock_guard<std::mutex> lock(request_mutex_);
        switch (action) {
        case input::PlayerAction::PlayPause: requests_.toggle_pause_requested = true; should_show_osd = true; break;
        case input::PlayerAction::SeekBackward:
        case input::PlayerAction::SeekForward: {
            const double delta = ((modifiers & KMOD_CTRL) != 0) ? kSeekStepSecondsCtrl : kSeekStepSeconds;
            requests_.seek_delta_seconds += action == input::PlayerAction::SeekForward ? delta : -delta;
            requests_.seek_delta_requested = true; should_show_osd = true; break; }
        case input::PlayerAction::VolumeUp:
        case input::PlayerAction::VolumeDown: {
            const float next = clampVolume(overlay_volume_.load() + (action == input::PlayerAction::VolumeUp ? 1.0f : -1.0f) * kVolumeStep);
            requests_.requested_volume = next; requests_.volume_change_requested = true; overlay_volume_.store(next);
            if (next > 0.0001f) last_nonzero_volume_ = next; should_show_osd = true; break; }
        case input::PlayerAction::ToggleMute: {
            const float current = clampVolume(overlay_volume_.load());
            float next = current > 0.0001f ? 0.0f : std::max(0.5f, clampVolume(last_nonzero_volume_));
            if (current > 0.0001f) last_nonzero_volume_ = current;
            requests_.requested_volume = next; requests_.volume_change_requested = true; overlay_volume_.store(next); should_show_osd = true; break; }
        case input::PlayerAction::SpeedDown:
        case input::PlayerAction::SpeedUp: requests_.speed_delta += action == input::PlayerAction::SpeedUp ? kSpeedStep : -kSpeedStep; requests_.speed_change_requested = true; should_show_osd = true; break;
        case input::PlayerAction::SpeedReset: requests_.speed_reset_requested = true; should_show_osd = true; break;
        case input::PlayerAction::SetABRepeatStart: requests_.ab_repeat_start_requested = true; should_show_osd = true; break;
        case input::PlayerAction::SetABRepeatEnd: requests_.ab_repeat_end_requested = true; should_show_osd = true; break;
        case input::PlayerAction::ClearABRepeat: requests_.ab_repeat_clear_requested = true; should_show_osd = true; break;
        case input::PlayerAction::TakeScreenshot: requests_.screenshot_requested = true; break;
        case input::PlayerAction::StepFrameBackward: requests_.step_frame_backward_requested = true; should_show_osd = true; break;
        case input::PlayerAction::StepFrameForward: requests_.step_frame_forward_requested = true; should_show_osd = true; break;
        case input::PlayerAction::SubtitleDelayDown:
        case input::PlayerAction::SubtitleDelayUp: {
            const double delta = action == input::PlayerAction::SubtitleDelayUp ? 0.1 : -0.1;
            if ((modifiers & KMOD_CTRL) != 0) { requests_.audio_delay_delta_seconds += delta; requests_.audio_delay_change_requested = true; }
            else { requests_.subtitle_delay_delta_seconds += delta; requests_.subtitle_delay_change_requested = true; }
            should_show_osd = true; break; }
        case input::PlayerAction::SeekTo10Percent: case input::PlayerAction::SeekTo20Percent: case input::PlayerAction::SeekTo30Percent:
        case input::PlayerAction::SeekTo40Percent: case input::PlayerAction::SeekTo50Percent: case input::PlayerAction::SeekTo60Percent:
        case input::PlayerAction::SeekTo70Percent: case input::PlayerAction::SeekTo80Percent: case input::PlayerAction::SeekTo90Percent:
            requests_.seek_ratio = (static_cast<int>(action) - static_cast<int>(input::PlayerAction::SeekTo10Percent) + 1) / 10.0;
            requests_.seek_requested = true; should_show_osd = true; break;
        case input::PlayerAction::PreviousChapter: requests_.previous_chapter_requested = true; should_show_osd = true; break;
        case input::PlayerAction::NextChapter: requests_.next_chapter_requested = true; should_show_osd = true; break;
        case input::PlayerAction::PreviousItem: requests_.previous_item_requested = true; should_show_osd = true; break;
        case input::PlayerAction::NextItem: requests_.next_item_requested = true; should_show_osd = true; break;
        case input::PlayerAction::ToggleSubtitle: requests_.subtitle_toggle_requested = true; should_show_osd = true; break;
        case input::PlayerAction::ToggleFullscreen: requestFullscreenToggle(); should_show_osd = true; break;
        case input::PlayerAction::Quit: should_quit_.store(true); break;
        }
    }
    if (should_show_osd) { showOsdFor(); requestRedrawIfPaused(); }
}

void OpenGLVideoRenderer::Impl::requestFullscreenToggle() { fullscreen_toggle_requested_.store(true); frame_cv_.notify_one(); }

bool OpenGLVideoRenderer::Impl::consumeFlagRequest(bool& flag) {
    std::lock_guard<std::mutex> lock(request_mutex_);
    if (!flag) return false;
    flag = false;
    return true;
}

bool OpenGLVideoRenderer::Impl::consumeDoubleRequest(bool& requested, double& value, double& out) {
    std::lock_guard<std::mutex> lock(request_mutex_);
    if (!requested) return false;
    out = value;
    value = 0.0;
    requested = false;
    return true;
}

bool OpenGLVideoRenderer::Impl::consumeFloatRequest(bool& requested, float& value, float& out) {
    std::lock_guard<std::mutex> lock(request_mutex_);
    if (!requested) return false;
    out = value;
    requested = false;
    return true;
}

bool OpenGLVideoRenderer::Impl::consumeTogglePauseRequest() { return consumeFlagRequest(requests_.toggle_pause_requested); }
bool OpenGLVideoRenderer::Impl::consumeSeekRequest(double& normalized_position) { return consumeDoubleRequest(requests_.seek_requested, requests_.seek_ratio, normalized_position); }
bool OpenGLVideoRenderer::Impl::consumeSeekDeltaRequest(double& delta_seconds) { return consumeDoubleRequest(requests_.seek_delta_requested, requests_.seek_delta_seconds, delta_seconds); }
bool OpenGLVideoRenderer::Impl::consumeVolumeChangeRequest(float& volume) { const bool ok = consumeFloatRequest(requests_.volume_change_requested, requests_.requested_volume, volume); if (ok && volume > 0.0001f) last_nonzero_volume_ = volume; return ok; }
bool OpenGLVideoRenderer::Impl::consumeSpeedChangeRequest(double& speed_delta) { return consumeDoubleRequest(requests_.speed_change_requested, requests_.speed_delta, speed_delta); }
bool OpenGLVideoRenderer::Impl::consumeResetSpeedRequest() { return consumeFlagRequest(requests_.speed_reset_requested); }
bool OpenGLVideoRenderer::Impl::consumeToggleSubtitleRequest() { return consumeFlagRequest(requests_.subtitle_toggle_requested); }
bool OpenGLVideoRenderer::Impl::consumeSetABRepeatStartRequest() { return consumeFlagRequest(requests_.ab_repeat_start_requested); }
bool OpenGLVideoRenderer::Impl::consumeSetABRepeatEndRequest() { return consumeFlagRequest(requests_.ab_repeat_end_requested); }
bool OpenGLVideoRenderer::Impl::consumeClearABRepeatRequest() { return consumeFlagRequest(requests_.ab_repeat_clear_requested); }
bool OpenGLVideoRenderer::Impl::consumeScreenshotRequest() { return consumeFlagRequest(requests_.screenshot_requested); }
bool OpenGLVideoRenderer::Impl::consumeStepFrameBackwardRequest() { return consumeFlagRequest(requests_.step_frame_backward_requested); }
bool OpenGLVideoRenderer::Impl::consumeStepFrameForwardRequest() { return consumeFlagRequest(requests_.step_frame_forward_requested); }
bool OpenGLVideoRenderer::Impl::consumeSubtitleDelayChangeRequest(double& delta_seconds) { return consumeDoubleRequest(requests_.subtitle_delay_change_requested, requests_.subtitle_delay_delta_seconds, delta_seconds); }
bool OpenGLVideoRenderer::Impl::consumeAudioDelayChangeRequest(double& delta_seconds) { return consumeDoubleRequest(requests_.audio_delay_change_requested, requests_.audio_delay_delta_seconds, delta_seconds); }
bool OpenGLVideoRenderer::Impl::consumeNextChapterRequest() { return consumeFlagRequest(requests_.next_chapter_requested); }
bool OpenGLVideoRenderer::Impl::consumePreviousChapterRequest() { return consumeFlagRequest(requests_.previous_chapter_requested); }
bool OpenGLVideoRenderer::Impl::consumeNextItemRequest() { return consumeFlagRequest(requests_.next_item_requested); }
bool OpenGLVideoRenderer::Impl::consumePreviousItemRequest() { return consumeFlagRequest(requests_.previous_item_requested); }

void OpenGLVideoRenderer::Impl::setOverlayState(double position, double duration, float volume, bool paused) {
    overlay_position_.store(std::max(0.0, position));
    overlay_duration_.store(std::max(0.0, duration));
    overlay_volume_.store(clampVolume(volume));
    overlay_paused_.store(paused);
    if (paused) requestRedraw();
}

void OpenGLVideoRenderer::Impl::setSubtitleText(const std::string& text) {
    if (text.empty()) { setSubtitleItems({}); return; }
    subtitle::SubtitleItem item{};
    item.text = text;
    item.raw_text = text;
    item.style.style_name = "plain";
    item.style.font_family = "Microsoft YaHei UI";
    item.style.font_size = 36.0;
    item.style.bold = true;
    item.style.primary_color = subtitle::SubtitleColor(255, 255, 255, 252);
    item.style.outline_color = subtitle::SubtitleColor(0, 0, 0, 255);
    item.style.background_color = subtitle::SubtitleColor(4, 4, 4, 148);
    item.style.alignment = 2;
    item.style.margin_l = 24;
    item.style.margin_r = 24;
    item.style.margin_v = 24;
    item.style.border_style = 3;
    item.style.outline = 2.0;
    item.style.shadow = 2.0;
    setSubtitleItems({item});
}

void OpenGLVideoRenderer::Impl::setSubtitleItems(const std::vector<subtitle::SubtitleItem>& items) {
    {
        std::lock_guard<std::mutex> lock(subtitle_mutex_);
        subtitle_items_ = items;
    }
    subtitle_generation_.fetch_add(1);
    requestRedrawIfPaused();
}

void OpenGLVideoRenderer::Impl::setHotkeyManager(const input::HotkeyManager& hotkey_manager) {
    std::lock_guard<std::mutex> lock(request_mutex_);
    hotkey_manager_ = hotkey_manager;
}

RendererDiagnostics OpenGLVideoRenderer::Impl::getDiagnostics() const {
    RendererDiagnostics diagnostics;
    diagnostics.display_copy_frames = frame_copy_frames_.load();
    diagnostics.display_copy_bytes = frame_copy_bytes_.load();
    diagnostics.display_copy_time_us_total = frame_copy_time_us_total_.load();
    diagnostics.display_copy_time_us_max = frame_copy_time_us_max_.load();
    return diagnostics;
}

void OpenGLVideoRenderer::Impl::resetDiagnostics() {
    frame_copy_frames_.store(0);
    frame_copy_bytes_.store(0);
    frame_copy_time_us_total_.store(0);
    frame_copy_time_us_max_.store(0);
}

bool OpenGLVideoRenderer::Impl::supportsNativeFrameFormat(AVPixelFormat format) const {
#if defined(_WIN32)
    return format == AV_PIX_FMT_D3D11 && native_d3d_device_ != nullptr && native_startup_allowed_.load() && !native_session_disabled_.load();
#else
    (void)format;
    return false;
#endif
}

bool OpenGLVideoRenderer::Impl::supportsDirectFrameFormat(AVPixelFormat format) const {
    return format == AV_PIX_FMT_YUV420P || format == AV_PIX_FMT_NV12;
}

void* OpenGLVideoRenderer::Impl::nativeDeviceHandle() const {
#if defined(_WIN32)
    return native_d3d_device_.Get();
#else
    return nullptr;
#endif
}

#if defined(_WIN32)
bool OpenGLVideoRenderer::Impl::loadWglInteropFunctions() {
    wgl_dx_open_device_nv_ = reinterpret_cast<PFNWGLDXOPENDEVICENVPROC>(SDL_GL_GetProcAddress("wglDXOpenDeviceNV"));
    wgl_dx_close_device_nv_ = reinterpret_cast<PFNWGLDXCLOSEDEVICENVPROC>(SDL_GL_GetProcAddress("wglDXCloseDeviceNV"));
    wgl_dx_register_object_nv_ = reinterpret_cast<PFNWGLDXREGISTEROBJECTNVPROC>(SDL_GL_GetProcAddress("wglDXRegisterObjectNV"));
    wgl_dx_unregister_object_nv_ = reinterpret_cast<PFNWGLDXUNREGISTEROBJECTNVPROC>(SDL_GL_GetProcAddress("wglDXUnregisterObjectNV"));
    wgl_dx_lock_objects_nv_ = reinterpret_cast<PFNWGLDXLOCKOBJECTSNVPROC>(SDL_GL_GetProcAddress("wglDXLockObjectsNV"));
    wgl_dx_unlock_objects_nv_ = reinterpret_cast<PFNWGLDXUNLOCKOBJECTSNVPROC>(SDL_GL_GetProcAddress("wglDXUnlockObjectsNV"));
    return wgl_dx_open_device_nv_ && wgl_dx_close_device_nv_ && wgl_dx_register_object_nv_ &&
           wgl_dx_unregister_object_nv_ && wgl_dx_lock_objects_nv_ && wgl_dx_unlock_objects_nv_;
}

bool OpenGLVideoRenderer::Impl::createNativeD3DDevice() {
    UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if defined(DEBUG_MODE)
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
    const D3D_FEATURE_LEVEL requested_levels[] = {D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0};
    D3D_FEATURE_LEVEL feature_level = D3D_FEATURE_LEVEL_11_0;
    HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags,
                                   requested_levels, 2u, D3D11_SDK_VERSION,
                                   native_d3d_device_.GetAddressOf(), &feature_level,
                                   native_d3d_context_.GetAddressOf());
#if defined(DEBUG_MODE)
    if (FAILED(hr)) {
        flags &= ~D3D11_CREATE_DEVICE_DEBUG;
        hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags,
                               requested_levels, 2u, D3D11_SDK_VERSION,
                               native_d3d_device_.ReleaseAndGetAddressOf(), &feature_level,
                               native_d3d_context_.ReleaseAndGetAddressOf());
    }
#endif
    if (FAILED(hr) || !native_d3d_device_ || !native_d3d_context_) {
        LOG_WARNING("OpenGL native D3D11 device creation failed: hr=" << static_cast<long>(hr));
        return false;
    }
    if (FAILED(native_d3d_device_.As(&native_d3d_device3_)) || !native_d3d_device3_) {
        LOG_WARNING("OpenGL native D3D11 interop requires ID3D11Device3 support");
        return false;
    }
    ComPtr<ID3D11Multithread> multithread;
    if (FAILED(native_d3d_context_.As(&multithread)) || !multithread) {
        LOG_WARNING("OpenGL native D3D11 interop requires ID3D11Multithread support");
        return false;
    }
    multithread->SetMultithreadProtected(TRUE);
    D3D11_SAMPLER_DESC sampler_desc{};
    sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.MaxLOD = D3D11_FLOAT32_MAX;
    return SUCCEEDED(native_d3d_device_->CreateSamplerState(&sampler_desc, native_sampler_state_.GetAddressOf()));
}

bool OpenGLVideoRenderer::Impl::createNativeD3DShaders() {
    static const char* kVideoVs = R"(
struct VSIn { float2 pos : POSITION; float2 uv : TEXCOORD0; };
struct VSOut { float4 pos : SV_POSITION; float2 uv : TEXCOORD0; };
VSOut main(VSIn input) { VSOut output; output.pos = float4(input.pos, 0.0, 1.0); output.uv = input.uv; return output; }
)";
    static const char* kNv12Ps = R"(
cbuffer ColorMatrix : register(b0) { float4 coeffR; float4 coeffG; float4 coeffB; float4 reserved; };
Texture2D texY : register(t0); Texture2D texUV : register(t1); SamplerState samp : register(s0);
struct PSIn { float4 pos : SV_POSITION; float2 uv : TEXCOORD0; };
float4 main(PSIn input) : SV_Target {
    float y = texY.Sample(samp, input.uv).r;
    float2 uv = texUV.Sample(samp, input.uv).rg;
    float3 rgb = float3(dot(float4(y, uv.x, uv.y, 1.0), coeffR), dot(float4(y, uv.x, uv.y, 1.0), coeffG), dot(float4(y, uv.x, uv.y, 1.0), coeffB));
    return float4(saturate(rgb), 1.0);
}
)";
    ComPtr<ID3DBlob> blob;
    if (FAILED(compileD3DShader(kVideoVs, "main", "vs_4_0", blob)) ||
        FAILED(native_d3d_device_->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr,
                                                      native_video_vertex_shader_.GetAddressOf()))) return false;
    const D3D11_INPUT_ELEMENT_DESC layout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    if (FAILED(native_d3d_device_->CreateInputLayout(layout, 2, blob->GetBufferPointer(), blob->GetBufferSize(), native_video_input_layout_.GetAddressOf()))) return false;
    if (FAILED(compileD3DShader(kNv12Ps, "main", "ps_4_0", blob)) ||
        FAILED(native_d3d_device_->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, native_nv12_pixel_shader_.GetAddressOf()))) return false;

    const NativeVideoVertex vertices[] = {
        {-1.0f, -1.0f, 0.0f, 1.0f}, {-1.0f,  1.0f, 0.0f, 0.0f}, { 1.0f, -1.0f, 1.0f, 1.0f},
        { 1.0f, -1.0f, 1.0f, 1.0f}, {-1.0f,  1.0f, 0.0f, 0.0f}, { 1.0f,  1.0f, 1.0f, 0.0f},
    };
    D3D11_BUFFER_DESC vertex_desc{};
    vertex_desc.ByteWidth = sizeof(vertices);
    vertex_desc.Usage = D3D11_USAGE_IMMUTABLE;
    vertex_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vertex_data{}; vertex_data.pSysMem = vertices;
    if (FAILED(native_d3d_device_->CreateBuffer(&vertex_desc, &vertex_data, native_video_vertex_buffer_.GetAddressOf()))) return false;
    D3D11_BUFFER_DESC constant_desc{};
    constant_desc.ByteWidth = sizeof(ColorMatrixConstants);
    constant_desc.Usage = D3D11_USAGE_DEFAULT;
    constant_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    return SUCCEEDED(native_d3d_device_->CreateBuffer(&constant_desc, nullptr, native_color_matrix_buffer_.GetAddressOf()));
}

bool OpenGLVideoRenderer::Impl::ensureNativeSourceViewsLocked(ID3D11Texture2D* texture, intptr_t index, DXGI_FORMAT format) {
    if (!native_d3d_device3_ || !texture) return false;
    if (texture == native_source_texture_ptr_ && index == native_source_index_ && format == native_source_format_ && native_srv_y_ && native_srv_uv_) return true;
    native_srv_y_.Reset(); native_srv_uv_.Reset();
    native_source_texture_ptr_ = texture; native_source_index_ = index; native_source_format_ = format;

    D3D11_TEXTURE2D_DESC texture_desc{}; texture->GetDesc(&texture_desc);
    const UINT first_slice = texture_desc.ArraySize > 1 ? static_cast<UINT>(std::max<intptr_t>(0, index)) : 0;
    const bool high_bit_depth = format == DXGI_FORMAT_P010 || format == DXGI_FORMAT_P016;
    const DXGI_FORMAT y_format = high_bit_depth ? DXGI_FORMAT_R16_UNORM : DXGI_FORMAT_R8_UNORM;
    const DXGI_FORMAT uv_format = high_bit_depth ? DXGI_FORMAT_R16G16_UNORM : DXGI_FORMAT_R8G8_UNORM;

    D3D11_SHADER_RESOURCE_VIEW_DESC1 y_desc{}; y_desc.Format = y_format;
    if (texture_desc.ArraySize > 1) {
        y_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
        y_desc.Texture2DArray.MostDetailedMip = 0; y_desc.Texture2DArray.MipLevels = 1;
        y_desc.Texture2DArray.FirstArraySlice = first_slice; y_desc.Texture2DArray.ArraySize = 1; y_desc.Texture2DArray.PlaneSlice = 0;
    } else {
        y_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        y_desc.Texture2D.MostDetailedMip = 0; y_desc.Texture2D.MipLevels = 1; y_desc.Texture2D.PlaneSlice = 0;
    }
    D3D11_SHADER_RESOURCE_VIEW_DESC1 uv_desc = y_desc; uv_desc.Format = uv_format;
    if (texture_desc.ArraySize > 1) uv_desc.Texture2DArray.PlaneSlice = 1;
    else uv_desc.Texture2D.PlaneSlice = 1;

    const HRESULT y_plane_hr = native_d3d_device3_->CreateShaderResourceView1(texture, &y_desc, native_srv_y_.GetAddressOf());
    if (FAILED(y_plane_hr)) { disableNativeInterop("CreateShaderResourceView1 for Y plane failed", &texture_desc, index, y_plane_hr, S_OK); return false; }
    const HRESULT uv_plane_hr = native_d3d_device3_->CreateShaderResourceView1(texture, &uv_desc, native_srv_uv_.GetAddressOf());
    if (FAILED(uv_plane_hr)) { disableNativeInterop("CreateShaderResourceView1 for UV plane failed", &texture_desc, index, y_plane_hr, uv_plane_hr); return false; }
    return true;
}

void OpenGLVideoRenderer::Impl::resetNativeInteropTarget() {
    if (interop_object_handle_ && interop_device_handle_ && wgl_dx_unregister_object_nv_) {
        if (interop_object_locked_ && wgl_dx_unlock_objects_nv_) {
            HANDLE object_handle = interop_object_handle_;
            wgl_dx_unlock_objects_nv_(interop_device_handle_, 1, &object_handle);
            interop_object_locked_ = false;
        }
        wgl_dx_unregister_object_nv_(interop_device_handle_, interop_object_handle_);
        interop_object_handle_ = nullptr;
    }
    if (native_gl_texture_ != 0) { glDeleteTextures(1, &native_gl_texture_); native_gl_texture_ = 0; }
    native_output_rtv_.Reset();
    native_output_texture_.Reset();
    native_output_width_ = native_output_height_ = 0;
}

bool OpenGLVideoRenderer::Impl::ensureNativeInteropTarget(int width, int height) {
    const int safe_width = std::max(1, width);
    const int safe_height = std::max(1, height);
    if (native_output_texture_ && native_output_rtv_ && interop_object_handle_ &&
        native_output_width_ == safe_width && native_output_height_ == safe_height && native_gl_texture_ != 0) return true;

    resetNativeInteropTarget();
    D3D11_TEXTURE2D_DESC desc{};
    desc.Width = static_cast<UINT>(safe_width); desc.Height = static_cast<UINT>(safe_height);
    desc.MipLevels = 1; desc.ArraySize = 1; desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT; desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    if (FAILED(native_d3d_device_->CreateTexture2D(&desc, nullptr, native_output_texture_.GetAddressOf())) ||
        FAILED(native_d3d_device_->CreateRenderTargetView(native_output_texture_.Get(), nullptr, native_output_rtv_.GetAddressOf()))) return false;

    glGenTextures(1, &native_gl_texture_);
    if (native_gl_texture_ == 0) return false;
    glBindTexture(GL_TEXTURE_2D, native_gl_texture_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, safe_width, safe_height, 0, GL_BGRA, GL_UNSIGNED_BYTE, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);

    interop_object_handle_ = wgl_dx_register_object_nv_(interop_device_handle_, native_output_texture_.Get(), native_gl_texture_, GL_TEXTURE_2D, WGL_ACCESS_READ_ONLY_NV);
    if (!interop_object_handle_) { resetNativeInteropTarget(); return false; }
    native_output_width_ = safe_width; native_output_height_ = safe_height;
    return true;
}

void OpenGLVideoRenderer::Impl::disableNativeInterop(const char* reason, const D3D11_TEXTURE2D_DESC* texture_desc, intptr_t texture_index, HRESULT y_plane_hr, HRESULT uv_plane_hr) {
    resetNativeInteropTarget(); native_srv_y_.Reset(); native_srv_uv_.Reset(); native_source_texture_ptr_ = nullptr;
    native_source_index_ = 0; native_source_format_ = DXGI_FORMAT_UNKNOWN;
    if (native_session_disabled_.exchange(true)) return;
    LOG_WARNING("OpenGL native D3D11 interop disabled for current session: " << reason
                << " texture_format=" << (texture_desc ? static_cast<int>(texture_desc->Format) : -1)
                << " array_size=" << (texture_desc ? texture_desc->ArraySize : 0)
                << " bind_flags=" << (texture_desc ? texture_desc->BindFlags : 0)
                << " misc_flags=" << (texture_desc ? texture_desc->MiscFlags : 0)
                << " texture_index=" << texture_index
                << " y_plane_hr=" << static_cast<long>(y_plane_hr)
                << " uv_plane_hr=" << static_cast<long>(uv_plane_hr)
                << " fallback=copyback-to-software");
}

bool OpenGLVideoRenderer::Impl::updateNativeGlTexture(const AVFrame* frame) {
    if (!frame || !native_d3d_device_ || !native_d3d_context_ || !interop_device_handle_ || native_session_disabled_.load()) return false;
    if (frame->format != AV_PIX_FMT_D3D11 || !frame->data[0]) return false;
    auto* texture = reinterpret_cast<ID3D11Texture2D*>(frame->data[0]);
    const intptr_t index = reinterpret_cast<intptr_t>(frame->data[1]);
    D3D11_TEXTURE2D_DESC desc{}; texture->GetDesc(&desc);
    const bool supported = desc.Format == DXGI_FORMAT_NV12 || desc.Format == DXGI_FORMAT_P010 || desc.Format == DXGI_FORMAT_P016;
    if (!supported) { disableNativeInterop("decoder surface format is not supported for OpenGL interop", &desc, index, S_OK, S_OK); return false; }
    if (!ensureNativeSourceViewsLocked(texture, index, desc.Format)) return false;
    if (!ensureNativeInteropTarget(frame->width, frame->height)) { disableNativeInterop("failed to allocate/register OpenGL interop target", &desc, index, S_OK, S_OK); return false; }

    if (interop_object_locked_) {
        HANDLE object_handle = interop_object_handle_;
        if (!wgl_dx_unlock_objects_nv_(interop_device_handle_, 1, &object_handle)) {
            disableNativeInterop("wglDXUnlockObjectsNV failed before D3D11 render", &desc, index, S_OK, S_OK); return false;
        }
        interop_object_locked_ = false;
    }

    D3D11_VIEWPORT viewport{}; viewport.Width = static_cast<float>(std::max(1, frame->width)); viewport.Height = static_cast<float>(std::max(1, frame->height)); viewport.MinDepth = 0.0f; viewport.MaxDepth = 1.0f;
    native_d3d_context_->RSSetViewports(1, &viewport);
    const UINT stride = sizeof(NativeVideoVertex); const UINT offset = 0;
    native_d3d_context_->IASetInputLayout(native_video_input_layout_.Get());
    native_d3d_context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    native_d3d_context_->IASetVertexBuffers(0, 1, native_video_vertex_buffer_.GetAddressOf(), &stride, &offset);
    native_d3d_context_->VSSetShader(native_video_vertex_shader_.Get(), nullptr, 0);
    native_d3d_context_->PSSetShader(native_nv12_pixel_shader_.Get(), nullptr, 0);
    native_d3d_context_->PSSetSamplers(0, 1, native_sampler_state_.GetAddressOf());
    const ColorMatrixConstants matrix = buildNativeColorMatrix(frame, desc.Format == DXGI_FORMAT_P010 || desc.Format == DXGI_FORMAT_P016);
    native_d3d_context_->UpdateSubresource(native_color_matrix_buffer_.Get(), 0, nullptr, &matrix, 0, 0);
    native_d3d_context_->PSSetConstantBuffers(0, 1, native_color_matrix_buffer_.GetAddressOf());
    ID3D11RenderTargetView* rtvs[] = {native_output_rtv_.Get()};
    native_d3d_context_->OMSetRenderTargets(1, rtvs, nullptr);
    const float clear_color[4] = {0.0f, 0.0f, 0.0f, 1.0f}; native_d3d_context_->ClearRenderTargetView(native_output_rtv_.Get(), clear_color);
    ID3D11ShaderResourceView* srvs[] = {native_srv_y_.Get(), native_srv_uv_.Get()};
    native_d3d_context_->PSSetShaderResources(0, 2, srvs);
    native_d3d_context_->Draw(6, 0);
    ID3D11ShaderResourceView* null_srvs[] = {nullptr, nullptr}; native_d3d_context_->PSSetShaderResources(0, 2, null_srvs);
    native_d3d_context_->OMSetRenderTargets(0, nullptr, nullptr);
    native_d3d_context_->Flush();

    HANDLE object_handle = interop_object_handle_;
    if (!wgl_dx_lock_objects_nv_(interop_device_handle_, 1, &object_handle)) { disableNativeInterop("wglDXLockObjectsNV failed after D3D11 render", &desc, index, S_OK, S_OK); return false; }
    interop_object_locked_ = true;
    return true;
}

bool OpenGLVideoRenderer::Impl::initializeNativeInterop() {
    native_diagnostics_ = D3D11VideoRenderer::probeSystemDiagnostics();
    LOG_INFO("[diag:opengl-native] adapter=\"" << native_diagnostics_.adapter_name << "\" driver_version=" << native_diagnostics_.driver_version
             << " native_direct_allowed=" << boolName(native_diagnostics_.native_direct_allowed)
             << " rule=" << native_diagnostics_.native_direct_disable_rule);
    if (!native_diagnostics_.native_direct_allowed) {
        native_startup_allowed_.store(false);
        LOG_WARNING("OpenGL native D3D11 interop startup disabled: rule=" << native_diagnostics_.native_direct_disable_rule
                    << " reason=\"" << native_diagnostics_.native_direct_disable_reason << "\" fallback=copyback-to-software");
        return false;
    }
    if (!loadWglInteropFunctions()) {
        native_startup_allowed_.store(false);
        LOG_WARNING("OpenGL native D3D11 interop unavailable: missing WGL_NV_DX_interop entry points, fallback=copyback-to-software");
        return false;
    }
    if (!createNativeD3DDevice() || !createNativeD3DShaders()) { destroyNativeInterop(); return false; }
    interop_device_handle_ = wgl_dx_open_device_nv_(native_d3d_device_.Get());
    if (!interop_device_handle_) {
        LOG_WARNING("OpenGL native D3D11 interop device open failed; likely adapter/driver interop mismatch, fallback=copyback-to-software");
        destroyNativeInterop();
        return false;
    }
    native_startup_allowed_.store(true); native_session_disabled_.store(false);
    LOG_INFO("OpenGL native D3D11 interop enabled for AV_PIX_FMT_D3D11 surfaces");
    return true;
}

void OpenGLVideoRenderer::Impl::destroyNativeInterop() {
    resetNativeInteropTarget();
    native_srv_y_.Reset(); native_srv_uv_.Reset(); native_source_texture_ptr_ = nullptr; native_source_index_ = 0; native_source_format_ = DXGI_FORMAT_UNKNOWN;
    native_sampler_state_.Reset(); native_color_matrix_buffer_.Reset(); native_video_vertex_buffer_.Reset(); native_video_input_layout_.Reset(); native_nv12_pixel_shader_.Reset(); native_video_vertex_shader_.Reset();
    if (interop_device_handle_ && wgl_dx_close_device_nv_) wgl_dx_close_device_nv_(interop_device_handle_);
    interop_device_handle_ = nullptr;
    wgl_dx_open_device_nv_ = nullptr; wgl_dx_close_device_nv_ = nullptr; wgl_dx_register_object_nv_ = nullptr; wgl_dx_unregister_object_nv_ = nullptr; wgl_dx_lock_objects_nv_ = nullptr; wgl_dx_unlock_objects_nv_ = nullptr;
    native_d3d_context_.Reset(); native_d3d_device3_.Reset(); native_d3d_device_.Reset();
    native_startup_allowed_.store(false); native_session_disabled_.store(false);
}
#endif

OpenGLVideoRenderer::OpenGLVideoRenderer() : impl_(std::make_unique<Impl>()) {}
OpenGLVideoRenderer::~OpenGLVideoRenderer() = default;

bool OpenGLVideoRenderer::init(const VideoRendererConfig& config) { return impl_->init(config); }
void OpenGLVideoRenderer::close() { impl_->close(); }
void OpenGLVideoRenderer::renderFrame(const core::VideoFrame& frame) { impl_->renderFrame(frame); }
void OpenGLVideoRenderer::present() { impl_->present(); }
void OpenGLVideoRenderer::clear() { impl_->clear(); }
void OpenGLVideoRenderer::handleEvents() { impl_->handleEvents(); }
bool OpenGLVideoRenderer::shouldQuit() const { return impl_->shouldQuit(); }
bool OpenGLVideoRenderer::consumeTogglePauseRequest() { return impl_->consumeTogglePauseRequest(); }
bool OpenGLVideoRenderer::consumeSeekRequest(double& normalized_position) { return impl_->consumeSeekRequest(normalized_position); }
bool OpenGLVideoRenderer::consumeSeekDeltaRequest(double& delta_seconds) { return impl_->consumeSeekDeltaRequest(delta_seconds); }
bool OpenGLVideoRenderer::consumeVolumeChangeRequest(float& volume) { return impl_->consumeVolumeChangeRequest(volume); }
bool OpenGLVideoRenderer::consumeSpeedChangeRequest(double& speed_delta) { return impl_->consumeSpeedChangeRequest(speed_delta); }
bool OpenGLVideoRenderer::consumeResetSpeedRequest() { return impl_->consumeResetSpeedRequest(); }
bool OpenGLVideoRenderer::consumeToggleSubtitleRequest() { return impl_->consumeToggleSubtitleRequest(); }
bool OpenGLVideoRenderer::consumeSetABRepeatStartRequest() { return impl_->consumeSetABRepeatStartRequest(); }
bool OpenGLVideoRenderer::consumeSetABRepeatEndRequest() { return impl_->consumeSetABRepeatEndRequest(); }
bool OpenGLVideoRenderer::consumeClearABRepeatRequest() { return impl_->consumeClearABRepeatRequest(); }
bool OpenGLVideoRenderer::consumeScreenshotRequest() { return impl_->consumeScreenshotRequest(); }
bool OpenGLVideoRenderer::consumeStepFrameBackwardRequest() { return impl_->consumeStepFrameBackwardRequest(); }
bool OpenGLVideoRenderer::consumeStepFrameForwardRequest() { return impl_->consumeStepFrameForwardRequest(); }
bool OpenGLVideoRenderer::consumeSubtitleDelayChangeRequest(double& delta_seconds) { return impl_->consumeSubtitleDelayChangeRequest(delta_seconds); }
bool OpenGLVideoRenderer::consumeAudioDelayChangeRequest(double& delta_seconds) { return impl_->consumeAudioDelayChangeRequest(delta_seconds); }
bool OpenGLVideoRenderer::consumeNextChapterRequest() { return impl_->consumeNextChapterRequest(); }
bool OpenGLVideoRenderer::consumePreviousChapterRequest() { return impl_->consumePreviousChapterRequest(); }
bool OpenGLVideoRenderer::consumeNextItemRequest() { return impl_->consumeNextItemRequest(); }
bool OpenGLVideoRenderer::consumePreviousItemRequest() { return impl_->consumePreviousItemRequest(); }
void OpenGLVideoRenderer::setOverlayState(double position, double duration, float volume, bool paused) { impl_->setOverlayState(position, duration, volume, paused); }
void OpenGLVideoRenderer::setSubtitleText(const std::string& text) { impl_->setSubtitleText(text); }
void OpenGLVideoRenderer::setSubtitleItems(const std::vector<subtitle::SubtitleItem>& items) { impl_->setSubtitleItems(items); }
void OpenGLVideoRenderer::setHotkeyManager(const input::HotkeyManager& hotkey_manager) { impl_->setHotkeyManager(hotkey_manager); }
RendererDiagnostics OpenGLVideoRenderer::getDiagnostics() const { return impl_->getDiagnostics(); }
void OpenGLVideoRenderer::resetDiagnostics() { impl_->resetDiagnostics(); }
bool OpenGLVideoRenderer::supportsNativeFrameFormat(AVPixelFormat format) const { return impl_->supportsNativeFrameFormat(format); }
bool OpenGLVideoRenderer::supportsDirectFrameFormat(AVPixelFormat format) const { return impl_->supportsDirectFrameFormat(format); }
void* OpenGLVideoRenderer::nativeDeviceHandle() const { return impl_->nativeDeviceHandle(); }
const char* OpenGLVideoRenderer::rendererBackendName() const { return "OpenGL"; }

}  // namespace vp::render






