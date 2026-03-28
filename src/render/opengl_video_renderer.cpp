#include "render/opengl_video_renderer.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <limits>
#include <mutex>
#include <optional>
#include <sstream>
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
#include <SDL2/SDL_syswm.h>
#include <SDL2/SDL_opengl.h>
#elif __has_include(<SDL.h>)
#include <SDL.h>
#include <SDL_syswm.h>
#include <SDL_opengl.h>
#else
#error "SDL2 headers not found"
#endif

#if !defined(_WIN32)
#ifdef None
#undef None
#endif
#ifdef Complex
#undef Complex
#endif
#endif

#if defined(_WIN32)
#include <windows.h>
#include <d3d11.h>
#include <d3d11_1.h>
#include <d3d11_3.h>
#include <d3d11_4.h>
#include <dxgi1_6.h>
#include <d2d1.h>
#include <d2d1helper.h>
#include <d3dcompiler.h>
#include <dwrite.h>
#include <dwrite_1.h>
#include <wrl/client.h>
#endif

extern "C" {
#include <libavutil/frame.h>
#include <libavutil/pixdesc.h>
}

#include "logger.h"
#include "render/output_color_profile.h"
#include "render/d3d11_video_renderer.h"
#include "subtitle/subtitle_font_registry.h"

namespace vp::render {

namespace {

constexpr int kMinWindowWidth = 320;
constexpr int kMinWindowHeight = 180;
constexpr float kVolumeStep = 0.05f;
constexpr double kSeekStepSeconds = 5.0;
constexpr double kSeekStepSecondsCtrl = 30.0;
constexpr double kSpeedStep = 0.1;
constexpr int kControlPanelInset = 12;
constexpr int kControlPanelHeight = 72;
constexpr int kControlPadding = 12;
constexpr int kControlGap = 12;
constexpr int kBarHeight = 8;
constexpr int kVolumeBarWidth = 96;
constexpr int kMinProgressBarWidth = 96;
constexpr uint64_t kOsdHoldMs = 2200;
constexpr uint64_t kOsdFadeOutMs = 220;
constexpr auto kOpenGLPresentWaitTimeout = std::chrono::milliseconds(250);
constexpr float kMinSubtitleFontSize = 20.0f;
constexpr float kMaxSubtitleFontSize = 72.0f;
constexpr float kSubtitleFontScale = 0.055f;
constexpr float kSubtitleHorizontalPadding = 18.0f;
constexpr float kSubtitleVerticalPadding = 10.0f;

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

bool pointInRect(int x, int y, const SDL_Rect& rect) {
    return x >= rect.x && y >= rect.y && x < rect.x + rect.w && y < rect.y + rect.h;
}

std::string twoDigitString(int value) {
    if (value < 10) {
        return std::string("0") + std::to_string(value);
    }
    return std::to_string(value);
}

std::string formatClockText(double seconds, bool force_hours, bool unknown = false) {
    if (unknown || !std::isfinite(seconds)) {
        return force_hours ? "--:--:--" : "--:--";
    }
    const int total_seconds = std::max(0, static_cast<int>(std::floor(seconds)));
    const int hours = total_seconds / 3600;
    const int minutes = (total_seconds / 60) % 60;
    const int secs = total_seconds % 60;
    if (force_hours || hours > 0) {
        return std::to_string(hours) + ":" + twoDigitString(minutes) + ":" + twoDigitString(secs);
    }
    return twoDigitString(total_seconds / 60) + ":" + twoDigitString(secs);
}

std::string formatOverlayTimeText(double position_seconds, double duration_seconds) {
    const bool duration_known = duration_seconds > 0.001 && std::isfinite(duration_seconds);
    const bool force_hours = position_seconds >= 3600.0 || (duration_known && duration_seconds >= 3600.0);
    return formatClockText(position_seconds, force_hours) + " / " +
           formatClockText(duration_seconds, force_hours, !duration_known);
}

constexpr uint8_t kSegmentTop = 1u << 0;
constexpr uint8_t kSegmentUpperLeft = 1u << 1;
constexpr uint8_t kSegmentUpperRight = 1u << 2;
constexpr uint8_t kSegmentMiddle = 1u << 3;
constexpr uint8_t kSegmentLowerLeft = 1u << 4;
constexpr uint8_t kSegmentLowerRight = 1u << 5;
constexpr uint8_t kSegmentBottom = 1u << 6;

uint8_t sevenSegmentMask(char ch) {
    switch (ch) {
    case '0':
        return kSegmentTop | kSegmentUpperLeft | kSegmentUpperRight | kSegmentLowerLeft | kSegmentLowerRight |
               kSegmentBottom;
    case '1':
        return kSegmentUpperRight | kSegmentLowerRight;
    case '2':
        return kSegmentTop | kSegmentUpperRight | kSegmentMiddle | kSegmentLowerLeft | kSegmentBottom;
    case '3':
        return kSegmentTop | kSegmentUpperRight | kSegmentMiddle | kSegmentLowerRight | kSegmentBottom;
    case '4':
        return kSegmentUpperLeft | kSegmentUpperRight | kSegmentMiddle | kSegmentLowerRight;
    case '5':
        return kSegmentTop | kSegmentUpperLeft | kSegmentMiddle | kSegmentLowerRight | kSegmentBottom;
    case '6':
        return kSegmentTop | kSegmentUpperLeft | kSegmentMiddle | kSegmentLowerLeft | kSegmentLowerRight |
               kSegmentBottom;
    case '7':
        return kSegmentTop | kSegmentUpperRight | kSegmentLowerRight;
    case '8':
        return kSegmentTop | kSegmentUpperLeft | kSegmentUpperRight | kSegmentMiddle | kSegmentLowerLeft |
               kSegmentLowerRight | kSegmentBottom;
    case '9':
        return kSegmentTop | kSegmentUpperLeft | kSegmentUpperRight | kSegmentMiddle | kSegmentLowerRight |
               kSegmentBottom;
    case '-':
        return kSegmentMiddle;
    default:
        return 0;
    }
}

int overlayGlyphWidth(char ch, int glyph_height, int stroke) {
    if (ch == ' ') {
        return std::max(4, glyph_height / 3);
    }
    if (ch == ':') {
        return std::max(stroke * 2, glyph_height / 4);
    }
    if (ch == '/') {
        return std::max(stroke * 2, glyph_height / 2);
    }
    return std::max(8, glyph_height * 11 / 18);
}

int overlayGlyphAdvance(char ch, int glyph_height, int stroke) {
    return overlayGlyphWidth(ch, glyph_height, stroke) + std::max(2, glyph_height / 8);
}

int measureOverlayTextWidth(const std::string& text, int glyph_height) {
    const int stroke = std::max(2, glyph_height / 7);
    int width = 0;
    for (char ch : text) {
        width += overlayGlyphAdvance(ch, glyph_height, stroke);
    }
    return std::max(0, width - std::max(2, glyph_height / 8));
}

int chromaExtent(int value) {
    return std::max(1, (value + 1) / 2);
}

bool isLimitedRange(AVColorRange range) {
    return range != AVCOL_RANGE_JPEG;
}

enum class ColorMatrixKind {
    Bt601,
    Bt709,
    Bt2020,
};

enum class ColorTransferMode {
    Sdr = 0,
    Pq = 1,
    Hlg = 2,
};

enum class ColorGamutMode {
    Disabled = 0,
    Bt2020ToBt709 = 1,
};

enum class ColorSamplePrecision {
    Byte8,
    Word16Normalized,
};

ColorMatrixKind colorMatrixKind(AVColorSpace space) {
    switch (space) {
    case AVCOL_SPC_BT470BG:
    case AVCOL_SPC_SMPTE170M:
    case AVCOL_SPC_FCC:
        return ColorMatrixKind::Bt601;
    case AVCOL_SPC_BT2020_CL:
    case AVCOL_SPC_BT2020_NCL:
        return ColorMatrixKind::Bt2020;
    default:
        return ColorMatrixKind::Bt709;
    }
}

struct ColorCoefficients {
    std::array<float, 4> r{};
    std::array<float, 4> g{};
    std::array<float, 4> b{};
};

struct ColorPipelineConfig {
    ColorCoefficients coeffs{};
    float transfer_mode{static_cast<float>(ColorTransferMode::Sdr)};
    float gamut_mode{static_cast<float>(ColorGamutMode::Disabled)};
    bool hdr_tone_mapping{false};
    bool gamut_mapping{false};
    ColorMatrixKind matrix_kind{ColorMatrixKind::Bt709};
};

const char* colorMatrixName(ColorMatrixKind kind) {
    switch (kind) {
    case ColorMatrixKind::Bt601:
        return "bt601";
    case ColorMatrixKind::Bt2020:
        return "bt2020";
    default:
        return "bt709";
    }
}

const char* colorRangeName(AVColorRange range) {
    switch (range) {
    case AVCOL_RANGE_JPEG:
        return "full";
    case AVCOL_RANGE_MPEG:
        return "limited";
    default:
        return "unspecified";
    }
}

const char* colorSpaceName(AVColorSpace space) {
    switch (space) {
    case AVCOL_SPC_BT470BG:
        return "bt470bg";
    case AVCOL_SPC_SMPTE170M:
        return "smpte170m";
    case AVCOL_SPC_FCC:
        return "fcc";
    case AVCOL_SPC_BT709:
        return "bt709";
    case AVCOL_SPC_BT2020_CL:
        return "bt2020_cl";
    case AVCOL_SPC_BT2020_NCL:
        return "bt2020_ncl";
    default:
        return "unspecified";
    }
}

const char* transferName(AVColorTransferCharacteristic transfer) {
    switch (transfer) {
    case AVCOL_TRC_BT709:
        return "bt709";
    case AVCOL_TRC_SMPTE2084:
        return "smpte2084";
    case AVCOL_TRC_ARIB_STD_B67:
        return "arib-std-b67";
    case AVCOL_TRC_BT2020_10:
        return "bt2020-10";
    case AVCOL_TRC_BT2020_12:
        return "bt2020-12";
    case AVCOL_TRC_LINEAR:
        return "linear";
    default:
        return "unspecified";
    }
}

const char* primariesName(AVColorPrimaries primaries) {
    switch (primaries) {
    case AVCOL_PRI_BT709:
        return "bt709";
    case AVCOL_PRI_BT2020:
        return "bt2020";
    case AVCOL_PRI_SMPTE170M:
        return "smpte170m";
    case AVCOL_PRI_BT470BG:
        return "bt470bg";
    default:
        return "unspecified";
    }
}

bool isPqTransfer(AVColorTransferCharacteristic transfer) {
    return transfer == AVCOL_TRC_SMPTE2084;
}

bool isHlgTransfer(AVColorTransferCharacteristic transfer) {
    return transfer == AVCOL_TRC_ARIB_STD_B67;
}

bool isHdrTransfer(AVColorTransferCharacteristic transfer) {
    return isPqTransfer(transfer) || isHlgTransfer(transfer);
}

bool useBt2020GamutMapping(AVColorSpace space, AVColorPrimaries primaries) {
    return space == AVCOL_SPC_BT2020_CL ||
           space == AVCOL_SPC_BT2020_NCL ||
           primaries == AVCOL_PRI_BT2020;
}

bool isHighBitDepthSemiPlanarFormat(AVPixelFormat format) {
    return format == AV_PIX_FMT_P010LE || format == AV_PIX_FMT_P016LE;
}

bool isSoftwareTwoPlaneFormat(AVPixelFormat format) {
    return format == AV_PIX_FMT_NV12 || isHighBitDepthSemiPlanarFormat(format);
}

ColorSamplePrecision colorSamplePrecisionForFormat(AVPixelFormat format) {
    return isHighBitDepthSemiPlanarFormat(format) ? ColorSamplePrecision::Word16Normalized
                                                  : ColorSamplePrecision::Byte8;
}

ColorCoefficients buildColorCoefficients(AVColorRange range, AVColorSpace space, ColorSamplePrecision precision) {
    const bool limited = isLimitedRange(range);
    const ColorMatrixKind matrix_kind = colorMatrixKind(space);
    float y_bias = 0.0f;
    float uv_bias = -0.5f;
    float y_scale = 1.0f;
    if (precision == ColorSamplePrecision::Word16Normalized) {
        if (limited) {
            y_bias = -(4096.0f / 65535.0f);
            y_scale = 65535.0f / 56064.0f;
            uv_bias = -(32768.0f / 65535.0f);
        } else {
            uv_bias = -(32768.0f / 65535.0f);
        }
    } else {
        y_bias = limited ? (-16.0f / 255.0f) : 0.0f;
        uv_bias = limited ? (-128.0f / 255.0f) : -0.5f;
        y_scale = limited ? (255.0f / 219.0f) : 1.0f;
    }
    float rv = 1.792741f;
    float gu = -0.213249f;
    float gv = -0.532909f;
    float bu = 2.112402f;
    switch (matrix_kind) {
    case ColorMatrixKind::Bt601:
        rv = 1.596027f;
        gu = -0.391762f;
        gv = -0.812968f;
        bu = 2.017232f;
        break;
    case ColorMatrixKind::Bt2020:
        rv = 1.474600f;
        gu = -0.164553f;
        gv = -0.571353f;
        bu = 1.881400f;
        break;
    case ColorMatrixKind::Bt709:
    default:
        break;
    }

    ColorCoefficients coeffs;
    coeffs.r = {y_scale, 0.0f, rv, y_scale * y_bias + rv * uv_bias};
    coeffs.g = {y_scale, gu, gv, y_scale * y_bias + (gu + gv) * uv_bias};
    coeffs.b = {y_scale, bu, 0.0f, y_scale * y_bias + bu * uv_bias};
    return coeffs;
}

ColorPipelineConfig buildColorPipelineConfig(AVColorRange range,
                                             AVColorSpace space,
                                             AVColorTransferCharacteristic transfer,
                                             AVColorPrimaries primaries,
                                             ColorSamplePrecision precision = ColorSamplePrecision::Byte8) {
    ColorPipelineConfig config;
    config.coeffs = buildColorCoefficients(range, space, precision);
    config.matrix_kind = colorMatrixKind(space);
    if (isPqTransfer(transfer)) {
        config.transfer_mode = static_cast<float>(ColorTransferMode::Pq);
        config.hdr_tone_mapping = true;
    } else if (isHlgTransfer(transfer)) {
        config.transfer_mode = static_cast<float>(ColorTransferMode::Hlg);
        config.hdr_tone_mapping = true;
    }
    if (useBt2020GamutMapping(space, primaries)) {
        config.gamut_mode = static_cast<float>(ColorGamutMode::Bt2020ToBt709);
        config.gamut_mapping = true;
    }
    return config;
}

std::optional<std::string> readEnvVar(const char* key) {
    if (!key || key[0] == '\0') {
        return std::nullopt;
    }
#if defined(_WIN32)
    char* value = nullptr;
    size_t length = 0;
    if (_dupenv_s(&value, &length, key) != 0 || !value) {
        return std::nullopt;
    }
    std::string result(value);
    std::free(value);
    return result;
#else
    if (const char* value = std::getenv(key)) {
        return std::string(value);
    }
    return std::nullopt;
#endif
}

std::string toLowerAscii(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

bool stringStartsWith(const std::string& text, const char* prefix) {
    if (!prefix || prefix[0] == '\0') {
        return true;
    }
    return text.rfind(prefix, 0) == 0;
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
    PFNGLTEXIMAGE3DPROC TexImage3D{};
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
    const bool mandatory_loaded = loadGlProc(g_gl.CreateShader, "glCreateShader") &&
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
    if (!mandatory_loaded) {
        return false;
    }
    g_gl.TexImage3D = reinterpret_cast<PFNGLTEXIMAGE3DPROC>(SDL_GL_GetProcAddress("glTexImage3D"));
    return true;
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
uniform sampler3D texLut3D;
uniform vec4 coeffR;
uniform vec4 coeffG;
uniform vec4 coeffB;
uniform vec4 colorConfig0;
varying vec2 v_texcoord;

vec3 decodeGamma22(vec3 value) {
    return pow(max(value, vec3(0.0)), vec3(2.2));
}

vec3 encodeGamma22(vec3 value) {
    return pow(max(value, vec3(0.0)), vec3(1.0 / 2.2));
}

vec3 decodePq(vec3 value) {
    const float m1 = 0.1593017578125;
    const float m2 = 78.84375;
    const float c1 = 0.8359375;
    const float c2 = 18.8515625;
    const float c3 = 18.6875;
    vec3 vp = pow(max(value, vec3(0.0)), vec3(1.0 / m2));
    vec3 num = max(vp - vec3(c1), vec3(0.0));
    vec3 den = max(vec3(c2) - vec3(c3) * vp, vec3(1e-6));
    return pow(num / den, vec3(1.0 / m1));
}

vec3 decodeHlg(vec3 value) {
    vec3 lower = (value * value) / 3.0;
    vec3 upper = (exp((value - 0.55991073) / 0.17883277) + 0.28466892) / 12.0;
    bvec3 cutoff = lessThanEqual(value, vec3(0.5));
    return vec3(cutoff.x ? lower.x : upper.x,
                cutoff.y ? lower.y : upper.y,
                cutoff.z ? lower.z : upper.z);
}

vec3 bt2020ToBt709(vec3 value) {
    return mat3(1.6605, -0.1246, -0.0182,
                -0.5876, 1.1329, -0.1006,
                -0.0728, -0.0083, 1.1187) * value;
}

vec3 applyLut(vec3 value) {
    int lut_enabled = int(colorConfig0.w + 0.5);
    vec3 clamped_value = clamp(value, 0.0, 1.0);
    if (lut_enabled == 0) {
        return clamped_value;
    }
    return clamp(texture3D(texLut3D, clamped_value).rgb, 0.0, 1.0);
}

vec3 postProcessRgb(vec3 encoded_rgb) {
    int transfer_mode = int(colorConfig0.x + 0.5);
    int gamut_mode = int(colorConfig0.y + 0.5);
    int hdr_output_mode = int(colorConfig0.z + 0.5);
    vec3 working = max(encoded_rgb, vec3(0.0));
    bool linearized = false;
    if (transfer_mode == 1) {
        working = decodePq(working) * (10000.0 / 203.0);
        linearized = true;
    } else if (transfer_mode == 2) {
        working = decodeHlg(working) * (1000.0 / 203.0);
        linearized = true;
    }
    if (transfer_mode != 0 && hdr_output_mode == 1) {
        return applyLut(clamp(max(encoded_rgb, vec3(0.0)), 0.0, 1.0));
    }
    if (gamut_mode == 1) {
        if (!linearized) {
            working = decodeGamma22(working);
            linearized = true;
        }
        working = max(bt2020ToBt709(working), vec3(0.0));
    }
    if (transfer_mode != 0) {
        if (!linearized) {
            working = decodeGamma22(working);
            linearized = true;
        }
        working = working / (vec3(1.0) + working);
    }
    if (linearized) {
        return applyLut(clamp(encodeGamma22(working), 0.0, 1.0));
    }
    return applyLut(clamp(working, 0.0, 1.0));
}

void main() {
    float y = texture2D(texY, v_texcoord).r;
    float u = texture2D(texU, v_texcoord).r;
    float v = texture2D(texV, v_texcoord).r;
    vec4 yuv = vec4(y, u, v, 1.0);
    vec3 rgb = vec3(dot(yuv, coeffR), dot(yuv, coeffG), dot(yuv, coeffB));
    gl_FragColor = vec4(postProcessRgb(rgb), 1.0);
}
)";

constexpr const char* kNv12FragmentShaderSource = R"(#version 120
uniform sampler2D texY;
uniform sampler2D texUV;
uniform sampler3D texLut3D;
uniform vec4 coeffR;
uniform vec4 coeffG;
uniform vec4 coeffB;
uniform vec4 colorConfig0;
varying vec2 v_texcoord;

vec3 decodeGamma22(vec3 value) {
    return pow(max(value, vec3(0.0)), vec3(2.2));
}

vec3 encodeGamma22(vec3 value) {
    return pow(max(value, vec3(0.0)), vec3(1.0 / 2.2));
}

vec3 decodePq(vec3 value) {
    const float m1 = 0.1593017578125;
    const float m2 = 78.84375;
    const float c1 = 0.8359375;
    const float c2 = 18.8515625;
    const float c3 = 18.6875;
    vec3 vp = pow(max(value, vec3(0.0)), vec3(1.0 / m2));
    vec3 num = max(vp - vec3(c1), vec3(0.0));
    vec3 den = max(vec3(c2) - vec3(c3) * vp, vec3(1e-6));
    return pow(num / den, vec3(1.0 / m1));
}

vec3 decodeHlg(vec3 value) {
    vec3 lower = (value * value) / 3.0;
    vec3 upper = (exp((value - 0.55991073) / 0.17883277) + 0.28466892) / 12.0;
    bvec3 cutoff = lessThanEqual(value, vec3(0.5));
    return vec3(cutoff.x ? lower.x : upper.x,
                cutoff.y ? lower.y : upper.y,
                cutoff.z ? lower.z : upper.z);
}

vec3 bt2020ToBt709(vec3 value) {
    return mat3(1.6605, -0.1246, -0.0182,
                -0.5876, 1.1329, -0.1006,
                -0.0728, -0.0083, 1.1187) * value;
}

vec3 applyLut(vec3 value) {
    int lut_enabled = int(colorConfig0.w + 0.5);
    vec3 clamped_value = clamp(value, 0.0, 1.0);
    if (lut_enabled == 0) {
        return clamped_value;
    }
    return clamp(texture3D(texLut3D, clamped_value).rgb, 0.0, 1.0);
}

vec3 postProcessRgb(vec3 encoded_rgb) {
    int transfer_mode = int(colorConfig0.x + 0.5);
    int gamut_mode = int(colorConfig0.y + 0.5);
    int hdr_output_mode = int(colorConfig0.z + 0.5);
    vec3 working = max(encoded_rgb, vec3(0.0));
    bool linearized = false;
    if (transfer_mode == 1) {
        working = decodePq(working) * (10000.0 / 203.0);
        linearized = true;
    } else if (transfer_mode == 2) {
        working = decodeHlg(working) * (1000.0 / 203.0);
        linearized = true;
    }
    if (transfer_mode != 0 && hdr_output_mode == 1) {
        return applyLut(clamp(max(encoded_rgb, vec3(0.0)), 0.0, 1.0));
    }
    if (gamut_mode == 1) {
        if (!linearized) {
            working = decodeGamma22(working);
            linearized = true;
        }
        working = max(bt2020ToBt709(working), vec3(0.0));
    }
    if (transfer_mode != 0) {
        if (!linearized) {
            working = decodeGamma22(working);
            linearized = true;
        }
        working = working / (vec3(1.0) + working);
    }
    if (linearized) {
        return applyLut(clamp(encodeGamma22(working), 0.0, 1.0));
    }
    return applyLut(clamp(working, 0.0, 1.0));
}

void main() {
    float y = texture2D(texY, v_texcoord).r;
    vec4 uv_sample = texture2D(texUV, v_texcoord);
    vec4 yuv = vec4(y, uv_sample.r, uv_sample.a, 1.0);
    vec3 rgb = vec3(dot(yuv, coeffR), dot(yuv, coeffG), dot(yuv, coeffB));
    gl_FragColor = vec4(postProcessRgb(rgb), 1.0);
}
)";

constexpr const char* kRgbFragmentShaderSource = R"(#version 120
uniform sampler2D texRgb;
uniform sampler3D texLut3D;
uniform vec4 colorConfig0;
varying vec2 v_texcoord;

vec3 applyLut(vec3 value) {
    int lut_enabled = int(colorConfig0.w + 0.5);
    vec3 clamped_value = clamp(value, 0.0, 1.0);
    if (lut_enabled == 0) {
        return clamped_value;
    }
    return clamp(texture3D(texLut3D, clamped_value).rgb, 0.0, 1.0);
}

void main() {
    vec3 rgb = texture2D(texRgb, v_texcoord).rgb;
    gl_FragColor = vec4(applyLut(rgb), 1.0);
}
)";

const char* boolName(bool value) { return value ? "true" : "false"; }

#if defined(_WIN32)
using Microsoft::WRL::ComPtr;
constexpr size_t kSubtitleBitmapCacheLimit = 32;

struct ColorMatrixConstants {
    float coeff_r[4];
    float coeff_g[4];
    float coeff_b[4];
    float color_config0[4];
};

uint64_t hashSubtitleBitmap(const subtitle::SubtitleBitmap& bitmap) {
    constexpr uint64_t kFnvOffset = 1469598103934665603ull;
    constexpr uint64_t kFnvPrime = 1099511628211ull;

    auto hash_bytes = [kFnvPrime](uint64_t seed, const uint8_t* data, size_t size) {
        uint64_t hash = seed;
        for (size_t i = 0; i < size; ++i) {
            hash ^= static_cast<uint64_t>(data[i]);
            hash *= kFnvPrime;
        }
        return hash;
    };

    uint64_t hash = kFnvOffset;
    const std::array<int, 2> header = {bitmap.width, bitmap.height};
    hash = hash_bytes(hash,
                      reinterpret_cast<const uint8_t*>(header.data()),
                      header.size() * sizeof(header[0]));
    return hash_bytes(hash, bitmap.rgba.data(), bitmap.rgba.size());
}

std::vector<uint8_t> premultiplySubtitleBitmapBgra(const subtitle::SubtitleBitmap& bitmap) {
    const size_t pixel_count = static_cast<size_t>(bitmap.width) * static_cast<size_t>(bitmap.height);
    std::vector<uint8_t> premul_bgra(pixel_count * 4u, 0u);
    for (size_t i = 0; i < pixel_count; ++i) {
        const uint8_t r = bitmap.rgba[i * 4u + 0u];
        const uint8_t g = bitmap.rgba[i * 4u + 1u];
        const uint8_t b = bitmap.rgba[i * 4u + 2u];
        const uint8_t a = bitmap.rgba[i * 4u + 3u];
        const uint16_t alpha = static_cast<uint16_t>(a);
        premul_bgra[i * 4u + 0u] = static_cast<uint8_t>((static_cast<uint16_t>(b) * alpha + 127u) / 255u);
        premul_bgra[i * 4u + 1u] = static_cast<uint8_t>((static_cast<uint16_t>(g) * alpha + 127u) / 255u);
        premul_bgra[i * 4u + 2u] = static_cast<uint8_t>((static_cast<uint16_t>(r) * alpha + 127u) / 255u);
        premul_bgra[i * 4u + 3u] = a;
    }
    return premul_bgra;
}

ColorMatrixConstants buildNativeColorMatrix(const AVFrame* frame, bool high_bit_depth, bool hdr_output_bridge_active) {
    const ColorPipelineConfig pipeline = buildColorPipelineConfig(frame ? frame->color_range : AVCOL_RANGE_UNSPECIFIED,
                                                                  frame ? frame->colorspace : AVCOL_SPC_UNSPECIFIED,
                                                                  frame ? frame->color_trc : AVCOL_TRC_UNSPECIFIED,
                                                                  frame ? frame->color_primaries : AVCOL_PRI_UNSPECIFIED,
                                                                  high_bit_depth ? ColorSamplePrecision::Word16Normalized
                                                                                 : ColorSamplePrecision::Byte8);

    ColorMatrixConstants constants{};
    std::copy(pipeline.coeffs.r.begin(), pipeline.coeffs.r.end(), constants.coeff_r);
    std::copy(pipeline.coeffs.g.begin(), pipeline.coeffs.g.end(), constants.coeff_g);
    std::copy(pipeline.coeffs.b.begin(), pipeline.coeffs.b.end(), constants.coeff_b);
    constants.color_config0[0] = pipeline.transfer_mode;
    constants.color_config0[1] = pipeline.gamut_mode;
    constants.color_config0[2] = hdr_output_bridge_active ? 1.0f : 0.0f;
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

DWRITE_TEXT_ALIGNMENT toTextAlignment(int alignment) {
    switch (subtitleHorizontalGroup(alignment)) {
    case 0:
        return DWRITE_TEXT_ALIGNMENT_LEADING;
    case 2:
        return DWRITE_TEXT_ALIGNMENT_TRAILING;
    default:
        return DWRITE_TEXT_ALIGNMENT_CENTER;
    }
}

D2D1_COLOR_F toD2DColor(const subtitle::SubtitleColor& color) {
    return D2D1::ColorF(static_cast<float>(color.r) / 255.0f,
                        static_cast<float>(color.g) / 255.0f,
                        static_cast<float>(color.b) / 255.0f,
                        static_cast<float>(color.a) / 255.0f);
}

subtitle::SubtitleColor applySubtitleOpacity(const subtitle::SubtitleColor& color, float opacity) {
    subtitle::SubtitleColor adjusted = color;
    const int scaled_alpha = static_cast<int>(std::lround(static_cast<double>(color.a) *
                                                          std::clamp(opacity, 0.0f, 1.0f)));
    adjusted.a = static_cast<uint8_t>(std::clamp(scaled_alpha, 0, 255));
    return adjusted;
}

bool subtitleAnimationHasMotion(const subtitle::SubtitleStyleAnimation& animation) {
    if (!animation.has_move) {
        return false;
    }
    if (std::abs(animation.move_x2 - animation.move_x1) <= 0.001 &&
        std::abs(animation.move_y2 - animation.move_y1) <= 0.001) {
        return false;
    }
    return !animation.move_has_timing || animation.move_t2_seconds > animation.move_t1_seconds;
}

bool subtitleAnimationHasFade(const subtitle::SubtitleStyleAnimation& animation) {
    switch (animation.fade_mode) {
    case subtitle::SubtitleFadeMode::Simple:
        return animation.fade_in_seconds > 0.0005 || animation.fade_out_seconds > 0.0005;
    case subtitle::SubtitleFadeMode::Complex:
        return animation.fade_alpha1 != animation.fade_alpha2 ||
               animation.fade_alpha2 != animation.fade_alpha3 ||
               animation.fade_t2_seconds > animation.fade_t1_seconds ||
               animation.fade_t4_seconds > animation.fade_t3_seconds;
    case subtitle::SubtitleFadeMode::None:
    default:
        return false;
    }
}

bool subtitleAnimationHasStyleTransitions(const subtitle::SubtitleStyleAnimation& animation) {
    return std::any_of(animation.style_transitions.begin(),
                       animation.style_transitions.end(),
                       [](const subtitle::SubtitleStyleTransition& transition) {
                           return transition.property_mask != 0;
                       });
}

struct SubtitleAnimationState {
    bool has_position{false};
    double position_x{0.0};
    double position_y{0.0};
    float opacity{1.0f};
};

float evaluateComplexFadeOpacity(const subtitle::SubtitleStyleAnimation& animation, double item_elapsed_seconds) {
    const double a1 = static_cast<double>(animation.fade_alpha1);
    const double a2 = static_cast<double>(animation.fade_alpha2);
    const double a3 = static_cast<double>(animation.fade_alpha3);
    double value = a3;
    if (item_elapsed_seconds <= animation.fade_t1_seconds) {
        value = a1;
    } else if (item_elapsed_seconds < animation.fade_t2_seconds && animation.fade_t2_seconds > animation.fade_t1_seconds) {
        const double progress = (item_elapsed_seconds - animation.fade_t1_seconds) /
                                (animation.fade_t2_seconds - animation.fade_t1_seconds);
        value = a1 + (a2 - a1) * std::clamp(progress, 0.0, 1.0);
    } else if (item_elapsed_seconds <= animation.fade_t3_seconds) {
        value = a2;
    } else if (item_elapsed_seconds < animation.fade_t4_seconds && animation.fade_t4_seconds > animation.fade_t3_seconds) {
        const double progress = (item_elapsed_seconds - animation.fade_t3_seconds) /
                                (animation.fade_t4_seconds - animation.fade_t3_seconds);
        value = a2 + (a3 - a2) * std::clamp(progress, 0.0, 1.0);
    }
    return static_cast<float>(std::clamp(value / 255.0, 0.0, 1.0));
}

float evaluateStyleTransitionProgress(const subtitle::SubtitleStyleTransition& transition,
                                      double item_elapsed_seconds,
                                      double item_duration_seconds) {
    const double start_seconds = transition.has_timing ? transition.start_seconds : 0.0;
    const double end_seconds = transition.has_timing ? transition.end_seconds : item_duration_seconds;
    if (item_elapsed_seconds <= start_seconds) {
        return 0.0f;
    }
    if (end_seconds <= start_seconds + 0.0005) {
        return item_elapsed_seconds >= end_seconds ? 1.0f : 0.0f;
    }
    const double linear = std::clamp((item_elapsed_seconds - start_seconds) / (end_seconds - start_seconds), 0.0, 1.0);
    return static_cast<float>(std::pow(linear, std::max(0.01, transition.accel)));
}

uint8_t lerpByte(uint8_t from, uint8_t to, float progress) {
    const float blended = static_cast<float>(from) + (static_cast<float>(to) - static_cast<float>(from)) * progress;
    long value = std::lround(blended);
    value = std::max<long>(0, std::min<long>(255, value));
    return static_cast<uint8_t>(value);
}

subtitle::SubtitleColor lerpSubtitleColor(const subtitle::SubtitleColor& from,
                                          const subtitle::SubtitleColor& to,
                                          float progress) {
    return subtitle::SubtitleColor(
        lerpByte(from.r, to.r, progress),
        lerpByte(from.g, to.g, progress),
        lerpByte(from.b, to.b, progress),
        lerpByte(from.a, to.a, progress));
}

double lerpDouble(double from, double to, float progress) {
    return from + (to - from) * static_cast<double>(progress);
}

void applyStyleTransition(subtitle::SubtitleStyle& style,
                          const subtitle::SubtitleStyleTransition& transition,
                          float progress) {
    if (progress <= 0.0001f || transition.property_mask == 0) {
        return;
    }

    const subtitle::SubtitleStyle& target = transition.target_style;
    if ((transition.property_mask & subtitle::kSubtitleTransitionFontSize) != 0) {
        style.font_size = lerpDouble(style.font_size, target.font_size, progress);
    }
    if ((transition.property_mask & subtitle::kSubtitleTransitionPrimaryColor) != 0) {
        style.primary_color = lerpSubtitleColor(style.primary_color, target.primary_color, progress);
    }
    if ((transition.property_mask & subtitle::kSubtitleTransitionSecondaryColor) != 0) {
        style.secondary_color = lerpSubtitleColor(style.secondary_color, target.secondary_color, progress);
    }
    if ((transition.property_mask & subtitle::kSubtitleTransitionOutlineColor) != 0) {
        style.outline_color = lerpSubtitleColor(style.outline_color, target.outline_color, progress);
    }
    if ((transition.property_mask & subtitle::kSubtitleTransitionBackgroundColor) != 0) {
        style.background_color = lerpSubtitleColor(style.background_color, target.background_color, progress);
    }
    if ((transition.property_mask & subtitle::kSubtitleTransitionOutlineX) != 0) {
        style.outline_x = lerpDouble(style.outline_x, target.outline_x, progress);
        style.outline = (style.outline_x + style.outline_y) * 0.5;
    }
    if ((transition.property_mask & subtitle::kSubtitleTransitionOutlineY) != 0) {
        style.outline_y = lerpDouble(style.outline_y, target.outline_y, progress);
        style.outline = (style.outline_x + style.outline_y) * 0.5;
    }
    if ((transition.property_mask & subtitle::kSubtitleTransitionShadowX) != 0) {
        style.shadow_x = lerpDouble(style.shadow_x, target.shadow_x, progress);
        style.shadow = (std::abs(style.shadow_x) + std::abs(style.shadow_y)) * 0.5;
    }
    if ((transition.property_mask & subtitle::kSubtitleTransitionShadowY) != 0) {
        style.shadow_y = lerpDouble(style.shadow_y, target.shadow_y, progress);
        style.shadow = (std::abs(style.shadow_x) + std::abs(style.shadow_y)) * 0.5;
    }
    if ((transition.property_mask & subtitle::kSubtitleTransitionSpacing) != 0) {
        style.spacing = lerpDouble(style.spacing, target.spacing, progress);
    }
    if ((transition.property_mask & subtitle::kSubtitleTransitionScaleX) != 0) {
        style.scale_x_percent = lerpDouble(style.scale_x_percent, target.scale_x_percent, progress);
    }
    if ((transition.property_mask & subtitle::kSubtitleTransitionScaleY) != 0) {
        style.scale_y_percent = lerpDouble(style.scale_y_percent, target.scale_y_percent, progress);
    }
    if ((transition.property_mask & subtitle::kSubtitleTransitionRotationZ) != 0) {
        style.rotation_degrees = lerpDouble(style.rotation_degrees, target.rotation_degrees, progress);
    }
    if ((transition.property_mask & subtitle::kSubtitleTransitionRotationX) != 0) {
        style.rotation_x_degrees = lerpDouble(style.rotation_x_degrees, target.rotation_x_degrees, progress);
    }
    if ((transition.property_mask & subtitle::kSubtitleTransitionRotationY) != 0) {
        style.rotation_y_degrees = lerpDouble(style.rotation_y_degrees, target.rotation_y_degrees, progress);
    }
    if ((transition.property_mask & subtitle::kSubtitleTransitionShearX) != 0) {
        style.shear_x = lerpDouble(style.shear_x, target.shear_x, progress);
    }
    if ((transition.property_mask & subtitle::kSubtitleTransitionShearY) != 0) {
        style.shear_y = lerpDouble(style.shear_y, target.shear_y, progress);
    }
    if ((transition.property_mask & subtitle::kSubtitleTransitionRotationOrigin) != 0) {
        style.rotation_origin_x = lerpDouble(style.rotation_origin_x, target.rotation_origin_x, progress);
        style.rotation_origin_y = lerpDouble(style.rotation_origin_y, target.rotation_origin_y, progress);
        style.has_rotation_origin = style.has_rotation_origin || target.has_rotation_origin;
        if (progress >= 0.999f) {
            style.has_rotation_origin = target.has_rotation_origin;
        }
    }
}

subtitle::SubtitleStyle resolveAnimatedSubtitleStyle(const subtitle::SubtitleStyle& base_style,
                                                     const subtitle::SubtitleStyleAnimation& animation,
                                                     double item_elapsed_seconds,
                                                     double item_duration_seconds) {
    subtitle::SubtitleStyle style = base_style;
    for (const subtitle::SubtitleStyleTransition& transition : animation.style_transitions) {
        applyStyleTransition(style,
                             transition,
                             evaluateStyleTransitionProgress(transition, item_elapsed_seconds, item_duration_seconds));
    }
    return style;
}

SubtitleAnimationState resolveSubtitleAnimationState(const subtitle::SubtitleItem& item, double subtitle_clock_seconds) {
    SubtitleAnimationState state;
    const subtitle::SubtitleStyleAnimation& animation = item.animation;
    const double item_elapsed_seconds = subtitle_clock_seconds - item.start_seconds;
    const double item_duration_seconds = std::max(0.0, item.end_seconds - item.start_seconds);

    if (animation.has_move) {
        state.has_position = true;
        const double start_seconds = animation.move_has_timing ? animation.move_t1_seconds : 0.0;
        const double end_seconds = animation.move_has_timing ? animation.move_t2_seconds : item_duration_seconds;
        double progress = 1.0;
        if (item_elapsed_seconds <= start_seconds) {
            progress = 0.0;
        } else if (end_seconds > start_seconds + 0.0005) {
            progress = std::clamp((item_elapsed_seconds - start_seconds) / (end_seconds - start_seconds), 0.0, 1.0);
        }
        state.position_x = animation.move_x1 + (animation.move_x2 - animation.move_x1) * progress;
        state.position_y = animation.move_y1 + (animation.move_y2 - animation.move_y1) * progress;
    } else if (item.style.has_position) {
        state.has_position = true;
        state.position_x = item.style.position_x;
        state.position_y = item.style.position_y;
    }

    switch (animation.fade_mode) {
    case subtitle::SubtitleFadeMode::Simple: {
        const double fade_in = std::max(0.0, animation.fade_in_seconds);
        const double fade_out = std::max(0.0, animation.fade_out_seconds);
        float opacity = 1.0f;
        if (fade_in > 0.0005) {
            opacity = std::min(opacity,
                               static_cast<float>(std::clamp(item_elapsed_seconds / fade_in, 0.0, 1.0)));
        }
        if (fade_out > 0.0005) {
            opacity = std::min(opacity,
                               static_cast<float>(std::clamp((item_duration_seconds - item_elapsed_seconds) / fade_out,
                                                             0.0,
                                                             1.0)));
        }
        state.opacity = std::clamp(opacity, 0.0f, 1.0f);
        break;
    }
    case subtitle::SubtitleFadeMode::Complex:
        state.opacity = evaluateComplexFadeOpacity(animation, item_elapsed_seconds);
        break;
    case subtitle::SubtitleFadeMode::None:
    default:
        state.opacity = 1.0f;
        break;
    }

    return state;
}

float scaleSubtitleMetric(double value, int script_extent, float target_extent, float fallback) {
    if (script_extent > 0 && value > 0.0) {
        return static_cast<float>(value * static_cast<double>(target_extent) / static_cast<double>(script_extent));
    }
    if (value > 0.0) {
        return static_cast<float>(value);
    }
    return fallback;
}

float clampSubtitleCoordinate(float value, float lower, float upper) {
    if (lower > upper) {
        return (lower + upper) * 0.5f;
    }
    return std::clamp(value, lower, upper);
}

float subtitlePercentScale(double percent) {
    if (percent <= 0.0) {
        return 1.0f;
    }
    return std::max(0.01f, static_cast<float>(percent / 100.0));
}

DWRITE_WORD_WRAPPING toWordWrapping(int wrap_style) {
    return wrap_style == 2 ? DWRITE_WORD_WRAPPING_NO_WRAP : DWRITE_WORD_WRAPPING_WRAP;
}

float subtitleAnchorLocalX(float layout_width, int alignment) {
    switch (subtitleHorizontalGroup(alignment)) {
    case 0:
        return 0.0f;
    case 2:
        return layout_width;
    default:
        return layout_width * 0.5f;
    }
}

float subtitleAnchorLocalY(float layout_height, int alignment) {
    switch (subtitleVerticalGroup(alignment)) {
    case 2:
        return 0.0f;
    case 1:
        return layout_height * 0.5f;
    default:
        return layout_height;
    }
}

struct SubtitleLocalBounds {
    float left{0.0f};
    float top{0.0f};
    float right{0.0f};
    float bottom{0.0f};
    float width() const { return right - left; }
    float height() const { return bottom - top; }
    float centerY() const { return (top + bottom) * 0.5f; }
};

SubtitleLocalBounds computeTransformedSubtitleBounds(float rect_left,
                                                    float rect_top,
                                                    float rect_right,
                                                    float rect_bottom,
                                                    float anchor_local_x,
                                                    float anchor_local_y,
                                                    float scale_x,
                                                    float scale_y,
                                                    float rotation_degrees) {
    const float radians = static_cast<float>(rotation_degrees * 3.14159265358979323846 / 180.0);
    const float cosine = std::cos(radians);
    const float sine = std::sin(radians);

    const std::array<D2D1_POINT_2F, 4> corners = {{
        D2D1::Point2F(rect_left, rect_top),
        D2D1::Point2F(rect_right, rect_top),
        D2D1::Point2F(rect_left, rect_bottom),
        D2D1::Point2F(rect_right, rect_bottom),
    }};

    SubtitleLocalBounds bounds;
    bool initialized = false;
    for (const D2D1_POINT_2F& corner : corners) {
        const float local_x = (corner.x - anchor_local_x) * scale_x;
        const float local_y = (corner.y - anchor_local_y) * scale_y;
        const float rotated_x = local_x * cosine - local_y * sine;
        const float rotated_y = local_x * sine + local_y * cosine;
        const float transformed_x = anchor_local_x + rotated_x;
        const float transformed_y = anchor_local_y + rotated_y;
        if (!initialized) {
            bounds.left = bounds.right = transformed_x;
            bounds.top = bounds.bottom = transformed_y;
            initialized = true;
            continue;
        }
        bounds.left = std::min(bounds.left, transformed_x);
        bounds.right = std::max(bounds.right, transformed_x);
        bounds.top = std::min(bounds.top, transformed_y);
        bounds.bottom = std::max(bounds.bottom, transformed_y);
    }
    return bounds;
}

struct SubtitleAffineTransform {
    float m11{1.0f};
    float m12{0.0f};
    float m21{0.0f};
    float m22{1.0f};
    D2D1_POINT_2F origin{D2D1::Point2F(0.0f, 0.0f)};
};

float subtitleProjectedScale(float base_scale, double rotation_degrees) {
    float projection = static_cast<float>(std::cos(rotation_degrees * 3.14159265358979323846 / 180.0));
    if (std::abs(projection) < 0.01f) {
        projection = projection < 0.0f ? -0.01f : 0.01f;
    }
    return base_scale * projection;
}

SubtitleAffineTransform buildSubtitleAffineTransform(const subtitle::SubtitleStyle& style,
                                                     float base_scale_x,
                                                     float base_scale_y,
                                                     float script_scale_x,
                                                     float script_scale_y,
                                                     const SDL_Rect& video_rect,
                                                     const D2D1_POINT_2F& default_origin) {
    SubtitleAffineTransform transform;
    transform.origin = default_origin;
    if (style.has_rotation_origin) {
        transform.origin = D2D1::Point2F(
            static_cast<float>(video_rect.x) + static_cast<float>(style.rotation_origin_x) * script_scale_x,
            static_cast<float>(video_rect.y) + static_cast<float>(style.rotation_origin_y) * script_scale_y);
    }

    const float scale_x = subtitleProjectedScale(base_scale_x, style.rotation_y_degrees);
    const float scale_y = subtitleProjectedScale(base_scale_y, style.rotation_x_degrees);
    const float shear_x = static_cast<float>(style.shear_x);
    const float shear_y = static_cast<float>(style.shear_y);
    const float radians = static_cast<float>(style.rotation_degrees * 3.14159265358979323846 / 180.0);
    const float cosine = std::cos(radians);
    const float sine = std::sin(radians);

    transform.m11 = scale_x * (cosine - sine * shear_y);
    transform.m12 = scale_x * (sine + cosine * shear_y);
    transform.m21 = scale_y * (cosine * shear_x - sine);
    transform.m22 = scale_y * (sine * shear_x + cosine);
    return transform;
}

D2D1_POINT_2F transformSubtitleWorldPoint(float x,
                                          float y,
                                          const SubtitleAffineTransform& transform) {
    const float dx = x - transform.origin.x;
    const float dy = y - transform.origin.y;
    return D2D1::Point2F(transform.origin.x + transform.m11 * dx + transform.m21 * dy,
                         transform.origin.y + transform.m12 * dx + transform.m22 * dy);
}

SubtitleLocalBounds computeTransformedSubtitleBounds(float rect_left,
                                                    float rect_top,
                                                    float rect_right,
                                                    float rect_bottom,
                                                    const SubtitleAffineTransform& transform) {
    const std::array<D2D1_POINT_2F, 4> corners = {{
        D2D1::Point2F(rect_left, rect_top),
        D2D1::Point2F(rect_right, rect_top),
        D2D1::Point2F(rect_left, rect_bottom),
        D2D1::Point2F(rect_right, rect_bottom),
    }};

    SubtitleLocalBounds bounds;
    bool initialized = false;
    for (const D2D1_POINT_2F& corner : corners) {
        const D2D1_POINT_2F transformed = transformSubtitleWorldPoint(corner.x, corner.y, transform);
        if (!initialized) {
            bounds.left = bounds.right = transformed.x;
            bounds.top = bounds.bottom = transformed.y;
            initialized = true;
            continue;
        }
        bounds.left = std::min(bounds.left, transformed.x);
        bounds.right = std::max(bounds.right, transformed.x);
        bounds.top = std::min(bounds.top, transformed.y);
        bounds.bottom = std::max(bounds.bottom, transformed.y);
    }
    return bounds;
}

D2D1_MATRIX_3X2_F subtitleTransformMatrix(const SubtitleAffineTransform& transform) {
    return D2D1::Matrix3x2F(
        transform.m11,
        transform.m12,
        transform.m21,
        transform.m22,
        transform.origin.x - (transform.m11 * transform.origin.x + transform.m21 * transform.origin.y),
        transform.origin.y - (transform.m12 * transform.origin.x + transform.m22 * transform.origin.y));
}

float subtitleDrawingUnitScale(int drawing_scale) {
    if (drawing_scale <= 1) {
        return 1.0f;
    }
    const int shift = std::min(20, drawing_scale - 1);
    return 1.0f / static_cast<float>(1u << shift);
}

enum class AssVectorOpType {
    Move,
    Line,
    Bezier,
    Close,
};

struct AssVectorOp {
    AssVectorOpType type{AssVectorOpType::Move};
    std::array<D2D1_POINT_2F, 3> points{};
    size_t point_count{0};
};

struct ParsedAssVectorShape {
    std::vector<AssVectorOp> ops;
    SubtitleLocalBounds bounds{};
    bool valid{false};
};

bool isAssVectorCommandToken(const std::string& token) {
    return token.size() == 1 && std::isalpha(static_cast<unsigned char>(token[0])) != 0;
}

bool parseAssVectorFloat(const std::string& token, float& value) {
    try {
        value = std::stof(token);
        return true;
    } catch (...) {
        return false;
    }
}

std::vector<std::string> tokenizeAssVectorCommands(const std::string& commands) {
    std::vector<std::string> tokens;
    std::string current;
    const auto flush_current = [&]() {
        if (!current.empty()) {
            tokens.push_back(current);
            current.clear();
        }
    };

    for (char ch : commands) {
        const unsigned char uch = static_cast<unsigned char>(ch);
        if (std::isalpha(uch) != 0) {
            flush_current();
            std::string token(1, static_cast<char>(std::tolower(uch)));
            tokens.push_back(token);
            continue;
        }
        if (std::isdigit(uch) != 0 || ch == '-' || ch == '+' || ch == '.') {
            current.push_back(ch);
            continue;
        }
        flush_current();
    }
    flush_current();
    return tokens;
}

bool parseAssVectorShape(const std::string& commands,
                         float unit_scale_x,
                         float unit_scale_y,
                         bool normalize_origin,
                         ParsedAssVectorShape& shape_out) {
    shape_out = {};
    const std::vector<std::string> tokens = tokenizeAssVectorCommands(commands);
    if (tokens.empty()) {
        return false;
    }

    std::vector<AssVectorOp> ops;
    SubtitleLocalBounds bounds;
    bool bounds_initialized = false;
    const auto update_bounds = [&](float x, float y) {
        if (!bounds_initialized) {
            bounds.left = bounds.right = x;
            bounds.top = bounds.bottom = y;
            bounds_initialized = true;
            return;
        }
        bounds.left = std::min(bounds.left, x);
        bounds.right = std::max(bounds.right, x);
        bounds.top = std::min(bounds.top, y);
        bounds.bottom = std::max(bounds.bottom, y);
    };

    char command = '\0';
    size_t index = 0;
    while (index < tokens.size()) {
        if (isAssVectorCommandToken(tokens[index])) {
            command = static_cast<char>(std::tolower(static_cast<unsigned char>(tokens[index][0])));
            ++index;
            if (command == 'c') {
                ops.push_back(AssVectorOp{AssVectorOpType::Close, {}, 0});
            }
            continue;
        }

        if (command == '\0') {
            ++index;
            continue;
        }

        auto parse_point = [&](D2D1_POINT_2F& point) -> bool {
            if (index + 1 >= tokens.size() || isAssVectorCommandToken(tokens[index]) || isAssVectorCommandToken(tokens[index + 1])) {
                return false;
            }
            float x = 0.0f;
            float y = 0.0f;
            if (!parseAssVectorFloat(tokens[index], x) || !parseAssVectorFloat(tokens[index + 1], y)) {
                return false;
            }
            index += 2;
            point = D2D1::Point2F(x * unit_scale_x, y * unit_scale_y);
            update_bounds(point.x, point.y);
            return true;
        };

        if (command == 'm' || command == 'n') {
            bool first_pair = true;
            while (index < tokens.size() && !isAssVectorCommandToken(tokens[index])) {
                D2D1_POINT_2F point{};
                if (!parse_point(point)) {
                    break;
                }
                AssVectorOp op{};
                op.type = first_pair ? AssVectorOpType::Move : AssVectorOpType::Line;
                op.points[0] = point;
                op.point_count = 1;
                ops.push_back(op);
                first_pair = false;
            }
            continue;
        }

        if (command == 'l' || command == 's' || command == 'p') {
            while (index < tokens.size() && !isAssVectorCommandToken(tokens[index])) {
                D2D1_POINT_2F point{};
                if (!parse_point(point)) {
                    break;
                }
                AssVectorOp op{};
                op.type = AssVectorOpType::Line;
                op.points[0] = point;
                op.point_count = 1;
                ops.push_back(op);
            }
            continue;
        }

        if (command == 'b') {
            while (index < tokens.size() && !isAssVectorCommandToken(tokens[index])) {
                D2D1_POINT_2F p1{};
                D2D1_POINT_2F p2{};
                D2D1_POINT_2F p3{};
                if (!parse_point(p1) || !parse_point(p2) || !parse_point(p3)) {
                    break;
                }
                AssVectorOp op{};
                op.type = AssVectorOpType::Bezier;
                op.points[0] = p1;
                op.points[1] = p2;
                op.points[2] = p3;
                op.point_count = 3;
                ops.push_back(op);
            }
            continue;
        }

        ++index;
    }

    if (!bounds_initialized || ops.empty()) {
        return false;
    }

    if (normalize_origin) {
        for (AssVectorOp& op : ops) {
            for (size_t i = 0; i < op.point_count; ++i) {
                op.points[i].x -= bounds.left;
                op.points[i].y -= bounds.top;
            }
        }
        bounds.right -= bounds.left;
        bounds.bottom -= bounds.top;
        bounds.left = 0.0f;
        bounds.top = 0.0f;
    }

    shape_out.ops = std::move(ops);
    shape_out.bounds = bounds;
    shape_out.valid = true;
    return true;
}

bool buildAssVectorGeometry(ID2D1Factory* factory,
                            const ParsedAssVectorShape& shape,
                            float offset_x,
                            float offset_y,
                            ID2D1PathGeometry** geometry_out) {
    if (!factory || !geometry_out || !shape.valid) {
        return false;
    }

    *geometry_out = nullptr;
    ComPtr<ID2D1PathGeometry> geometry;
    if (FAILED(factory->CreatePathGeometry(geometry.GetAddressOf())) || !geometry) {
        return false;
    }

    ComPtr<ID2D1GeometrySink> sink;
    if (FAILED(geometry->Open(sink.GetAddressOf())) || !sink) {
        return false;
    }

    bool figure_open = false;
    for (const AssVectorOp& op : shape.ops) {
        switch (op.type) {
        case AssVectorOpType::Move: {
            if (figure_open) {
                sink->EndFigure(D2D1_FIGURE_END_CLOSED);
            }
            const D2D1_POINT_2F point = D2D1::Point2F(offset_x + op.points[0].x, offset_y + op.points[0].y);
            sink->BeginFigure(point, D2D1_FIGURE_BEGIN_FILLED);
            figure_open = true;
            break;
        }
        case AssVectorOpType::Line: {
            const D2D1_POINT_2F point = D2D1::Point2F(offset_x + op.points[0].x, offset_y + op.points[0].y);
            if (!figure_open) {
                sink->BeginFigure(point, D2D1_FIGURE_BEGIN_FILLED);
                figure_open = true;
            } else {
                sink->AddLine(point);
            }
            break;
        }
        case AssVectorOpType::Bezier: {
            if (!figure_open) {
                sink->BeginFigure(D2D1::Point2F(offset_x + op.points[2].x, offset_y + op.points[2].y),
                                  D2D1_FIGURE_BEGIN_FILLED);
                figure_open = true;
            }
            sink->AddBezier(D2D1::BezierSegment(
                D2D1::Point2F(offset_x + op.points[0].x, offset_y + op.points[0].y),
                D2D1::Point2F(offset_x + op.points[1].x, offset_y + op.points[1].y),
                D2D1::Point2F(offset_x + op.points[2].x, offset_y + op.points[2].y)));
            break;
        }
        case AssVectorOpType::Close:
            if (figure_open) {
                sink->EndFigure(D2D1_FIGURE_END_CLOSED);
                figure_open = false;
            }
            break;
        }
    }
    if (figure_open) {
        sink->EndFigure(D2D1_FIGURE_END_CLOSED);
    }
    sink->Close();

    *geometry_out = geometry.Detach();
    return true;
}

bool buildAssClipGeometry(ID2D1Factory* factory,
                          const std::string& commands,
                          int clip_scale,
                          const SDL_Rect& video_rect,
                          float script_scale_x,
                          float script_scale_y,
                          ID2D1PathGeometry** geometry_out) {
    ParsedAssVectorShape shape;
    const float unit_scale = subtitleDrawingUnitScale(clip_scale);
    if (!parseAssVectorShape(commands, script_scale_x * unit_scale, script_scale_y * unit_scale, false, shape)) {
        return false;
    }
    return buildAssVectorGeometry(factory,
                                  shape,
                                  static_cast<float>(video_rect.x),
                                  static_cast<float>(video_rect.y),
                                  geometry_out);
}

bool buildInverseClipGeometry(ID2D1Factory* factory,
                              ID2D1Geometry* clip_geometry,
                              const SDL_Rect& video_rect,
                              ID2D1PathGeometry** geometry_out) {
    if (!factory || !clip_geometry || !geometry_out) {
        return false;
    }

    *geometry_out = nullptr;
    ComPtr<ID2D1RectangleGeometry> full_rect_geometry;
    if (FAILED(factory->CreateRectangleGeometry(
            D2D1::RectF(static_cast<float>(video_rect.x),
                        static_cast<float>(video_rect.y),
                        static_cast<float>(video_rect.x + video_rect.w),
                        static_cast<float>(video_rect.y + video_rect.h)),
            full_rect_geometry.GetAddressOf())) || !full_rect_geometry) {
        return false;
    }

    ComPtr<ID2D1PathGeometry> combined_geometry;
    if (FAILED(factory->CreatePathGeometry(combined_geometry.GetAddressOf())) || !combined_geometry) {
        return false;
    }

    ComPtr<ID2D1GeometrySink> sink;
    if (FAILED(combined_geometry->Open(sink.GetAddressOf())) || !sink) {
        return false;
    }

    const HRESULT hr = full_rect_geometry->CombineWithGeometry(
        clip_geometry,
        D2D1_COMBINE_MODE_EXCLUDE,
        nullptr,
        D2D1_DEFAULT_FLATTENING_TOLERANCE,
        sink.Get());
    if (FAILED(hr) || FAILED(sink->Close())) {
        return false;
    }

    *geometry_out = combined_geometry.Detach();
    return true;
}

bool subtitleItemHasAnimatedRuns(const subtitle::SubtitleItem& item) {
    if (subtitleAnimationHasMotion(item.animation) ||
        subtitleAnimationHasFade(item.animation) ||
        subtitleAnimationHasStyleTransitions(item.animation)) {
        return true;
    }
    return std::any_of(item.runs.begin(), item.runs.end(), [](const subtitle::SubtitleTextRun& run) {
        return run.karaoke_mode != subtitle::SubtitleKaraokeMode::None &&
               run.karaoke_end_centiseconds > run.karaoke_start_centiseconds;
    });
}

bool subtitleItemsHaveAnimatedRuns(const std::vector<subtitle::SubtitleItem>& items) {
    return std::any_of(items.begin(), items.end(), [](const subtitle::SubtitleItem& item) {
        return subtitleItemHasAnimatedRuns(item);
    });
}

struct KaraokeRunVisualState {
    subtitle::SubtitleColor base_fill_color{};
    bool has_highlight_overlay{false};
    subtitle::SubtitleColor overlay_fill_color{};
    float overlay_progress{0.0f};
};

KaraokeRunVisualState resolveKaraokeRunVisualState(const subtitle::SubtitleStyle& style,
                                                   const subtitle::SubtitleTextRun& run,
                                                   double item_elapsed_seconds) {
    KaraokeRunVisualState state;
    state.base_fill_color = style.primary_color;
    if (run.karaoke_mode == subtitle::SubtitleKaraokeMode::None ||
        run.karaoke_end_centiseconds <= run.karaoke_start_centiseconds) {
        return state;
    }

    const double karaoke_start_seconds = static_cast<double>(run.karaoke_start_centiseconds) / 100.0;
    const double karaoke_end_seconds = static_cast<double>(run.karaoke_end_centiseconds) / 100.0;
    if (item_elapsed_seconds < karaoke_start_seconds) {
        state.base_fill_color = style.secondary_color;
        return state;
    }
    if (item_elapsed_seconds >= karaoke_end_seconds) {
        state.base_fill_color = style.primary_color;
        return state;
    }

    if (run.karaoke_mode == subtitle::SubtitleKaraokeMode::Sweep) {
        state.base_fill_color = style.secondary_color;
        state.has_highlight_overlay = true;
        state.overlay_fill_color = style.primary_color;
        const double duration = std::max(0.001, karaoke_end_seconds - karaoke_start_seconds);
        state.overlay_progress = static_cast<float>(
            std::clamp((item_elapsed_seconds - karaoke_start_seconds) / duration, 0.0, 1.0));
        return state;
    }

    state.base_fill_color = style.primary_color;
    return state;
}

struct KaraokeHighlightClip {
    D2D1_RECT_F world_rect{};
};

std::vector<KaraokeHighlightClip> buildKaraokeHighlightClips(const std::vector<DWRITE_HIT_TEST_METRICS>& metrics,
                                                             float progress,
                                                             float draw_x,
                                                             float draw_y,
                                                             const SubtitleAffineTransform& transform);

bool collectTextRangeMetrics(IDWriteTextLayout* text_layout,
                             UINT32 start_index,
                             UINT32 length,
                             std::vector<DWRITE_HIT_TEST_METRICS>& metrics) {
    metrics.clear();
    if (!text_layout || length == 0) {
        return false;
    }

    UINT32 actual_count = 0;
    HRESULT hr = text_layout->HitTestTextRange(start_index, length, 0.0f, 0.0f, nullptr, 0, &actual_count);
    if (FAILED(hr) || actual_count == 0) {
        return false;
    }

    metrics.resize(actual_count);
    hr = text_layout->HitTestTextRange(start_index,
                                       length,
                                       0.0f,
                                       0.0f,
                                       metrics.data(),
                                       actual_count,
                                       &actual_count);
    if (FAILED(hr) || actual_count == 0) {
        metrics.clear();
        return false;
    }
    metrics.resize(actual_count);
    return true;
}

std::vector<KaraokeHighlightClip> buildKaraokeHighlightClips(const std::vector<DWRITE_HIT_TEST_METRICS>& metrics,
                                                             float progress,
                                                             float draw_x,
                                                             float draw_y,
                                                             float anchor_local_x,
                                                             float anchor_local_y,
                                                             float glyph_scale_x,
                                                             float glyph_scale_y,
                                                             float rotation_degrees) {
    const float radians = static_cast<float>(rotation_degrees * 3.14159265358979323846 / 180.0);
    const float cosine = std::cos(radians);
    const float sine = std::sin(radians);
    SubtitleAffineTransform transform;
    transform.origin = D2D1::Point2F(draw_x + anchor_local_x, draw_y + anchor_local_y);
    transform.m11 = glyph_scale_x * cosine;
    transform.m12 = glyph_scale_x * sine;
    transform.m21 = -glyph_scale_y * sine;
    transform.m22 = glyph_scale_y * cosine;
    return buildKaraokeHighlightClips(metrics, progress, draw_x, draw_y, transform);
}

std::vector<KaraokeHighlightClip> buildKaraokeHighlightClips(const std::vector<DWRITE_HIT_TEST_METRICS>& metrics,
                                                             float progress,
                                                             float draw_x,
                                                             float draw_y,
                                                             const SubtitleAffineTransform& transform) {
    std::vector<KaraokeHighlightClip> clips;
    if (metrics.empty() || progress <= 0.0f) {
        return clips;
    }

    float total_width = 0.0f;
    for (const DWRITE_HIT_TEST_METRICS& metric : metrics) {
        total_width += metric.width;
    }
    if (total_width <= 0.001f) {
        return clips;
    }

    float remaining_width = total_width * std::clamp(progress, 0.0f, 1.0f);
    for (const DWRITE_HIT_TEST_METRICS& metric : metrics) {
        if (remaining_width <= 0.001f) {
            break;
        }
        const float highlight_width = std::min(metric.width, remaining_width);
        remaining_width -= highlight_width;

        const SubtitleLocalBounds world_bounds = computeTransformedSubtitleBounds(
            draw_x + metric.left,
            draw_y + metric.top,
            draw_x + metric.left + highlight_width,
            draw_y + metric.top + metric.height,
            transform);

        KaraokeHighlightClip clip{};
        clip.world_rect = D2D1::RectF(world_bounds.left, world_bounds.top, world_bounds.right, world_bounds.bottom);
        clips.push_back(clip);
    }
    return clips;
}

std::vector<D2D1_RECT_F> buildSubtitleClipRegions(const SDL_Rect& video_rect,
                                                  const D2D1_RECT_F& clip_rect,
                                                  bool inverse_clip) {
    std::vector<D2D1_RECT_F> regions;
    const float video_left = static_cast<float>(video_rect.x);
    const float video_top = static_cast<float>(video_rect.y);
    const float video_right = static_cast<float>(video_rect.x + video_rect.w);
    const float video_bottom = static_cast<float>(video_rect.y + video_rect.h);

    const D2D1_RECT_F bounded_clip = D2D1::RectF(
        std::clamp(clip_rect.left, video_left, video_right),
        std::clamp(clip_rect.top, video_top, video_bottom),
        std::clamp(clip_rect.right, video_left, video_right),
        std::clamp(clip_rect.bottom, video_top, video_bottom));

    const auto append_region = [&](float left, float top, float right, float bottom) {
        if ((right - left) > 0.001f && (bottom - top) > 0.001f) {
            regions.push_back(D2D1::RectF(left, top, right, bottom));
        }
    };

    if (!inverse_clip) {
        append_region(bounded_clip.left, bounded_clip.top, bounded_clip.right, bounded_clip.bottom);
        return regions;
    }

    if (bounded_clip.right <= bounded_clip.left || bounded_clip.bottom <= bounded_clip.top) {
        append_region(video_left, video_top, video_right, video_bottom);
        return regions;
    }

    append_region(video_left, video_top, video_right, bounded_clip.top);
    append_region(video_left, bounded_clip.top, bounded_clip.left, bounded_clip.bottom);
    append_region(bounded_clip.right, bounded_clip.top, video_right, bounded_clip.bottom);
    append_region(video_left, bounded_clip.bottom, video_right, video_bottom);
    return regions;
}

bool stringContainsCaseInsensitive(const std::string& text, const char* needle) {
    if (!needle || needle[0] == '\0') {
        return true;
    }
    return toLowerAscii(text).find(toLowerAscii(needle)) != std::string::npos;
}

enum class OpenGLNativeInteropOverride {
    Auto,
    Disable,
    Force,
};

enum class OpenGLPresentMode {
    Auto,
    Paced,
    Immediate,
};

enum class OpenGLHdrBridgeMode {
    Auto,
    Off,
    Force,
};

enum class OpenGLHdrBridgeDecision {
    NotEvaluated = 0,
    FrameSdr,
    DisabledByMode,
    UnsupportedPlatform,
    ProbeUnavailable,
    DisplayHdrInactive,
    ForceEnabled,
    AutoEnabled,
};

struct OpenGLNativeInteropRule {
    const char* rule_name;
    const char* reason;
    bool match_missing_wgl_dx_interop{false};
    std::optional<UINT> vendor_id{};
    std::optional<UINT> device_id{};
    std::optional<UINT> subsystem_id{};
    const char* gl_vendor_substring{nullptr};
    const char* gl_renderer_substring{nullptr};
    const char* adapter_name_substring{nullptr};
    const char* driver_version_prefix{nullptr};
};

struct OpenGLProbeContext {
    bool gl_context_created{false};
    std::string create_context_error{};
    std::string gl_vendor{"unknown"};
    std::string gl_renderer{"unknown"};
    std::string gl_version{"unknown"};
    bool has_wgl_dx_interop{false};
    D3D11DiagnosticsSnapshot d3d11{};
};

constexpr OpenGLNativeInteropRule kOpenGLNativeHardBlockerRules[] = {
    {"missing-wgl-nv-dx-interop",
     "required WGL_NV_DX_interop entry points are not available in the current OpenGL context",
     true,
     std::nullopt,
     std::nullopt,
     std::nullopt,
     nullptr,
     nullptr,
     nullptr,
     nullptr},
};

constexpr OpenGLNativeInteropRule kOpenGLNativeQuirkRules[] = {
    {"software-opengl-context",
     "software OpenGL contexts are excluded from WGL_NV_DX_interop video surfaces",
     false,
     std::nullopt,
     std::nullopt,
     std::nullopt,
     "microsoft",
     "gdi generic",
     nullptr,
     nullptr},
    {"google-swiftshader-opengl-context",
     "Google SwiftShader software OpenGL contexts are excluded from native D3D11 interop video surfaces",
     false,
     std::nullopt,
     std::nullopt,
     std::nullopt,
     "google",
     "swiftshader",
     nullptr,
     nullptr},
    {"mesa-llvmpipe-opengl-context",
     "Mesa llvmpipe software OpenGL contexts are excluded from native D3D11 interop video surfaces",
     false,
     std::nullopt,
     std::nullopt,
     std::nullopt,
     "mesa",
     "llvmpipe",
     nullptr,
     nullptr},
    {"vmware-virtual-gpu-opengl-context",
     "VMware virtual GPU OpenGL contexts are conservatively disabled for native D3D11 interop pending field validation",
     false,
     std::nullopt,
     std::nullopt,
     std::nullopt,
     "vmware",
     nullptr,
     "vmware",
     nullptr},
    {"parallels-virtual-gpu-opengl-context",
     "Parallels virtual GPU OpenGL contexts are conservatively disabled for native D3D11 interop pending field validation",
     false,
     std::nullopt,
     std::nullopt,
     std::nullopt,
     "parallels",
     nullptr,
     "parallels",
     nullptr},
    {"amd-radeon-native-interop",
     "AMD Radeon OpenGL native D3D11 interop is conservatively disabled due to field stutter/interop instability reports; use MVP_OPENGL_NATIVE_INTEROP=force to override",
     false,
     0x1002u,
     std::nullopt,
     std::nullopt,
     nullptr,
     nullptr,
     "radeon",
     nullptr},
};

OpenGLNativeInteropOverride readOpenGLNativeInteropOverride() {
    const auto value = readEnvVar("MVP_OPENGL_NATIVE_INTEROP");
    if (!value) {
        return OpenGLNativeInteropOverride::Auto;
    }
    const std::string normalized = toLowerAscii(*value);
    if (normalized == "0" || normalized == "off" || normalized == "false" ||
        normalized == "disable" || normalized == "disabled" || normalized == "copyback") {
        return OpenGLNativeInteropOverride::Disable;
    }
    if (normalized == "force" || normalized == "forced" || normalized == "always") {
        return OpenGLNativeInteropOverride::Force;
    }
    return OpenGLNativeInteropOverride::Auto;
}

bool readOpenGLForceInitFail() {
    const auto value = readEnvVar("MVP_OPENGL_FORCE_INIT_FAIL");
    if (!value) {
        return false;
    }
    std::string normalized = toLowerAscii(*value);
    normalized.erase(
        std::remove_if(normalized.begin(), normalized.end(), [](unsigned char ch) {
            return std::isspace(ch) != 0;
        }),
        normalized.end());
    return normalized == "1" ||
           normalized == "true" ||
           normalized == "yes" ||
           normalized == "on" ||
           normalized == "force";
}

const char* nativeInteropOverrideName(OpenGLNativeInteropOverride value) {
    switch (value) {
    case OpenGLNativeInteropOverride::Disable:
        return "disable";
    case OpenGLNativeInteropOverride::Force:
        return "force";
    default:
        return "auto";
    }
}

OpenGLPresentMode readOpenGLPresentMode() {
    const auto value = readEnvVar("MVP_OPENGL_PRESENT_MODE");
    if (!value) {
        return OpenGLPresentMode::Auto;
    }
    const std::string normalized = toLowerAscii(*value);
    if (normalized == "paced" || normalized == "vsync" || normalized == "sync") {
        return OpenGLPresentMode::Paced;
    }
    if (normalized == "immediate" || normalized == "async" || normalized == "nosync" ||
        normalized == "off" || normalized == "0") {
        return OpenGLPresentMode::Immediate;
    }
    return OpenGLPresentMode::Auto;
}

const char* openGLPresentModeName(OpenGLPresentMode value) {
    switch (value) {
    case OpenGLPresentMode::Paced:
        return "paced";
    case OpenGLPresentMode::Immediate:
        return "immediate";
    default:
        return "auto";
    }
}

OpenGLHdrBridgeMode readOpenGLHdrBridgeMode() {
    const auto value = readEnvVar("MVP_OPENGL_HDR_OUTPUT_MODE");
    if (!value) {
        return OpenGLHdrBridgeMode::Auto;
    }
    const std::string normalized = toLowerAscii(*value);
    if (normalized == "0" || normalized == "off" || normalized == "false" ||
        normalized == "disable" || normalized == "disabled" || normalized == "sdr") {
        return OpenGLHdrBridgeMode::Off;
    }
    if (normalized == "1" || normalized == "on" || normalized == "true" ||
        normalized == "force" || normalized == "forced" || normalized == "hdr") {
        return OpenGLHdrBridgeMode::Force;
    }
    return OpenGLHdrBridgeMode::Auto;
}

const char* openGLHdrBridgeModeName(OpenGLHdrBridgeMode value) {
    switch (value) {
    case OpenGLHdrBridgeMode::Off:
        return "off";
    case OpenGLHdrBridgeMode::Force:
        return "force";
    default:
        return "auto";
    }
}

const char* openGLHdrBridgeDecisionName(OpenGLHdrBridgeDecision value) {
    switch (value) {
    case OpenGLHdrBridgeDecision::FrameSdr:
        return "frame-sdr";
    case OpenGLHdrBridgeDecision::DisabledByMode:
        return "disabled-by-mode";
    case OpenGLHdrBridgeDecision::UnsupportedPlatform:
        return "unsupported-platform";
    case OpenGLHdrBridgeDecision::ProbeUnavailable:
        return "probe-unavailable";
    case OpenGLHdrBridgeDecision::DisplayHdrInactive:
        return "display-hdr-inactive";
    case OpenGLHdrBridgeDecision::ForceEnabled:
        return "force-enabled";
    case OpenGLHdrBridgeDecision::AutoEnabled:
        return "auto-enabled";
    default:
        return "not-evaluated";
    }
}

std::string readOpenGLOutputLutPath() {
    const auto value = readEnvVar("MVP_OPENGL_3DLUT_FILE");
    return value ? *value : std::string{};
}

std::string readOpenGLIccProfilePath() {
    const auto value = readEnvVar("MVP_OPENGL_ICC_PROFILE_FILE");
    return value ? *value : std::string{};
}

bool readOpenGLAutoIccProfileEnabled() {
    const auto value = readEnvVar("MVP_OPENGL_AUTO_ICC");
    if (!value) {
        return false;
    }
    const std::string normalized = toLowerAscii(*value);
    return normalized == "1" || normalized == "true" || normalized == "on" ||
           normalized == "yes" || normalized == "auto" || normalized == "enable" ||
           normalized == "enabled";
}

std::string describeOpenGLSwapInterval(int interval) {
    switch (interval) {
    case 1:
        return "paced";
    case 0:
        return "immediate";
    case -1:
        return "adaptive";
    default:
        return "interval(" + std::to_string(interval) + ")";
    }
}

std::string trimAscii(const std::string& text) {
    size_t start = 0;
    while (start < text.size() && std::isspace(static_cast<unsigned char>(text[start])) != 0) {
        ++start;
    }
    size_t end = text.size();
    while (end > start && std::isspace(static_cast<unsigned char>(text[end - 1])) != 0) {
        --end;
    }
    return text.substr(start, end - start);
}

bool tryParseFloatToken(const std::string& token, float& out) {
    if (token.empty()) {
        return false;
    }
    char* end = nullptr;
    const float value = std::strtof(token.c_str(), &end);
    if (end == token.c_str() || (end && *end != '\0') || !std::isfinite(value)) {
        return false;
    }
    out = value;
    return true;
}

bool tryParseIntToken(const std::string& token, int& out) {
    if (token.empty()) {
        return false;
    }
    char* end = nullptr;
    const long value = std::strtol(token.c_str(), &end, 10);
    if (end == token.c_str() || (end && *end != '\0')) {
        return false;
    }
    if (value < std::numeric_limits<int>::min() || value > std::numeric_limits<int>::max()) {
        return false;
    }
    out = static_cast<int>(value);
    return true;
}

struct OutputDisplayBindingState {
    bool binding_succeeded{false};
    int sdl_display_index{-1};
    std::string sdl_display_name{"unknown"};
    std::string output_device_name{"unknown"};
    bool icc_profile_available{false};
    std::string icc_profile_path{};
    std::string binding_error{};
};

#if defined(_WIN32)
std::string utf8FromWide(const wchar_t* text) {
    if (!text || text[0] == L'\0') {
        return {};
    }
    const int needed = WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);
    if (needed <= 1) {
        return {};
    }
    std::string result(static_cast<size_t>(needed - 1), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text, -1, result.data(), needed, nullptr, nullptr);
    return result;
}

bool tryGetWindowHandle(SDL_Window* window, HWND& hwnd) {
    hwnd = nullptr;
    if (!window) {
        return false;
    }
    SDL_SysWMinfo wm_info{};
    SDL_VERSION(&wm_info.version);
    if (!SDL_GetWindowWMInfo(window, &wm_info)) {
        return false;
    }
    hwnd = wm_info.info.win.window;
    return hwnd != nullptr;
}

OutputDisplayBindingState resolveOutputDisplayBinding(SDL_Window* window) {
    OutputDisplayBindingState state;
    if (!window) {
        state.binding_error = "window unavailable";
        return state;
    }

    const int display_index = SDL_GetWindowDisplayIndex(window);
    state.sdl_display_index = display_index;
    if (display_index >= 0) {
        state.binding_succeeded = true;
        if (const char* display_name = SDL_GetDisplayName(display_index)) {
            state.sdl_display_name = display_name;
        }
    } else {
        state.binding_error = "SDL_GetWindowDisplayIndex failed";
    }

    HWND hwnd = nullptr;
    if (!tryGetWindowHandle(window, hwnd)) {
        if (state.binding_error.empty()) {
            state.binding_error = "failed to resolve HWND";
        }
        return state;
    }

    const HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    if (!monitor) {
        if (state.binding_error.empty()) {
            state.binding_error = "MonitorFromWindow failed";
        }
        return state;
    }

    MONITORINFOEXW monitor_info{};
    monitor_info.cbSize = sizeof(monitor_info);
    if (!GetMonitorInfoW(monitor, &monitor_info)) {
        if (state.binding_error.empty()) {
            state.binding_error = "GetMonitorInfoW failed";
        }
        return state;
    }
    state.output_device_name = utf8FromWide(monitor_info.szDevice);

    HDC display_dc = CreateDCW(L"DISPLAY", monitor_info.szDevice, nullptr, nullptr);
    if (!display_dc) {
        if (state.binding_error.empty()) {
            state.binding_error = "CreateDCW failed";
        }
        return state;
    }

    DWORD chars = MAX_PATH;
    std::vector<wchar_t> profile_path(chars, L'\0');
    BOOL profile_ok = GetICMProfileW(display_dc, &chars, profile_path.data());
    if (!profile_ok && chars > profile_path.size()) {
        profile_path.assign(chars, L'\0');
        profile_ok = GetICMProfileW(display_dc, &chars, profile_path.data());
    }
    DeleteDC(display_dc);

    if (!profile_ok) {
        if (state.binding_error.empty()) {
            state.binding_error = "GetICMProfileW failed";
        }
        return state;
    }

    state.icc_profile_available = true;
    state.icc_profile_path = utf8FromWide(profile_path.data());
    return state;
}

const char* dxgiColorSpaceName(DXGI_COLOR_SPACE_TYPE color_space) {
    switch (color_space) {
    case DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709:
        return "rgb_full_g22_p709";
    case DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709:
        return "rgb_full_g10_p709";
    case DXGI_COLOR_SPACE_RGB_STUDIO_G22_NONE_P709:
        return "rgb_studio_g22_p709";
    case DXGI_COLOR_SPACE_RGB_STUDIO_G22_NONE_P2020:
        return "rgb_studio_g22_p2020";
    case DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020:
        return "rgb_full_pq_p2020";
    case DXGI_COLOR_SPACE_RGB_STUDIO_G2084_NONE_P2020:
        return "rgb_studio_pq_p2020";
    case DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P2020:
        return "rgb_full_g22_p2020";
    case DXGI_COLOR_SPACE_YCBCR_STUDIO_G2084_LEFT_P2020:
        return "ycbcr_studio_pq_p2020";
    case DXGI_COLOR_SPACE_YCBCR_STUDIO_GHLG_TOPLEFT_P2020:
        return "ycbcr_studio_hlg_p2020";
    case DXGI_COLOR_SPACE_YCBCR_FULL_GHLG_TOPLEFT_P2020:
        return "ycbcr_full_hlg_p2020";
    default:
        return "unknown";
    }
}

bool dxgiColorSpaceHdrActive(DXGI_COLOR_SPACE_TYPE color_space) {
    return color_space == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020 ||
           color_space == DXGI_COLOR_SPACE_RGB_STUDIO_G2084_NONE_P2020 ||
           color_space == DXGI_COLOR_SPACE_YCBCR_STUDIO_G2084_LEFT_P2020 ||
           color_space == DXGI_COLOR_SPACE_YCBCR_STUDIO_GHLG_TOPLEFT_P2020 ||
           color_space == DXGI_COLOR_SPACE_YCBCR_FULL_GHLG_TOPLEFT_P2020;
}

bool dxgiColorSpaceAdvancedColorActive(DXGI_COLOR_SPACE_TYPE color_space) {
    return color_space == DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709 ||
           color_space == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020 ||
           color_space == DXGI_COLOR_SPACE_RGB_STUDIO_G2084_NONE_P2020 ||
           color_space == DXGI_COLOR_SPACE_RGB_STUDIO_G22_NONE_P2020 ||
           color_space == DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P2020 ||
           color_space == DXGI_COLOR_SPACE_YCBCR_STUDIO_G2084_LEFT_P2020 ||
           color_space == DXGI_COLOR_SPACE_YCBCR_STUDIO_GHLG_TOPLEFT_P2020 ||
           color_space == DXGI_COLOR_SPACE_YCBCR_FULL_GHLG_TOPLEFT_P2020;
}

bool matchesD3D11DiagnosticsAdapter(const DXGI_ADAPTER_DESC1& desc, const D3D11DiagnosticsSnapshot& d3d11) {
    if (desc.VendorId != d3d11.vendor_id || desc.DeviceId != d3d11.device_id) {
        return false;
    }
    if (d3d11.subsystem_id != 0 && desc.SubSysId != d3d11.subsystem_id) {
        return false;
    }
    if (d3d11.revision != 0 && desc.Revision != d3d11.revision) {
        return false;
    }
    return true;
}

static OpenGLHdrOutputDiagnosticsSnapshot probeOpenGLHdrOutput(const D3D11DiagnosticsSnapshot& d3d11) {
    OpenGLHdrOutputDiagnosticsSnapshot snapshot;
    if (!d3d11.probe_succeeded) {
        snapshot.probe_error = "d3d11 diagnostics did not succeed";
        return snapshot;
    }

    ComPtr<IDXGIFactory1> factory;
    const HRESULT factory_hr = CreateDXGIFactory1(IID_PPV_ARGS(factory.GetAddressOf()));
    if (FAILED(factory_hr) || !factory) {
        snapshot.probe_error = "CreateDXGIFactory1 failed";
        return snapshot;
    }

    for (UINT adapter_index = 0;; ++adapter_index) {
        ComPtr<IDXGIAdapter1> adapter;
        const HRESULT adapter_hr = factory->EnumAdapters1(adapter_index, adapter.GetAddressOf());
        if (adapter_hr == DXGI_ERROR_NOT_FOUND) {
            break;
        }
        if (FAILED(adapter_hr) || !adapter) {
            continue;
        }

        DXGI_ADAPTER_DESC1 adapter_desc{};
        if (FAILED(adapter->GetDesc1(&adapter_desc))) {
            continue;
        }
        if (!matchesD3D11DiagnosticsAdapter(adapter_desc, d3d11)) {
            continue;
        }

        snapshot.adapter_matched = true;
        snapshot.adapter_index = adapter_index;

        ComPtr<IDXGIOutput> first_output;
        UINT first_output_index = 0;
        for (UINT output_index = 0;; ++output_index) {
            ComPtr<IDXGIOutput> output;
            const HRESULT output_hr = adapter->EnumOutputs(output_index, output.GetAddressOf());
            if (output_hr == DXGI_ERROR_NOT_FOUND) {
                break;
            }
            if (FAILED(output_hr) || !output) {
                continue;
            }

            DXGI_OUTPUT_DESC output_desc{};
            if (FAILED(output->GetDesc(&output_desc))) {
                continue;
            }

            if (!first_output) {
                first_output = output;
                first_output_index = output_index;
            }
            if (output_desc.AttachedToDesktop) {
                first_output = output;
                first_output_index = output_index;
                break;
            }
        }

        if (!first_output) {
            snapshot.probe_succeeded = true;
            snapshot.probe_error = "matched adapter has no DXGI outputs";
            return snapshot;
        }

        snapshot.output_found = true;
        snapshot.output_index = first_output_index;

        DXGI_OUTPUT_DESC output_desc{};
        if (SUCCEEDED(first_output->GetDesc(&output_desc))) {
            snapshot.output_name = utf8FromWide(output_desc.DeviceName);
            snapshot.output_device_name = utf8FromWide(output_desc.DeviceName);
            snapshot.output_attached_to_desktop = output_desc.AttachedToDesktop;
        }

        ComPtr<IDXGIOutput6> output6;
        if (SUCCEEDED(first_output.As(&output6)) && output6) {
            snapshot.has_output6 = true;
            DXGI_OUTPUT_DESC1 output_desc1{};
            if (SUCCEEDED(output6->GetDesc1(&output_desc1))) {
                snapshot.output_name = utf8FromWide(output_desc1.DeviceName);
                snapshot.output_device_name = utf8FromWide(output_desc1.DeviceName);
                snapshot.output_attached_to_desktop = output_desc1.AttachedToDesktop;
                snapshot.color_space = dxgiColorSpaceName(output_desc1.ColorSpace);
                snapshot.advanced_color_active = dxgiColorSpaceAdvancedColorActive(output_desc1.ColorSpace);
                snapshot.hdr_active = dxgiColorSpaceHdrActive(output_desc1.ColorSpace);
                snapshot.bits_per_color = output_desc1.BitsPerColor;
                snapshot.min_luminance_nits = static_cast<double>(output_desc1.MinLuminance);
                snapshot.max_luminance_nits = static_cast<double>(output_desc1.MaxLuminance);
                snapshot.max_full_frame_luminance_nits = static_cast<double>(output_desc1.MaxFullFrameLuminance);
            }
        } else {
            snapshot.probe_error = "IDXGIOutput6 unavailable; HDR output capability details are limited";
        }

        snapshot.probe_succeeded = true;
        return snapshot;
    }

    snapshot.probe_error = "no DXGI adapter matched the D3D11 diagnostics adapter";
    return snapshot;
}
#endif

#if !defined(_WIN32)
OutputDisplayBindingState resolveOutputDisplayBinding(SDL_Window* window) {
    OutputDisplayBindingState state;
    if (!window) {
        state.binding_error = "window unavailable";
        return state;
    }

    const int display_index = SDL_GetWindowDisplayIndex(window);
    state.sdl_display_index = display_index;
    if (display_index >= 0) {
        state.binding_succeeded = true;
        if (const char* display_name = SDL_GetDisplayName(display_index)) {
            state.sdl_display_name = display_name;
            state.output_device_name = display_name;
        }
    } else {
        state.binding_error = "SDL_GetWindowDisplayIndex failed";
    }
    return state;
}
#endif

void configureOpenGLContextAttributes() {
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
#if defined(SDL_GL_CONTEXT_PROFILE_MASK) && defined(SDL_GL_CONTEXT_PROFILE_COMPATIBILITY)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
#endif
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
}

bool hasRequiredWglDXInteropEntryPoints() {
#if defined(_WIN32)
    return SDL_GL_GetProcAddress("wglDXOpenDeviceNV") != nullptr &&
           SDL_GL_GetProcAddress("wglDXCloseDeviceNV") != nullptr &&
           SDL_GL_GetProcAddress("wglDXRegisterObjectNV") != nullptr &&
           SDL_GL_GetProcAddress("wglDXUnregisterObjectNV") != nullptr &&
           SDL_GL_GetProcAddress("wglDXLockObjectsNV") != nullptr &&
           SDL_GL_GetProcAddress("wglDXUnlockObjectsNV") != nullptr;
#else
    return false;
#endif
}

template <size_t N>
const OpenGLNativeInteropRule* findMatchingOpenGLNativeRule(const OpenGLNativeInteropRule (&rules)[N],
                                                            const OpenGLDiagnosticsSnapshot& snapshot) {
    for (const OpenGLNativeInteropRule& rule : rules) {
        if (rule.match_missing_wgl_dx_interop && snapshot.has_wgl_dx_interop) {
            continue;
        }
        if (rule.vendor_id && snapshot.d3d11.vendor_id != *rule.vendor_id) {
            continue;
        }
        if (rule.device_id && snapshot.d3d11.device_id != *rule.device_id) {
            continue;
        }
        if (rule.subsystem_id && snapshot.d3d11.subsystem_id != *rule.subsystem_id) {
            continue;
        }
        if (!stringContainsCaseInsensitive(snapshot.gl_vendor, rule.gl_vendor_substring)) {
            continue;
        }
        if (!stringContainsCaseInsensitive(snapshot.gl_renderer, rule.gl_renderer_substring)) {
            continue;
        }
        if (!stringContainsCaseInsensitive(snapshot.d3d11.adapter_name, rule.adapter_name_substring)) {
            continue;
        }
        if (!stringStartsWith(snapshot.d3d11.driver_version, rule.driver_version_prefix)) {
            continue;
        }
        return &rule;
    }
    return nullptr;
}

#if defined(_WIN32)
static OpenGLHdrOutputDiagnosticsSnapshot probeOpenGLHdrOutput(const D3D11DiagnosticsSnapshot& d3d11);
#endif

OpenGLDiagnosticsSnapshot buildOpenGLDiagnosticsSnapshot(const OpenGLProbeContext& probe,
                                                         OpenGLNativeInteropOverride override_mode) {
    OpenGLDiagnosticsSnapshot snapshot;
    snapshot.supported_platform = true;
    snapshot.gl_context_created = probe.gl_context_created;
    snapshot.create_context_error = probe.create_context_error;
    snapshot.gl_vendor = probe.gl_vendor;
    snapshot.gl_renderer = probe.gl_renderer;
    snapshot.gl_version = probe.gl_version;
    snapshot.has_wgl_dx_interop = probe.has_wgl_dx_interop;
    snapshot.native_interop_env_override = nativeInteropOverrideName(override_mode);
    snapshot.d3d11 = probe.d3d11;
#if defined(_WIN32)
    snapshot.hdr_output = probeOpenGLHdrOutput(snapshot.d3d11);
#endif
    snapshot.probe_succeeded = snapshot.gl_context_created && snapshot.d3d11.probe_succeeded;

    if (!snapshot.gl_context_created) {
        snapshot.hard_blocker_matched = true;
        snapshot.hard_blocker_rule = "gl-context-create-failed";
        snapshot.hard_blocker_reason = snapshot.create_context_error.empty()
            ? "failed to create temporary OpenGL context"
            : snapshot.create_context_error;
    } else if (!snapshot.d3d11.native_direct_allowed) {
        snapshot.hard_blocker_matched = true;
        snapshot.hard_blocker_rule = std::string("d3d11-") + snapshot.d3d11.native_direct_disable_rule;
        snapshot.hard_blocker_reason = snapshot.d3d11.native_direct_disable_reason.empty()
            ? "D3D11 diagnostics disabled native direct rendering"
            : snapshot.d3d11.native_direct_disable_reason;
    }

    if (const OpenGLNativeInteropRule* rule = findMatchingOpenGLNativeRule(kOpenGLNativeHardBlockerRules, snapshot)) {
        if (!snapshot.hard_blocker_matched) {
            snapshot.hard_blocker_matched = true;
            snapshot.hard_blocker_rule = rule->rule_name;
            snapshot.hard_blocker_reason = rule->reason;
        }
    }

    if (const OpenGLNativeInteropRule* rule = findMatchingOpenGLNativeRule(kOpenGLNativeQuirkRules, snapshot)) {
        snapshot.quirk_rule_matched = true;
        snapshot.quirk_rule_name = rule->rule_name;
        snapshot.quirk_rule_reason = rule->reason;
    }

    if (override_mode == OpenGLNativeInteropOverride::Disable) {
        snapshot.native_direct_allowed = false;
        snapshot.native_direct_startup_disabled = true;
        snapshot.native_direct_disable_rule = "env_disable";
        snapshot.native_direct_disable_reason = "MVP_OPENGL_NATIVE_INTEROP explicitly disabled native interop";
        return snapshot;
    }

    if (snapshot.hard_blocker_matched) {
        snapshot.native_direct_allowed = false;
        snapshot.native_direct_startup_disabled = true;
        snapshot.native_direct_disable_rule = snapshot.hard_blocker_rule;
        snapshot.native_direct_disable_reason = snapshot.hard_blocker_reason;
        return snapshot;
    }

    if (snapshot.quirk_rule_matched && override_mode != OpenGLNativeInteropOverride::Force) {
        snapshot.native_direct_allowed = false;
        snapshot.native_direct_startup_disabled = true;
        snapshot.native_direct_disable_rule = snapshot.quirk_rule_name;
        snapshot.native_direct_disable_reason = snapshot.quirk_rule_reason;
        return snapshot;
    }

    snapshot.native_direct_allowed = true;
    snapshot.native_direct_startup_disabled = false;
    if (override_mode == OpenGLNativeInteropOverride::Force) {
        snapshot.native_direct_disable_rule = "env_force";
        snapshot.native_direct_disable_reason.clear();
        snapshot.env_force_overrode_quirk = snapshot.quirk_rule_matched;
        return snapshot;
    }

    snapshot.native_direct_disable_rule = "none";
    snapshot.native_direct_disable_reason.clear();
    return snapshot;
}

D3D11DiagnosticsSnapshot probeD3D11DiagnosticsIfAvailable() {
#if defined(MVP_HAVE_D3D11_RENDERER) && MVP_HAVE_D3D11_RENDERER
    return D3D11VideoRenderer::probeSystemDiagnostics();
#else
    return {};
#endif
}

OpenGLDiagnosticsSnapshot probeOpenGLDiagnosticsWithCurrentContext(OpenGLNativeInteropOverride override_mode) {
    OpenGLProbeContext probe;
    probe.gl_context_created = true;
    if (const GLubyte* vendor = glGetString(GL_VENDOR)) {
        probe.gl_vendor = reinterpret_cast<const char*>(vendor);
    }
    if (const GLubyte* renderer = glGetString(GL_RENDERER)) {
        probe.gl_renderer = reinterpret_cast<const char*>(renderer);
    }
    if (const GLubyte* version = glGetString(GL_VERSION)) {
        probe.gl_version = reinterpret_cast<const char*>(version);
    }
    probe.has_wgl_dx_interop = hasRequiredWglDXInteropEntryPoints();
    probe.d3d11 = probeD3D11DiagnosticsIfAvailable();
    return buildOpenGLDiagnosticsSnapshot(probe, override_mode);
}

OpenGLDiagnosticsSnapshot probeOpenGLDiagnosticsWithTemporaryContext(OpenGLNativeInteropOverride override_mode) {
    OpenGLProbeContext probe;
    probe.d3d11 = probeD3D11DiagnosticsIfAvailable();
    OpenGLOutputDisplayDiagnosticsSnapshot output_display_snapshot;

    const Uint32 sdl_state = SDL_WasInit(0);
    const bool video_initialized_before = (SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO) != 0;
    bool started_sdl = false;
    bool started_video_subsystem = false;
    if (!video_initialized_before) {
        if (sdl_state == 0) {
            if (SDL_Init(SDL_INIT_VIDEO) != 0) {
                probe.create_context_error = std::string("SDL_Init(SDL_INIT_VIDEO) failed: ") + SDL_GetError();
                return buildOpenGLDiagnosticsSnapshot(probe, override_mode);
            }
            started_sdl = true;
        } else if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0) {
            probe.create_context_error = std::string("SDL_InitSubSystem(SDL_INIT_VIDEO) failed: ") + SDL_GetError();
            return buildOpenGLDiagnosticsSnapshot(probe, override_mode);
        } else {
            started_video_subsystem = true;
        }
    }

    SDL_Window* probe_window = nullptr;
    SDL_GLContext probe_context = nullptr;
    configureOpenGLContextAttributes();
    probe_window = SDL_CreateWindow("mvp-opengl-diagnostics-probe",
                                    SDL_WINDOWPOS_UNDEFINED,
                                    SDL_WINDOWPOS_UNDEFINED,
                                    16,
                                    16,
                                    SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    if (!probe_window) {
        probe.create_context_error = std::string("SDL_CreateWindow failed: ") + SDL_GetError();
    } else {
        probe_context = SDL_GL_CreateContext(probe_window);
        if (!probe_context) {
            probe.create_context_error = std::string("SDL_GL_CreateContext failed: ") + SDL_GetError();
        } else if (SDL_GL_MakeCurrent(probe_window, probe_context) != 0) {
            probe.create_context_error = std::string("SDL_GL_MakeCurrent failed: ") + SDL_GetError();
        } else {
            probe.gl_context_created = true;
            if (const GLubyte* vendor = glGetString(GL_VENDOR)) {
                probe.gl_vendor = reinterpret_cast<const char*>(vendor);
            }
            if (const GLubyte* renderer = glGetString(GL_RENDERER)) {
                probe.gl_renderer = reinterpret_cast<const char*>(renderer);
            }
            if (const GLubyte* version = glGetString(GL_VERSION)) {
                probe.gl_version = reinterpret_cast<const char*>(version);
            }
            probe.has_wgl_dx_interop = hasRequiredWglDXInteropEntryPoints();
            const OutputDisplayBindingState binding = resolveOutputDisplayBinding(probe_window);
            output_display_snapshot.binding_succeeded = binding.binding_succeeded;
            output_display_snapshot.sdl_display_index = binding.sdl_display_index;
            output_display_snapshot.sdl_display_name = binding.sdl_display_name;
            output_display_snapshot.output_device_name = binding.output_device_name.empty() ? std::string("unknown") : binding.output_device_name;
            output_display_snapshot.icc_profile_available = binding.icc_profile_available;
            output_display_snapshot.icc_profile_source = binding.icc_profile_available ? std::string("display-auto") : std::string("none");
            output_display_snapshot.icc_profile_path = binding.icc_profile_available ? binding.icc_profile_path : std::string("none");
            output_display_snapshot.binding_error = binding.binding_error;
        }
    }

    OpenGLDiagnosticsSnapshot snapshot = buildOpenGLDiagnosticsSnapshot(probe, override_mode);
    snapshot.output_display = output_display_snapshot;

    if (probe_context) {
        SDL_GL_DeleteContext(probe_context);
    }
    if (probe_window) {
        SDL_DestroyWindow(probe_window);
    }
    if (started_video_subsystem) {
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
    } else if (started_sdl) {
        SDL_Quit();
    }

    return snapshot;
}

void logOpenGLStartupDiagnostics(const OpenGLDiagnosticsSnapshot& snapshot) {
    LOG_INFO("[diag:opengl-native] gl_vendor=\"" << snapshot.gl_vendor << "\" gl_renderer=\"" << snapshot.gl_renderer
             << "\" gl_version=\"" << snapshot.gl_version << "\" env_override=" << snapshot.native_interop_env_override
             << " has_wgl_dx_interop=" << boolName(snapshot.has_wgl_dx_interop));
    LOG_INFO("[diag:opengl-native] adapter=\"" << snapshot.d3d11.adapter_name << "\" driver_version=" << snapshot.d3d11.driver_version
             << " native_direct_allowed=" << boolName(snapshot.native_direct_allowed)
             << " rule=" << snapshot.native_direct_disable_rule);
    LOG_INFO("[diag:opengl-native] hard_blocker_matched=" << boolName(snapshot.hard_blocker_matched)
             << " hard_rule=" << snapshot.hard_blocker_rule
             << " quirk_rule_matched=" << boolName(snapshot.quirk_rule_matched)
             << " quirk_rule=" << snapshot.quirk_rule_name
             << " force_overrode_quirk=" << boolName(snapshot.env_force_overrode_quirk));
    const bool h264_any = snapshot.d3d11.decoder_profiles.h264_vld_nofgt || snapshot.d3d11.decoder_profiles.h264_vld_fgt;
    const bool hevc_any = snapshot.d3d11.decoder_profiles.hevc_main || snapshot.d3d11.decoder_profiles.hevc_main10;
    const bool vp9_any = snapshot.d3d11.decoder_profiles.vp9_profile0 || snapshot.d3d11.decoder_profiles.vp9_profile2_10bit;
    const bool av1_any = snapshot.d3d11.decoder_profiles.av1_profile0 ||
                         snapshot.d3d11.decoder_profiles.av1_profile1 ||
                         snapshot.d3d11.decoder_profiles.av1_profile2 ||
                         snapshot.d3d11.decoder_profiles.av1_profile2_12bit ||
                         snapshot.d3d11.decoder_profiles.av1_profile2_12bit_420;
    LOG_INFO("[diag:opengl-native] decoder_profiles h264_any=" << boolName(h264_any)
             << " hevc_any=" << boolName(hevc_any)
             << " vp9_any=" << boolName(vp9_any)
             << " av1_any=" << boolName(av1_any));
    LOG_INFO("[diag:opengl-hdr] probe_succeeded=" << boolName(snapshot.hdr_output.probe_succeeded)
             << " adapter_matched=" << boolName(snapshot.hdr_output.adapter_matched)
             << " output_found=" << boolName(snapshot.hdr_output.output_found)
             << " output=\"" << snapshot.hdr_output.output_name << "\""
             << " color_space=" << snapshot.hdr_output.color_space
             << " advanced_color_active=" << boolName(snapshot.hdr_output.advanced_color_active)
             << " hdr_active=" << boolName(snapshot.hdr_output.hdr_active)
             << " bits_per_color=" << snapshot.hdr_output.bits_per_color
             << " min_luminance_nits=" << snapshot.hdr_output.min_luminance_nits
             << " max_luminance_nits=" << snapshot.hdr_output.max_luminance_nits
             << " max_full_frame_luminance_nits=" << snapshot.hdr_output.max_full_frame_luminance_nits
             << " note=\"" << snapshot.hdr_output.probe_error << '"');
    LOG_INFO("[diag:opengl-output-display] binding_succeeded=" << boolName(snapshot.output_display.binding_succeeded)
             << " sdl_display_index=" << snapshot.output_display.sdl_display_index
             << " sdl_display_name=\"" << snapshot.output_display.sdl_display_name << "\""
             << " output_device_name=\"" << snapshot.output_display.output_device_name << "\""
             << " icc_profile_available=" << boolName(snapshot.output_display.icc_profile_available)
             << " icc_profile_source=" << snapshot.output_display.icc_profile_source
             << " icc_profile_path=\"" << snapshot.output_display.icc_profile_path << "\""
             << " note=\"" << snapshot.output_display.binding_error << '"');
}

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

#if !defined(_WIN32)
bool subtitleAnimationHasMotion(const subtitle::SubtitleStyleAnimation& animation) {
    if (!animation.has_move) {
        return false;
    }
    if (std::abs(animation.move_x2 - animation.move_x1) <= 0.001 &&
        std::abs(animation.move_y2 - animation.move_y1) <= 0.001) {
        return false;
    }
    return !animation.move_has_timing || animation.move_t2_seconds > animation.move_t1_seconds;
}

bool subtitleAnimationHasFade(const subtitle::SubtitleStyleAnimation& animation) {
    switch (animation.fade_mode) {
    case subtitle::SubtitleFadeMode::Simple:
        return animation.fade_in_seconds > 0.0005 || animation.fade_out_seconds > 0.0005;
    case subtitle::SubtitleFadeMode::Complex:
        return animation.fade_alpha1 != animation.fade_alpha2 ||
               animation.fade_alpha2 != animation.fade_alpha3 ||
               animation.fade_t2_seconds > animation.fade_t1_seconds ||
               animation.fade_t4_seconds > animation.fade_t3_seconds;
    case subtitle::SubtitleFadeMode::None:
    default:
        return false;
    }
}

bool subtitleAnimationHasStyleTransitions(const subtitle::SubtitleStyleAnimation& animation) {
    return std::any_of(animation.style_transitions.begin(),
                       animation.style_transitions.end(),
                       [](const subtitle::SubtitleStyleTransition& transition) {
                           return transition.property_mask != 0;
                       });
}

bool subtitleItemHasAnimatedRuns(const subtitle::SubtitleItem& item) {
    if (subtitleAnimationHasMotion(item.animation) ||
        subtitleAnimationHasFade(item.animation) ||
        subtitleAnimationHasStyleTransitions(item.animation)) {
        return true;
    }
    return std::any_of(item.runs.begin(), item.runs.end(), [](const subtitle::SubtitleTextRun& run) {
        return run.karaoke_mode != subtitle::SubtitleKaraokeMode::None &&
               run.karaoke_end_centiseconds > run.karaoke_start_centiseconds;
    });
}

bool subtitleItemsHaveAnimatedRuns(const std::vector<subtitle::SubtitleItem>& items) {
    return std::any_of(items.begin(), items.end(), [](const subtitle::SubtitleItem& item) {
        return subtitleItemHasAnimatedRuns(item);
    });
}

enum class OpenGLPresentMode {
    Auto,
    Paced,
    Immediate,
};

enum class OpenGLHdrBridgeMode {
    Auto,
    Off,
    Force,
};

enum class OpenGLHdrBridgeDecision {
    NotEvaluated = 0,
    FrameSdr,
    DisabledByMode,
    UnsupportedPlatform,
    ProbeUnavailable,
    DisplayHdrInactive,
    ForceEnabled,
    AutoEnabled,
};

bool readOpenGLForceInitFail() {
    const auto value = readEnvVar("MVP_OPENGL_FORCE_INIT_FAIL");
    if (!value) {
        return false;
    }
    std::string normalized = toLowerAscii(*value);
    normalized.erase(
        std::remove_if(normalized.begin(), normalized.end(), [](unsigned char ch) {
            return std::isspace(ch) != 0;
        }),
        normalized.end());
    return normalized == "1" ||
           normalized == "true" ||
           normalized == "yes" ||
           normalized == "on" ||
           normalized == "force";
}

OpenGLPresentMode readOpenGLPresentMode() {
    const auto value = readEnvVar("MVP_OPENGL_PRESENT_MODE");
    if (!value) {
        return OpenGLPresentMode::Auto;
    }
    const std::string normalized = toLowerAscii(*value);
    if (normalized == "paced" || normalized == "vsync" || normalized == "sync") {
        return OpenGLPresentMode::Paced;
    }
    if (normalized == "immediate" || normalized == "async" || normalized == "nosync" ||
        normalized == "off" || normalized == "0") {
        return OpenGLPresentMode::Immediate;
    }
    return OpenGLPresentMode::Auto;
}

const char* openGLPresentModeName(OpenGLPresentMode value) {
    switch (value) {
    case OpenGLPresentMode::Paced:
        return "paced";
    case OpenGLPresentMode::Immediate:
        return "immediate";
    default:
        return "auto";
    }
}

OpenGLHdrBridgeMode readOpenGLHdrBridgeMode() {
    const auto value = readEnvVar("MVP_OPENGL_HDR_OUTPUT_MODE");
    if (!value) {
        return OpenGLHdrBridgeMode::Auto;
    }
    const std::string normalized = toLowerAscii(*value);
    if (normalized == "0" || normalized == "off" || normalized == "false" ||
        normalized == "disable" || normalized == "disabled" || normalized == "sdr") {
        return OpenGLHdrBridgeMode::Off;
    }
    if (normalized == "1" || normalized == "on" || normalized == "true" ||
        normalized == "force" || normalized == "forced" || normalized == "hdr") {
        return OpenGLHdrBridgeMode::Force;
    }
    return OpenGLHdrBridgeMode::Auto;
}

const char* openGLHdrBridgeModeName(OpenGLHdrBridgeMode value) {
    switch (value) {
    case OpenGLHdrBridgeMode::Off:
        return "off";
    case OpenGLHdrBridgeMode::Force:
        return "force";
    default:
        return "auto";
    }
}

const char* openGLHdrBridgeDecisionName(OpenGLHdrBridgeDecision value) {
    switch (value) {
    case OpenGLHdrBridgeDecision::FrameSdr:
        return "frame-sdr";
    case OpenGLHdrBridgeDecision::DisabledByMode:
        return "disabled-by-mode";
    case OpenGLHdrBridgeDecision::UnsupportedPlatform:
        return "unsupported-platform";
    case OpenGLHdrBridgeDecision::ProbeUnavailable:
        return "probe-unavailable";
    case OpenGLHdrBridgeDecision::DisplayHdrInactive:
        return "display-hdr-inactive";
    case OpenGLHdrBridgeDecision::ForceEnabled:
        return "force-enabled";
    case OpenGLHdrBridgeDecision::AutoEnabled:
        return "auto-enabled";
    default:
        return "not-evaluated";
    }
}

std::string readOpenGLOutputLutPath() {
    const auto value = readEnvVar("MVP_OPENGL_3DLUT_FILE");
    return value ? *value : std::string{};
}

std::string readOpenGLIccProfilePath() {
    const auto value = readEnvVar("MVP_OPENGL_ICC_PROFILE_FILE");
    return value ? *value : std::string{};
}

bool readOpenGLAutoIccProfileEnabled() {
    const auto value = readEnvVar("MVP_OPENGL_AUTO_ICC");
    if (!value) {
        return false;
    }
    const std::string normalized = toLowerAscii(*value);
    return normalized == "1" || normalized == "true" || normalized == "on" ||
           normalized == "yes" || normalized == "auto" || normalized == "enable" ||
           normalized == "enabled";
}

std::string describeOpenGLSwapInterval(int interval) {
    switch (interval) {
    case 1:
        return "paced";
    case 0:
        return "immediate";
    case -1:
        return "adaptive";
    default:
        return "interval(" + std::to_string(interval) + ")";
    }
}

std::string trimAscii(const std::string& text) {
    size_t start = 0;
    while (start < text.size() && std::isspace(static_cast<unsigned char>(text[start])) != 0) {
        ++start;
    }
    size_t end = text.size();
    while (end > start && std::isspace(static_cast<unsigned char>(text[end - 1])) != 0) {
        --end;
    }
    return text.substr(start, end - start);
}

struct OutputDisplayBindingState {
    bool binding_succeeded{false};
    int sdl_display_index{-1};
    std::string sdl_display_name{"unknown"};
    std::string output_device_name{"unknown"};
    bool icc_profile_available{false};
    std::string icc_profile_path{};
    std::string binding_error{};
};

OutputDisplayBindingState resolveOutputDisplayBinding(SDL_Window* window) {
    OutputDisplayBindingState state;
    if (!window) {
        state.binding_error = "window unavailable";
        return state;
    }

    const int display_index = SDL_GetWindowDisplayIndex(window);
    state.sdl_display_index = display_index;
    if (display_index >= 0) {
        state.binding_succeeded = true;
        if (const char* display_name = SDL_GetDisplayName(display_index)) {
            state.sdl_display_name = display_name;
            state.output_device_name = display_name;
        }
    } else {
        state.binding_error = "SDL_GetWindowDisplayIndex failed";
    }
    return state;
}
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
    void handleEvents();
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
    bool consumePreviousSubtitleTrackRequest();
    bool consumeNextSubtitleTrackRequest();
    bool consumeSubtitleDelayChangeRequest(double& delta_seconds);
    bool consumeAudioDelayChangeRequest(double& delta_seconds);
    bool consumeNextChapterRequest();
    bool consumePreviousChapterRequest();
    bool consumeNextItemRequest();
    bool consumePreviousItemRequest();
    bool consumeOpenFileRequest(std::string& path);
    void setOverlayState(double position, double duration, float volume, bool paused);
    void setSubtitleClock(double subtitle_time_seconds);
    void setSubtitleText(const std::string& text);
    void setSubtitleItems(const std::vector<subtitle::SubtitleItem>& items);
    void setSubtitleTrackState(int current_ordinal, int track_count);
    void setSubtitleTrackCatalog(const std::vector<std::string>& track_labels, int current_ordinal);
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
        uint64_t submission_id{0};
        AVColorRange color_range{AVCOL_RANGE_UNSPECIFIED};
        AVColorSpace color_space{AVCOL_SPC_UNSPECIFIED};
        AVColorTransferCharacteristic color_transfer{AVCOL_TRC_UNSPECIFIED};
        AVColorPrimaries color_primaries{AVCOL_PRI_UNSPECIFIED};
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
            submission_id = 0;
            color_range = AVCOL_RANGE_UNSPECIFIED;
            color_space = AVCOL_SPC_UNSPECIFIED;
            color_transfer = AVCOL_TRC_UNSPECIFIED;
            color_primaries = AVCOL_PRI_UNSPECIFIED;
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
            submission_id = other.submission_id;
            color_range = other.color_range;
            color_space = other.color_space;
            color_transfer = other.color_transfer;
            color_primaries = other.color_primaries;
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
            other.submission_id = 0;
            other.color_range = AVCOL_RANGE_UNSPECIFIED;
            other.color_space = AVCOL_SPC_UNSPECIFIED;
            other.color_transfer = AVCOL_TRC_UNSPECIFIED;
            other.color_primaries = AVCOL_PRI_UNSPECIFIED;
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
        bool previous_subtitle_track_requested{false};
        bool next_subtitle_track_requested{false};
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
        std::vector<subtitle::SubtitleItem> items;
    };

    struct OutputColorState {
        bool hdr_bridge_requested{false};
        bool hdr_bridge_active{false};
        OpenGLHdrBridgeDecision hdr_bridge_decision{OpenGLHdrBridgeDecision::NotEvaluated};
        bool lut_active{false};
    };

    struct ControlLayout {
        SDL_Rect panel{0, 0, 0, 0};
        SDL_Rect previous_item_button{0, 0, 0, 0};
        SDL_Rect play_button{0, 0, 0, 0};
        SDL_Rect next_item_button{0, 0, 0, 0};
        SDL_Rect subtitle_previous_button{0, 0, 0, 0};
        SDL_Rect subtitle_text{0, 0, 0, 0};
        SDL_Rect subtitle_next_button{0, 0, 0, 0};
        SDL_Rect time_text{0, 0, 0, 0};
        SDL_Rect mute_button{0, 0, 0, 0};
        SDL_Rect fullscreen_button{0, 0, 0, 0};
        SDL_Rect progress_track{0, 0, 0, 0};
        SDL_Rect progress_hit_box{0, 0, 0, 0};
        SDL_Rect volume_track{0, 0, 0, 0};
        SDL_Rect volume_hit_box{0, 0, 0, 0};
    };

    bool copyFrameData(const AVFrame& frame, PendingVideoFrame& out);
    bool refNativeFrame(const AVFrame& frame, PendingVideoFrame& out);
    void renderLoop();
    void pumpEvents();
    void handleKeyDown(int key_code, unsigned short modifiers, bool is_repeat);
    void handleMouseButtonDown(int mouse_x, int mouse_y, uint8_t button);
    void handleMouseMotion(int mouse_x, int mouse_y);
    void handleMouseButtonUp(uint8_t button);
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
    OutputColorState evaluateOutputColorState(const PendingVideoFrame& frame);
    bool initializeOutputLutTexture(const std::string& lut_source,
                                    const std::string& lut_path,
                                    std::string& profile_description,
                                    std::string& lut_error);
    void resetOutputLutTexture();
    void markOutputColorBindingDirty();
    void refreshOutputColorBinding(bool force_reload = false);
    void logColorPipelineIfChanged(const PendingVideoFrame& frame, bool native_path);
    void drawAspectQuad(int drawable_width, int drawable_height, int frame_width, int frame_height);
    void drawFilledRect(int drawable_width, int drawable_height, int x, int y, int w, int h, float r, float g, float b, float a);
    void drawFilledTriangle(int drawable_width,
                            int drawable_height,
                            float x1,
                            float y1,
                            float x2,
                            float y2,
                            float x3,
                            float y3,
                            float r,
                            float g,
                            float b,
                            float a);
    void drawFilledCircle(int drawable_width,
                          int drawable_height,
                          float center_x,
                          float center_y,
                          float radius,
                          float r,
                          float g,
                          float b,
                          float a);
    void drawOverlayText(int drawable_width,
                         int drawable_height,
                         int x,
                         int y,
                         int glyph_height,
                         const std::string& text,
                         float r,
                         float g,
                         float b,
                         float a);
    ControlLayout computeControlLayout(int drawable_width, int drawable_height) const;
    void drawSubtitleOverlay(const PendingVideoFrame* frame, int drawable_width, int drawable_height);
    void drawOsdOverlay(const PendingVideoFrame* frame, int drawable_width, int drawable_height);
    bool updateSubtitleTextureIfNeeded(const PendingVideoFrame* frame, int drawable_width, int drawable_height);
    SubtitleSnapshot snapshotSubtitleState() const;
    bool consumeFlagRequest(bool& flag);
    bool consumeDoubleRequest(bool& requested, double& value, double& out);
    bool consumeFloatRequest(bool& requested, float& value, float& out);

#if defined(_WIN32)
    struct NativeVideoVertex { float x; float y; float u; float v; };
    struct SubtitleBitmapCacheEntry {
        uint64_t key{0};
        int width{0};
        int height{0};
        std::vector<uint8_t> premul_bgra;
        ComPtr<ID2D1Bitmap> d2d_bitmap;
        uint64_t last_use_token{0};
    };
    bool initializeNativeInterop();
    void destroyNativeInterop();
    bool loadWglInteropFunctions();
    bool createNativeD3DDevice();
    bool createNativeD3DShaders();
    bool ensureNativeSourceViewsLocked(ID3D11Texture2D* texture, intptr_t index, DXGI_FORMAT format);
    bool ensureNativeInteropTarget(int width, int height);
    void resetNativeInteropTarget();
    bool updateNativeGlTexture(const AVFrame* frame, bool hdr_bridge_active);
    bool ensureSubtitleTextResources();
    void resetSubtitleTextResources();
    void clearSubtitleBitmapCache();
    ID2D1Bitmap* getCachedSubtitleBitmap(const subtitle::SubtitleBitmap& bitmap);
    bool renderSubtitleItemsWithD2D(const std::vector<subtitle::SubtitleItem>& items, const PendingVideoFrame* frame, int drawable_width, int drawable_height, const SDL_Rect& video_rect);
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
    std::thread::id event_thread_id_{};
    std::atomic<bool> event_thread_guard_reported_{false};
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
    uint64_t last_submitted_frame_id_{0};
    uint64_t last_presented_frame_id_{0};
    bool present_wait_warning_logged_{false};

    std::mutex request_mutex_;
    ControlRequests requests_{};
    bool open_file_requested_{false};
    std::string open_file_path_;
    float last_nonzero_volume_{1.0f};
    bool dragging_seek_{false};
    bool dragging_volume_{false};
    bool seek_preview_active_{false};
    double seek_preview_ratio_{0.0};
    bool hover_panel_{false};
    bool hover_previous_item_button_{false};
    bool hover_play_button_{false};
    bool hover_next_item_button_{false};
    bool hover_subtitle_previous_button_{false};
    bool hover_subtitle_next_button_{false};
    bool hover_progress_{false};
    bool hover_mute_button_{false};
    bool hover_volume_{false};
    bool hover_fullscreen_button_{false};

    std::atomic<double> overlay_position_{0.0};
    std::atomic<double> overlay_duration_{0.0};
    std::atomic<double> subtitle_clock_seconds_{0.0};
    std::atomic<float> overlay_volume_{1.0f};
    std::atomic<bool> overlay_paused_{false};
    std::atomic<uint64_t> osd_visible_until_ms_{0};

    mutable std::mutex subtitle_mutex_;
    std::vector<subtitle::SubtitleItem> subtitle_items_;
    std::atomic<uint64_t> subtitle_generation_{1};
    std::atomic<bool> subtitle_has_animated_content_{false};
    std::atomic<int> subtitle_track_current_ordinal_{0};
    std::atomic<int> subtitle_track_count_{0};
    std::vector<std::string> subtitle_track_labels_;
    std::string subtitle_track_feedback_text_;
    std::atomic<uint64_t> subtitle_track_feedback_until_ms_{0};
    input::HotkeyManager hotkey_manager_{};

    GLuint yuv420_program_{0};
    GLint yuv420_tex_y_location_{-1};
    GLint yuv420_tex_u_location_{-1};
    GLint yuv420_tex_v_location_{-1};
    GLint yuv420_coeff_r_location_{-1};
    GLint yuv420_coeff_g_location_{-1};
    GLint yuv420_coeff_b_location_{-1};
    GLint yuv420_color_config0_location_{-1};
    GLint yuv420_lut_location_{-1};
    GLuint nv12_program_{0};
    GLint nv12_tex_y_location_{-1};
    GLint nv12_tex_uv_location_{-1};
    GLint nv12_coeff_r_location_{-1};
    GLint nv12_coeff_g_location_{-1};
    GLint nv12_coeff_b_location_{-1};
    GLint nv12_color_config0_location_{-1};
    GLint nv12_lut_location_{-1};
    GLuint rgb_program_{0};
    GLint rgb_tex_location_{-1};
    GLint rgb_color_config0_location_{-1};
    GLint rgb_lut_location_{-1};

    GLuint tex_y_{0};
    GLuint tex_u_{0};
    GLuint tex_v_{0};
    GLuint tex_uv_{0};
    GLuint output_lut_texture_{0};
    int texture_width_{0};
    int texture_height_{0};
    AVPixelFormat texture_format_{AV_PIX_FMT_NONE};

    GLuint subtitle_texture_{0};
    int subtitle_texture_width_{0};
    int subtitle_texture_height_{0};
    bool subtitle_texture_valid_{false};
    uint64_t rendered_subtitle_generation_{0};
    double rendered_subtitle_clock_seconds_{-1.0};
    SDL_Rect subtitle_cached_video_rect_{0, 0, 0, 0};
    int subtitle_cached_drawable_width_{0};
    int subtitle_cached_drawable_height_{0};
    std::vector<uint8_t> subtitle_pixels_;
    std::string last_color_pipeline_signature_;
    std::string gl_vendor_;
    std::string gl_renderer_;
    std::string gl_version_;

    std::atomic<uint64_t> frame_copy_frames_{0};
    std::atomic<uint64_t> frame_copy_bytes_{0};
    std::atomic<uint64_t> frame_copy_time_us_total_{0};
    std::atomic<uint64_t> frame_copy_time_us_max_{0};
    std::atomic<uint64_t> native_interop_frames_{0};
    std::atomic<uint64_t> native_interop_disable_events_{0};
    std::atomic<uint64_t> present_wait_timeouts_{0};
    OpenGLPresentMode present_mode_requested_{OpenGLPresentMode::Auto};
    std::string present_mode_active_{"unknown"};
    OpenGLHdrBridgeMode hdr_bridge_mode_{OpenGLHdrBridgeMode::Auto};
    std::atomic<bool> hdr_bridge_requested_{false};
    std::atomic<bool> hdr_bridge_active_{false};
    std::atomic<int> hdr_bridge_decision_{static_cast<int>(OpenGLHdrBridgeDecision::NotEvaluated)};
    mutable std::mutex output_color_mutex_;
    std::string configured_cube_lut_path_;
    std::string configured_manual_icc_profile_path_;
    bool output_auto_icc_enabled_{false};
    std::string output_lut_path_;
    std::string output_lut_error_;
    std::string output_lut_source_{"none"};
    std::string output_display_name_{"unknown"};
    std::string output_display_device_name_{"unknown"};
    std::string output_icc_profile_source_{"none"};
    std::string output_icc_profile_path_;
    std::string output_icc_profile_description_{"unknown"};
    std::string output_binding_error_;
    std::atomic<bool> output_lut_configured_{false};
    std::atomic<bool> output_lut_active_{false};
    std::atomic<int> output_lut_size_{0};
    std::atomic<uint64_t> output_lut_reload_count_{0};
    std::atomic<int> output_display_index_{-1};
    std::atomic<bool> output_icc_profile_available_{false};
    std::atomic<bool> output_color_binding_dirty_{true};

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
    OpenGLDiagnosticsSnapshot native_diagnostics_{};
    OpenGLNativeInteropOverride native_override_mode_{OpenGLNativeInteropOverride::Auto};
    ComPtr<ID2D1Factory> subtitle_d2d_factory_;
    ComPtr<IDWriteFactory> subtitle_dwrite_factory_;
    ComPtr<IDWriteFontCollection> subtitle_dwrite_font_collection_;
    ComPtr<ID2D1DCRenderTarget> subtitle_d2d_target_;
    ComPtr<ID2D1SolidColorBrush> subtitle_fill_brush_;
    ComPtr<ID2D1SolidColorBrush> subtitle_shadow_brush_;
    ComPtr<ID2D1SolidColorBrush> subtitle_background_brush_;
    std::vector<SubtitleBitmapCacheEntry> subtitle_bitmap_cache_;
    uint64_t subtitle_bitmap_cache_use_token_{0};
#endif
};

bool OpenGLVideoRenderer::Impl::init(const VideoRendererConfig& config) {
    close();
    if (readOpenGLForceInitFail()) {
        LOG_WARNING("OpenGL renderer init force-failed by MVP_OPENGL_FORCE_INIT_FAIL");
        return false;
    }
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
    event_thread_id_ = std::this_thread::get_id();
    event_thread_guard_reported_.store(false);
    fullscreen_.store(false);
    minimized_.store(false);
    render_running_.store(true);
    render_initialized_.store(false);
    render_init_success_.store(false);
    fullscreen_toggle_requested_.store(false);
    clear_requested_.store(false);
    redraw_requested_.store(false);
    osd_visible_until_ms_.store(0);
    subtitle_track_current_ordinal_.store(0);
    subtitle_track_count_.store(0);
    subtitle_track_feedback_until_ms_.store(0);
    {
        std::lock_guard<std::mutex> subtitle_lock(subtitle_mutex_);
        subtitle_track_labels_.clear();
        subtitle_track_feedback_text_.clear();
    }
    pending_frame_.reset();
    pending_frame_ready_ = false;
    last_submitted_frame_id_ = 0;
    last_presented_frame_id_ = 0;
    present_wait_warning_logged_ = false;
    {
        std::lock_guard<std::mutex> lock(request_mutex_);
        requests_ = ControlRequests{};
        open_file_requested_ = false;
        open_file_path_.clear();
        last_nonzero_volume_ = 1.0f;
        dragging_seek_ = false;
        dragging_volume_ = false;
        seek_preview_active_ = false;
        seek_preview_ratio_ = 0.0;
        hover_panel_ = false;
        hover_previous_item_button_ = false;
        hover_play_button_ = false;
        hover_next_item_button_ = false;
        hover_subtitle_previous_button_ = false;
        hover_subtitle_next_button_ = false;
        hover_progress_ = false;
        hover_mute_button_ = false;
        hover_volume_ = false;
        hover_fullscreen_button_ = false;
    }
    resetDiagnostics();
    hdr_bridge_mode_ = readOpenGLHdrBridgeMode();
    hdr_bridge_requested_.store(false);
    hdr_bridge_active_.store(false);
    hdr_bridge_decision_.store(static_cast<int>(OpenGLHdrBridgeDecision::NotEvaluated));
    configured_cube_lut_path_ = trimAscii(readOpenGLOutputLutPath());
    configured_manual_icc_profile_path_ = trimAscii(readOpenGLIccProfilePath());
    output_auto_icc_enabled_ = readOpenGLAutoIccProfileEnabled();
    output_lut_path_.clear();
    output_lut_error_.clear();
    output_lut_source_ = "none";
    output_display_index_.store(-1);
    output_display_name_ = "unknown";
    output_display_device_name_ = "unknown";
    output_icc_profile_available_.store(false);
    output_icc_profile_source_ = "none";
    output_icc_profile_path_.clear();
    output_icc_profile_description_ = "unknown";
    output_binding_error_.clear();
    output_lut_configured_.store(false);
    output_lut_active_.store(false);
    output_lut_size_.store(0);
    output_lut_reload_count_.store(0);
    output_color_binding_dirty_.store(true);

    render_thread_ = std::thread(&Impl::renderLoop, this);
    for (int i = 0; i < 200 && !render_initialized_.load(); ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    if (!render_initialized_.load() || !render_init_success_.load()) {
        LOG_ERROR("OpenGL renderer thread initialization failed");
        close();
        return false;
    }

    showOsdFor();
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
        last_submitted_frame_id_ = 0;
        last_presented_frame_id_ = 0;
        present_wait_warning_logged_ = false;
    }
    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }
    title_.clear();
    width_.store(0);
    height_.store(0);
    should_quit_.store(false);
    event_thread_id_ = std::thread::id{};
    event_thread_guard_reported_.store(false);
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
    rendered_subtitle_clock_seconds_ = -1.0;
    subtitle_cached_video_rect_ = SDL_Rect{0, 0, 0, 0};
    subtitle_cached_drawable_width_ = 0;
    subtitle_cached_drawable_height_ = 0;
    subtitle_pixels_.clear();
    last_color_pipeline_signature_.clear();
    gl_vendor_.clear();
    gl_renderer_.clear();
    gl_version_.clear();
    hdr_bridge_mode_ = OpenGLHdrBridgeMode::Auto;
    configured_cube_lut_path_.clear();
    configured_manual_icc_profile_path_.clear();
    output_auto_icc_enabled_ = false;
    hdr_bridge_requested_.store(false);
    hdr_bridge_active_.store(false);
    hdr_bridge_decision_.store(static_cast<int>(OpenGLHdrBridgeDecision::NotEvaluated));
    output_lut_path_.clear();
    output_lut_error_.clear();
    output_lut_source_ = "none";
    output_lut_configured_.store(false);
    output_lut_active_.store(false);
    output_lut_size_.store(0);
    output_lut_reload_count_.store(0);
    output_display_index_.store(-1);
    output_display_name_ = "unknown";
    output_display_device_name_ = "unknown";
    output_icc_profile_available_.store(false);
    output_icc_profile_source_ = "none";
    output_icc_profile_path_.clear();
    output_icc_profile_description_ = "unknown";
    output_binding_error_.clear();
    output_color_binding_dirty_.store(true);
#if defined(_WIN32)
    native_startup_allowed_.store(false);
    native_session_disabled_.store(false);
    native_diagnostics_ = {};
    native_override_mode_ = OpenGLNativeInteropOverride::Auto;
    resetSubtitleTextResources();
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
        copy.submission_id = ++last_submitted_frame_id_;
        pending_frame_ = std::move(copy);
        pending_frame_ready_ = true;
    }
    redraw_requested_.store(true);
    frame_cv_.notify_one();
}

bool OpenGLVideoRenderer::Impl::copyFrameData(const AVFrame& frame, PendingVideoFrame& out) {
    const AVPixelFormat format = static_cast<AVPixelFormat>(frame.format);
    if ((format != AV_PIX_FMT_YUV420P && !isSoftwareTwoPlaneFormat(format)) ||
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
    out.color_transfer = frame.color_trc;
    out.color_primaries = frame.color_primaries;
    const int bytes_per_component = isHighBitDepthSemiPlanarFormat(format) ? 2 : 1;
    out.y_pitch = frame.width * bytes_per_component;
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
        out.u_pitch = frame.width * bytes_per_component;
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
    out.color_transfer = frame.color_trc;
    out.color_primaries = frame.color_primaries;
    out.valid = true;
    return true;
}

void OpenGLVideoRenderer::Impl::present() {
    std::unique_lock<std::mutex> lock(frame_mutex_);
    const uint64_t target_submission_id = last_submitted_frame_id_;
    redraw_requested_.store(true);
    frame_cv_.notify_one();
    if (target_submission_id == 0 ||
        last_presented_frame_id_ >= target_submission_id ||
        !render_running_.load() ||
        !render_initialized_.load() ||
        !render_init_success_.load()) {
        return;
    }

    const bool presented = frame_cv_.wait_for(lock, kOpenGLPresentWaitTimeout, [this, target_submission_id] {
        return !render_running_.load() || last_presented_frame_id_ >= target_submission_id;
    });
    if (presented) {
        present_wait_warning_logged_ = false;
        return;
    }
    present_wait_timeouts_.fetch_add(1);
    if (!present_wait_warning_logged_) {
        present_wait_warning_logged_ = true;
        LOG_WARNING("OpenGL present wait timed out: target_submission_id=" << target_submission_id
                    << " presented_submission_id=" << last_presented_frame_id_
                    << " pending_frame_ready=" << boolName(pending_frame_ready_));
    }
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
        PendingVideoFrame incoming_frame;
        bool have_new_frame = false;
        bool clear_now = clear_requested_.exchange(false);
        bool redraw_now = redraw_requested_.exchange(false);

        {
            std::unique_lock<std::mutex> lock(frame_mutex_);
            if (!pending_frame_ready_ && !clear_now && !redraw_now) {
                frame_cv_.wait_for(lock, std::chrono::milliseconds(8), [this] {
                    return !render_running_.load() || pending_frame_ready_ || clear_requested_.load() ||
                           redraw_requested_.load();
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
        if (!render_running_.load()) {
            break;
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
            const uint64_t rendered_submission_id = frame_to_render->submission_id;
            renderCurrentFrame(frame_to_render);
            if (rendered_submission_id != 0) {
                {
                    std::lock_guard<std::mutex> lock(frame_mutex_);
                    last_presented_frame_id_ = std::max(last_presented_frame_id_, rendered_submission_id);
                }
                frame_cv_.notify_all();
            }
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
    present_mode_requested_ = readOpenGLPresentMode();
    const char* requested_present_mode = openGLPresentModeName(present_mode_requested_);
    if (present_mode_requested_ == OpenGLPresentMode::Immediate) {
        if (SDL_GL_SetSwapInterval(0) != 0) {
            LOG_WARNING("OpenGL renderer could not force immediate presents, falling back to paced: " << SDL_GetError());
            if (SDL_GL_SetSwapInterval(1) != 0) {
                LOG_WARNING("OpenGL renderer could not enable swap interval=1 after immediate fallback: " << SDL_GetError());
            }
        }
    } else if (SDL_GL_SetSwapInterval(1) != 0) {
        LOG_WARNING("OpenGL renderer could not enable swap interval=1, falling back to immediate presents: " << SDL_GetError());
        if (SDL_GL_SetSwapInterval(0) != 0) {
            LOG_WARNING("OpenGL renderer could not disable swap interval after fallback: " << SDL_GetError());
        }
    }
    present_mode_active_ = describeOpenGLSwapInterval(SDL_GL_GetSwapInterval());
    LOG_INFO("[diag:opengl-present] requested=" << requested_present_mode
             << " active=" << present_mode_active_);

    const GLubyte* vendor = glGetString(GL_VENDOR);
    const GLubyte* renderer = glGetString(GL_RENDERER);
    const GLubyte* version = glGetString(GL_VERSION);
    gl_vendor_ = vendor ? reinterpret_cast<const char*>(vendor) : "unknown";
    gl_renderer_ = renderer ? reinterpret_cast<const char*>(renderer) : "unknown";
    gl_version_ = version ? reinterpret_cast<const char*>(version) : "unknown";
    LOG_INFO("OpenGL context: vendor=" << gl_vendor_
             << " renderer=" << gl_renderer_
             << " version=" << gl_version_);

    const GLuint vertex_shader = compileShader(GL_VERTEX_SHADER, kVertexShaderSource, "opengl-video-vertex");
    const GLuint yuv420_fragment_shader = compileShader(GL_FRAGMENT_SHADER, kYuv420FragmentShaderSource, "opengl-yuv420-fragment");
    const GLuint nv12_fragment_shader = compileShader(GL_FRAGMENT_SHADER, kNv12FragmentShaderSource, "opengl-nv12-fragment");
    const GLuint rgb_fragment_shader = compileShader(GL_FRAGMENT_SHADER, kRgbFragmentShaderSource, "opengl-rgb-fragment");
    if (vertex_shader == 0 || yuv420_fragment_shader == 0 || nv12_fragment_shader == 0 || rgb_fragment_shader == 0) {
        if (vertex_shader != 0) g_gl.DeleteShader(vertex_shader);
        if (yuv420_fragment_shader != 0) g_gl.DeleteShader(yuv420_fragment_shader);
        if (nv12_fragment_shader != 0) g_gl.DeleteShader(nv12_fragment_shader);
        if (rgb_fragment_shader != 0) g_gl.DeleteShader(rgb_fragment_shader);
        destroyGlResources();
        return false;
    }

    yuv420_program_ = linkProgram(vertex_shader, yuv420_fragment_shader, "opengl-yuv420-program");
    nv12_program_ = linkProgram(vertex_shader, nv12_fragment_shader, "opengl-nv12-program");
    rgb_program_ = linkProgram(vertex_shader, rgb_fragment_shader, "opengl-rgb-program");
    g_gl.DeleteShader(vertex_shader);
    g_gl.DeleteShader(yuv420_fragment_shader);
    g_gl.DeleteShader(nv12_fragment_shader);
    g_gl.DeleteShader(rgb_fragment_shader);
    if (yuv420_program_ == 0 || nv12_program_ == 0 || rgb_program_ == 0) {
        destroyGlResources();
        return false;
    }

    yuv420_tex_y_location_ = g_gl.GetUniformLocation(yuv420_program_, "texY");
    yuv420_tex_u_location_ = g_gl.GetUniformLocation(yuv420_program_, "texU");
    yuv420_tex_v_location_ = g_gl.GetUniformLocation(yuv420_program_, "texV");
    yuv420_coeff_r_location_ = g_gl.GetUniformLocation(yuv420_program_, "coeffR");
    yuv420_coeff_g_location_ = g_gl.GetUniformLocation(yuv420_program_, "coeffG");
    yuv420_coeff_b_location_ = g_gl.GetUniformLocation(yuv420_program_, "coeffB");
    yuv420_color_config0_location_ = g_gl.GetUniformLocation(yuv420_program_, "colorConfig0");
    yuv420_lut_location_ = g_gl.GetUniformLocation(yuv420_program_, "texLut3D");
    nv12_tex_y_location_ = g_gl.GetUniformLocation(nv12_program_, "texY");
    nv12_tex_uv_location_ = g_gl.GetUniformLocation(nv12_program_, "texUV");
    nv12_coeff_r_location_ = g_gl.GetUniformLocation(nv12_program_, "coeffR");
    nv12_coeff_g_location_ = g_gl.GetUniformLocation(nv12_program_, "coeffG");
    nv12_coeff_b_location_ = g_gl.GetUniformLocation(nv12_program_, "coeffB");
    nv12_color_config0_location_ = g_gl.GetUniformLocation(nv12_program_, "colorConfig0");
    nv12_lut_location_ = g_gl.GetUniformLocation(nv12_program_, "texLut3D");
    rgb_tex_location_ = g_gl.GetUniformLocation(rgb_program_, "texRgb");
    rgb_color_config0_location_ = g_gl.GetUniformLocation(rgb_program_, "colorConfig0");
    rgb_lut_location_ = g_gl.GetUniformLocation(rgb_program_, "texLut3D");

    g_gl.UseProgram(yuv420_program_);
    g_gl.Uniform1i(yuv420_tex_y_location_, 0);
    g_gl.Uniform1i(yuv420_tex_u_location_, 1);
    g_gl.Uniform1i(yuv420_tex_v_location_, 2);
    g_gl.Uniform1i(yuv420_lut_location_, 3);
    g_gl.UseProgram(nv12_program_);
    g_gl.Uniform1i(nv12_tex_y_location_, 0);
    g_gl.Uniform1i(nv12_tex_uv_location_, 1);
    g_gl.Uniform1i(nv12_lut_location_, 3);
    g_gl.UseProgram(rgb_program_);
    g_gl.Uniform1i(rgb_tex_location_, 0);
    g_gl.Uniform1i(rgb_lut_location_, 3);
    g_gl.UseProgram(0);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    refreshOutputColorBinding(true);
    LOG_INFO("[diag:opengl-output] hdr_mode=" << openGLHdrBridgeModeName(hdr_bridge_mode_)
             << " lut_configured=" << boolName(output_lut_configured_.load())
             << " lut_size=" << output_lut_size_.load()
             << " lut_source=" << output_lut_source_
             << " lut_path=" << (output_lut_path_.empty() ? std::string("none") : output_lut_path_)
             << " display=" << output_display_name_
             << " icc_source=" << output_icc_profile_source_);

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
        rgb_program_ = 0;
        yuv420_tex_y_location_ = yuv420_tex_u_location_ = yuv420_tex_v_location_ = -1;
        yuv420_coeff_r_location_ = yuv420_coeff_g_location_ = yuv420_coeff_b_location_ = -1;
        yuv420_color_config0_location_ = -1;
        yuv420_lut_location_ = -1;
        nv12_tex_y_location_ = nv12_tex_uv_location_ = -1;
        nv12_coeff_r_location_ = nv12_coeff_g_location_ = nv12_coeff_b_location_ = -1;
        nv12_color_config0_location_ = -1;
        nv12_lut_location_ = -1;
        rgb_tex_location_ = -1;
        rgb_color_config0_location_ = -1;
        rgb_lut_location_ = -1;
        tex_y_ = tex_u_ = tex_v_ = tex_uv_ = 0;
        output_lut_texture_ = 0;
        texture_width_ = texture_height_ = 0;
        texture_format_ = AV_PIX_FMT_NONE;
        subtitle_texture_ = 0;
        subtitle_texture_width_ = subtitle_texture_height_ = 0;
        subtitle_texture_valid_ = false;
        rendered_subtitle_clock_seconds_ = -1.0;
        last_color_pipeline_signature_.clear();
        gl_vendor_.clear();
        gl_renderer_.clear();
        gl_version_.clear();
        present_mode_requested_ = OpenGLPresentMode::Auto;
        present_mode_active_ = "unknown";
    };

    if (!gl_context_) {
        reset_state_only();
        return;
    }
    if (window_) SDL_GL_MakeCurrent(window_, gl_context_);
    if (g_gl.UseProgram) g_gl.UseProgram(0);
#if defined(_WIN32)
    destroyNativeInterop();
    resetSubtitleTextResources();
#endif
    resetOutputLutTexture();
    if (subtitle_texture_ != 0) glDeleteTextures(1, &subtitle_texture_);
    if (tex_y_ != 0) glDeleteTextures(1, &tex_y_);
    if (tex_u_ != 0) glDeleteTextures(1, &tex_u_);
    if (tex_v_ != 0) glDeleteTextures(1, &tex_v_);
    if (tex_uv_ != 0) glDeleteTextures(1, &tex_uv_);
    if (yuv420_program_ != 0 && g_gl.DeleteProgram) g_gl.DeleteProgram(yuv420_program_);
    if (nv12_program_ != 0 && g_gl.DeleteProgram) g_gl.DeleteProgram(nv12_program_);
    if (rgb_program_ != 0 && g_gl.DeleteProgram) g_gl.DeleteProgram(rgb_program_);
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

    const bool high_bit_depth = isHighBitDepthSemiPlanarFormat(frame.format);
    const GLenum y_internal_format = high_bit_depth ? GL_LUMINANCE16 : GL_LUMINANCE;
    const GLenum uv_internal_format = high_bit_depth ? GL_LUMINANCE16_ALPHA16 : GL_LUMINANCE_ALPHA;
    const GLenum upload_type = high_bit_depth ? GL_UNSIGNED_SHORT : GL_UNSIGNED_BYTE;

    const auto create_texture = [upload_type](GLuint& texture_id,
                                              GLenum internal_format,
                                              GLenum upload_format,
                                              int width,
                                              int height) -> bool {
        glGenTextures(1, &texture_id);
        if (texture_id == 0) return false;
        glBindTexture(GL_TEXTURE_2D, texture_id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(internal_format), width, height, 0, upload_format, upload_type, nullptr);
        return glGetError() == GL_NO_ERROR;
    };

    if (!create_texture(tex_y_, y_internal_format, GL_LUMINANCE, frame.width, frame.height)) {
        LOG_ERROR("OpenGL texture allocation failed for Y plane");
        return false;
    }
    const int chroma_width = chromaExtent(frame.width);
    const int chroma_height = chromaExtent(frame.height);
    bool ok = false;
    if (frame.format == AV_PIX_FMT_YUV420P) {
        ok = create_texture(tex_u_, GL_LUMINANCE, GL_LUMINANCE, chroma_width, chroma_height) &&
             create_texture(tex_v_, GL_LUMINANCE, GL_LUMINANCE, chroma_width, chroma_height);
    } else if (isSoftwareTwoPlaneFormat(frame.format)) {
        ok = create_texture(tex_uv_, uv_internal_format, GL_LUMINANCE_ALPHA, chroma_width, chroma_height);
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
    const bool high_bit_depth = isHighBitDepthSemiPlanarFormat(frame.format);
    const GLenum upload_type = high_bit_depth ? GL_UNSIGNED_SHORT : GL_UNSIGNED_BYTE;
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    g_gl.ActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex_y_);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, frame.width, frame.height, GL_LUMINANCE, upload_type, frame.y_plane.data());
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
    } else if (isSoftwareTwoPlaneFormat(frame.format)) {
        g_gl.ActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, tex_uv_);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, chroma_width, chroma_height, GL_LUMINANCE_ALPHA, upload_type, frame.u_plane.data());
        if (glGetError() != GL_NO_ERROR) return false;
    } else {
        return false;
    }
    return true;
}

void OpenGLVideoRenderer::Impl::resetOutputLutTexture() {
    if (output_lut_texture_ != 0) {
        glDeleteTextures(1, &output_lut_texture_);
        output_lut_texture_ = 0;
    }
    output_lut_configured_.store(false);
    output_lut_size_.store(0);
    output_lut_active_.store(false);
}

bool OpenGLVideoRenderer::Impl::initializeOutputLutTexture(const std::string& lut_source,
                                                           const std::string& lut_path,
                                                           std::string& profile_description,
                                                           std::string& lut_error) {
    resetOutputLutTexture();
    output_lut_configured_.store(false);
    output_lut_active_.store(false);
    output_lut_size_.store(0);
    profile_description = "unknown";
    lut_error.clear();
    if (lut_source == "none" || lut_path.empty()) {
        return true;
    }

    OutputLut3DData lut_data;
    if (lut_source == "cube") {
        if (!loadCubeLut3D(lut_path, lut_data, lut_error)) {
            return false;
        }
    } else {
        IccProfileSummary summary;
        if (!generateIccProfileLut(lut_path, 17, lut_data, summary, lut_error)) {
            return false;
        }
        profile_description = summary.description;
    }

    if (!g_gl.TexImage3D) {
        lut_error = "glTexImage3D unavailable";
        return false;
    }

    glGenTextures(1, &output_lut_texture_);
    if (output_lut_texture_ == 0) {
        lut_error = "glGenTextures failed";
        return false;
    }

    g_gl.ActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_3D, output_lut_texture_);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    g_gl.TexImage3D(GL_TEXTURE_3D,
                    0,
                    GL_RGB,
                    lut_data.size,
                    lut_data.size,
                    lut_data.size,
                    0,
                    GL_RGB,
                    GL_UNSIGNED_BYTE,
                    lut_data.rgb8.data());
    const GLenum lut_upload_error = glGetError();
    glBindTexture(GL_TEXTURE_3D, 0);
    g_gl.ActiveTexture(GL_TEXTURE0);

    if (lut_upload_error != GL_NO_ERROR) {
        lut_error = "glTexImage3D upload failed";
        resetOutputLutTexture();
        return false;
    }

    output_lut_configured_.store(true);
    output_lut_size_.store(lut_data.size);
    return true;
}

void OpenGLVideoRenderer::Impl::markOutputColorBindingDirty() {
    output_color_binding_dirty_.store(true);
}

void OpenGLVideoRenderer::Impl::refreshOutputColorBinding(bool force_reload) {
    if (force_reload) {
        output_color_binding_dirty_.store(false);
    } else if (!output_color_binding_dirty_.exchange(false)) {
        return;
    }

    const OutputDisplayBindingState binding = resolveOutputDisplayBinding(window_);

    const std::string selected_lut_source =
        !configured_cube_lut_path_.empty() ? std::string("cube")
        : !configured_manual_icc_profile_path_.empty() ? std::string("icc-manual")
        : (output_auto_icc_enabled_ && binding.icc_profile_available && !binding.icc_profile_path.empty())
              ? std::string("icc-display")
              : std::string("none");

    const std::string selected_lut_path =
        selected_lut_source == "cube" ? configured_cube_lut_path_
        : selected_lut_source == "icc-manual" ? configured_manual_icc_profile_path_
        : selected_lut_source == "icc-display" ? binding.icc_profile_path
        : std::string{};

    const bool selected_icc_available =
        selected_lut_source == "icc-manual" ||
        selected_lut_source == "icc-display";
    const std::string selected_icc_source =
        selected_lut_source == "icc-manual" ? std::string("manual")
        : selected_lut_source == "icc-display" ? std::string("display-auto")
        : std::string("none");

    std::string current_lut_path;
    std::string current_lut_source;
    std::string current_display_name;
    std::string current_display_device_name;
    std::string current_icc_profile_source;
    std::string current_binding_error;
    {
        std::lock_guard<std::mutex> lock(output_color_mutex_);
        current_lut_path = output_lut_path_;
        current_lut_source = output_lut_source_;
        current_display_name = output_display_name_;
        current_display_device_name = output_display_device_name_;
        current_icc_profile_source = output_icc_profile_source_;
        current_binding_error = output_binding_error_;
    }

    const bool changed = force_reload ||
                         selected_lut_path != current_lut_path ||
                         selected_lut_source != current_lut_source ||
                         binding.sdl_display_index != output_display_index_.load() ||
                         binding.sdl_display_name != current_display_name ||
                         binding.output_device_name != current_display_device_name ||
                         selected_icc_source != current_icc_profile_source ||
                         binding.binding_error != current_binding_error;

    if (!changed) {
        return;
    }

    std::string profile_description = "unknown";
    std::string lut_error;
    const bool lut_ready = initializeOutputLutTexture(selected_lut_source, selected_lut_path, profile_description, lut_error);

    {
        std::lock_guard<std::mutex> lock(output_color_mutex_);
        output_lut_path_ = selected_lut_path;
        output_lut_error_ = lut_error;
        output_lut_source_ = selected_lut_source;
        output_display_name_ = binding.sdl_display_name;
        output_display_device_name_ = binding.output_device_name.empty() ? std::string("unknown") : binding.output_device_name;
        output_icc_profile_source_ = selected_icc_source;
        output_icc_profile_path_ = selected_icc_available ? selected_lut_path : std::string{};
        output_icc_profile_description_ = selected_icc_available ? profile_description : std::string("unknown");
        output_binding_error_ = binding.binding_error;
    }
    output_display_index_.store(binding.sdl_display_index);
    output_icc_profile_available_.store(selected_icc_available);
    output_lut_reload_count_.fetch_add(1);

    if (!lut_ready) {
        LOG_WARNING("OpenGL output LUT refresh failed: source=" << selected_lut_source
                    << " path=" << (selected_lut_path.empty() ? std::string("none") : selected_lut_path)
                    << " error=" << lut_error);
    }
}

OpenGLVideoRenderer::Impl::OutputColorState OpenGLVideoRenderer::Impl::evaluateOutputColorState(const PendingVideoFrame& frame) {
    OutputColorState state;
    const bool frame_hdr = isHdrTransfer(frame.color_transfer);
    state.hdr_bridge_requested = frame_hdr && hdr_bridge_mode_ != OpenGLHdrBridgeMode::Off;

    if (!frame_hdr) {
        state.hdr_bridge_decision = OpenGLHdrBridgeDecision::FrameSdr;
    } else if (hdr_bridge_mode_ == OpenGLHdrBridgeMode::Off) {
        state.hdr_bridge_decision = OpenGLHdrBridgeDecision::DisabledByMode;
    } else {
#if defined(_WIN32)
        if (hdr_bridge_mode_ == OpenGLHdrBridgeMode::Force) {
            state.hdr_bridge_active = true;
            state.hdr_bridge_decision = OpenGLHdrBridgeDecision::ForceEnabled;
        } else if (!native_diagnostics_.hdr_output.probe_succeeded ||
                   !native_diagnostics_.hdr_output.output_found ||
                   !native_diagnostics_.hdr_output.has_output6) {
            state.hdr_bridge_decision = OpenGLHdrBridgeDecision::ProbeUnavailable;
        } else if (!(native_diagnostics_.hdr_output.hdr_active || native_diagnostics_.hdr_output.advanced_color_active)) {
            state.hdr_bridge_decision = OpenGLHdrBridgeDecision::DisplayHdrInactive;
        } else {
            state.hdr_bridge_active = true;
            state.hdr_bridge_decision = OpenGLHdrBridgeDecision::AutoEnabled;
        }
#else
        state.hdr_bridge_decision = OpenGLHdrBridgeDecision::UnsupportedPlatform;
#endif
    }

    state.lut_active = output_lut_configured_.load() && output_lut_texture_ != 0 && !state.hdr_bridge_active;

    hdr_bridge_requested_.store(state.hdr_bridge_requested);
    hdr_bridge_active_.store(state.hdr_bridge_active);
    hdr_bridge_decision_.store(static_cast<int>(state.hdr_bridge_decision));
    output_lut_active_.store(state.lut_active);
    return state;
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
    const OutputColorState output_state = evaluateOutputColorState(frame);
    logColorPipelineIfChanged(frame, false);
    const ColorPipelineConfig pipeline = buildColorPipelineConfig(frame.color_range,
                                                                  frame.color_space,
                                                                  frame.color_transfer,
                                                                  frame.color_primaries,
                                                                  colorSamplePrecisionForFormat(frame.format));
    const GLuint program = isSoftwareTwoPlaneFormat(frame.format) ? nv12_program_ : yuv420_program_;
    g_gl.UseProgram(program);
    if (isSoftwareTwoPlaneFormat(frame.format)) {
        g_gl.Uniform4f(nv12_coeff_r_location_, pipeline.coeffs.r[0], pipeline.coeffs.r[1], pipeline.coeffs.r[2], pipeline.coeffs.r[3]);
        g_gl.Uniform4f(nv12_coeff_g_location_, pipeline.coeffs.g[0], pipeline.coeffs.g[1], pipeline.coeffs.g[2], pipeline.coeffs.g[3]);
        g_gl.Uniform4f(nv12_coeff_b_location_, pipeline.coeffs.b[0], pipeline.coeffs.b[1], pipeline.coeffs.b[2], pipeline.coeffs.b[3]);
        g_gl.Uniform4f(nv12_color_config0_location_,
                       pipeline.transfer_mode,
                       pipeline.gamut_mode,
                       output_state.hdr_bridge_active ? 1.0f : 0.0f,
                       output_state.lut_active ? 1.0f : 0.0f);
    } else {
        g_gl.Uniform4f(yuv420_coeff_r_location_, pipeline.coeffs.r[0], pipeline.coeffs.r[1], pipeline.coeffs.r[2], pipeline.coeffs.r[3]);
        g_gl.Uniform4f(yuv420_coeff_g_location_, pipeline.coeffs.g[0], pipeline.coeffs.g[1], pipeline.coeffs.g[2], pipeline.coeffs.g[3]);
        g_gl.Uniform4f(yuv420_coeff_b_location_, pipeline.coeffs.b[0], pipeline.coeffs.b[1], pipeline.coeffs.b[2], pipeline.coeffs.b[3]);
        g_gl.Uniform4f(yuv420_color_config0_location_,
                       pipeline.transfer_mode,
                       pipeline.gamut_mode,
                       output_state.hdr_bridge_active ? 1.0f : 0.0f,
                       output_state.lut_active ? 1.0f : 0.0f);
    }
    g_gl.ActiveTexture(GL_TEXTURE3);
    if (output_state.lut_active && output_lut_texture_ != 0) {
        glBindTexture(GL_TEXTURE_3D, output_lut_texture_);
    } else {
        glBindTexture(GL_TEXTURE_3D, 0);
    }
    drawAspectQuad(drawable_width, drawable_height, frame.width, frame.height);
    g_gl.ActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_3D, 0);
    g_gl.ActiveTexture(GL_TEXTURE0);
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

void OpenGLVideoRenderer::Impl::drawFilledTriangle(int drawable_width,
                                                   int drawable_height,
                                                   float x1,
                                                   float y1,
                                                   float x2,
                                                   float y2,
                                                   float x3,
                                                   float y3,
                                                   float r,
                                                   float g,
                                                   float b,
                                                   float a) {
    const auto to_gl_x = [drawable_width](float value) {
        return (value / static_cast<float>(drawable_width)) * 2.0f - 1.0f;
    };
    const auto to_gl_y = [drawable_height](float value) {
        return 1.0f - (value / static_cast<float>(drawable_height)) * 2.0f;
    };
    glColor4f(r, g, b, a);
    glBegin(GL_TRIANGLES);
    glVertex2f(to_gl_x(x1), to_gl_y(y1));
    glVertex2f(to_gl_x(x2), to_gl_y(y2));
    glVertex2f(to_gl_x(x3), to_gl_y(y3));
    glEnd();
}

void OpenGLVideoRenderer::Impl::drawFilledCircle(int drawable_width,
                                                 int drawable_height,
                                                 float center_x,
                                                 float center_y,
                                                 float radius,
                                                 float r,
                                                 float g,
                                                 float b,
                                                 float a) {
    if (radius <= 0.5f) {
        return;
    }
    const auto to_gl_x = [drawable_width](float value) {
        return (value / static_cast<float>(drawable_width)) * 2.0f - 1.0f;
    };
    const auto to_gl_y = [drawable_height](float value) {
        return 1.0f - (value / static_cast<float>(drawable_height)) * 2.0f;
    };
    constexpr int kSegments = 24;
    glColor4f(r, g, b, a);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(to_gl_x(center_x), to_gl_y(center_y));
    for (int index = 0; index <= kSegments; ++index) {
        const float angle = (static_cast<float>(index) / static_cast<float>(kSegments)) * 6.28318530718f;
        const float px = center_x + std::cos(angle) * radius;
        const float py = center_y + std::sin(angle) * radius;
        glVertex2f(to_gl_x(px), to_gl_y(py));
    }
    glEnd();
}

void OpenGLVideoRenderer::Impl::drawOverlayText(int drawable_width,
                                                int drawable_height,
                                                int x,
                                                int y,
                                                int glyph_height,
                                                const std::string& text,
                                                float r,
                                                float g,
                                                float b,
                                                float a) {
    if (glyph_height <= 0 || text.empty()) {
        return;
    }
    const int stroke = std::max(2, glyph_height / 7);
    const int half_height = glyph_height / 2;
    const int spacing = std::max(2, glyph_height / 8);
    int cursor_x = x;

    for (char ch : text) {
        const int glyph_width = overlayGlyphWidth(ch, glyph_height, stroke);
        if (ch == ' ') {
            cursor_x += glyph_width + spacing;
            continue;
        }
        if (ch == ':') {
            const int dot_size = std::max(2, stroke);
            const int dot_x = cursor_x + std::max(0, (glyph_width - dot_size) / 2);
            drawFilledRect(drawable_width, drawable_height,
                           dot_x,
                           y + glyph_height / 4 - dot_size / 2,
                           dot_size,
                           dot_size,
                           r,
                           g,
                           b,
                           a);
            drawFilledRect(drawable_width, drawable_height,
                           dot_x,
                           y + (glyph_height * 3) / 4 - dot_size / 2,
                           dot_size,
                           dot_size,
                           r,
                           g,
                           b,
                           a);
            cursor_x += glyph_width + spacing;
            continue;
        }
        if (ch == '/') {
            const int step_count = std::max(4, glyph_height / 4);
            for (int step = 0; step < step_count; ++step) {
                const float t = static_cast<float>(step) / static_cast<float>(std::max(1, step_count - 1));
                const int px = cursor_x + static_cast<int>(std::lround((1.0f - t) * static_cast<float>(glyph_width - stroke)));
                const int py = y + static_cast<int>(std::lround(t * static_cast<float>(glyph_height - stroke)));
                drawFilledRect(drawable_width, drawable_height, px, py, stroke, stroke + 1, r, g, b, a);
            }
            cursor_x += glyph_width + spacing;
            continue;
        }

        const uint8_t mask = sevenSegmentMask(ch);
        const int horizontal_width = std::max(1, glyph_width - stroke * 2);
        const int vertical_height = std::max(1, half_height - stroke);
        if ((mask & kSegmentTop) != 0) {
            drawFilledRect(drawable_width, drawable_height, cursor_x + stroke, y, horizontal_width, stroke, r, g, b, a);
        }
        if ((mask & kSegmentUpperLeft) != 0) {
            drawFilledRect(drawable_width, drawable_height, cursor_x, y + stroke, stroke, vertical_height, r, g, b, a);
        }
        if ((mask & kSegmentUpperRight) != 0) {
            drawFilledRect(drawable_width, drawable_height,
                           cursor_x + glyph_width - stroke,
                           y + stroke,
                           stroke,
                           vertical_height,
                           r,
                           g,
                           b,
                           a);
        }
        if ((mask & kSegmentMiddle) != 0) {
            drawFilledRect(drawable_width, drawable_height,
                           cursor_x + stroke,
                           y + half_height - stroke / 2,
                           horizontal_width,
                           stroke,
                           r,
                           g,
                           b,
                           a);
        }
        if ((mask & kSegmentLowerLeft) != 0) {
            drawFilledRect(drawable_width, drawable_height,
                           cursor_x,
                           y + half_height,
                           stroke,
                           vertical_height,
                           r,
                           g,
                           b,
                           a);
        }
        if ((mask & kSegmentLowerRight) != 0) {
            drawFilledRect(drawable_width, drawable_height,
                           cursor_x + glyph_width - stroke,
                           y + half_height,
                           stroke,
                           vertical_height,
                           r,
                           g,
                           b,
                           a);
        }
        if ((mask & kSegmentBottom) != 0) {
            drawFilledRect(drawable_width, drawable_height,
                           cursor_x + stroke,
                           y + glyph_height - stroke,
                           horizontal_width,
                           stroke,
                           r,
                           g,
                           b,
                           a);
        }
        cursor_x += glyph_width + spacing;
    }
}

void OpenGLVideoRenderer::Impl::logColorPipelineIfChanged(const PendingVideoFrame& frame, bool native_path) {
    const ColorPipelineConfig pipeline = buildColorPipelineConfig(frame.color_range,
                                                                  frame.color_space,
                                                                  frame.color_transfer,
                                                                  frame.color_primaries,
                                                                  colorSamplePrecisionForFormat(frame.format));
    const char* format_name = av_get_pix_fmt_name(frame.format);
    std::ostringstream signature;
    signature << (native_path ? "native" : "software")
              << '|' << (format_name ? format_name : "unknown")
              << '|' << colorMatrixName(pipeline.matrix_kind)
              << '|' << colorRangeName(frame.color_range)
              << '|' << colorSpaceName(frame.color_space)
              << '|' << transferName(frame.color_transfer)
              << '|' << primariesName(frame.color_primaries)
              << '|' << (pipeline.hdr_tone_mapping ? '1' : '0')
              << '|' << (pipeline.gamut_mapping ? '1' : '0');
    const std::string value = signature.str();
    if (value == last_color_pipeline_signature_) {
        return;
    }
    last_color_pipeline_signature_ = value;

    std::ostringstream log_line;
    log_line << "[diag:opengl-color] path=" << (native_path ? "native" : "software")
             << " format=" << (format_name ? format_name : "unknown")
             << " matrix=" << colorMatrixName(pipeline.matrix_kind)
             << " range=" << colorRangeName(frame.color_range)
             << " colorspace=" << colorSpaceName(frame.color_space)
             << " transfer=" << transferName(frame.color_transfer)
             << " primaries=" << primariesName(frame.color_primaries)
             << " hdr=" << boolName(isHdrTransfer(frame.color_transfer))
             << " tone_map=" << (pipeline.hdr_tone_mapping ? "reinhard-sdr" : "off")
             << " gamut_map=" << (pipeline.gamut_mapping ? "bt2020-to-bt709" : "off");
    LOG_INFO(log_line.str());
}

void OpenGLVideoRenderer::Impl::renderNativeFrame(const PendingVideoFrame& frame, int drawable_width, int drawable_height) {
#if defined(_WIN32)
    const OutputColorState output_state = evaluateOutputColorState(frame);
    logColorPipelineIfChanged(frame, true);
    if (!updateNativeGlTexture(frame.native_frame, output_state.hdr_bridge_active) || native_gl_texture_ == 0) return;
    native_interop_frames_.fetch_add(1);
    g_gl.UseProgram(rgb_program_);
    g_gl.Uniform4f(rgb_color_config0_location_,
                   0.0f,
                   0.0f,
                   output_state.hdr_bridge_active ? 1.0f : 0.0f,
                   output_state.lut_active ? 1.0f : 0.0f);
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    g_gl.ActiveTexture(GL_TEXTURE3);
    if (output_state.lut_active && output_lut_texture_ != 0) {
        glBindTexture(GL_TEXTURE_3D, output_lut_texture_);
    } else {
        glBindTexture(GL_TEXTURE_3D, 0);
    }
    g_gl.ActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, native_gl_texture_);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    drawAspectQuad(drawable_width, drawable_height, frame.width, frame.height);
    glBindTexture(GL_TEXTURE_2D, 0);
    g_gl.ActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_3D, 0);
    g_gl.ActiveTexture(GL_TEXTURE0);
    glDisable(GL_TEXTURE_2D);
    g_gl.UseProgram(0);
#else
    (void)frame; (void)drawable_width; (void)drawable_height;
#endif
}

void OpenGLVideoRenderer::Impl::renderCurrentFrame(const PendingVideoFrame* frame) {
    if (!window_ || !gl_context_) return;
    SDL_GL_MakeCurrent(window_, gl_context_);
    refreshOutputColorBinding(false);

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
    snapshot.items = subtitle_items_;
    return snapshot;
}

#if defined(_WIN32)
bool OpenGLVideoRenderer::Impl::ensureSubtitleTextResources() {
    if (subtitle_d2d_factory_ && subtitle_dwrite_factory_ && subtitle_d2d_target_ &&
        subtitle_fill_brush_ && subtitle_shadow_brush_ && subtitle_background_brush_) {
        return true;
    }

    resetSubtitleTextResources();
    if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, subtitle_d2d_factory_.GetAddressOf())) || !subtitle_d2d_factory_) {
        LOG_WARNING("OpenGL subtitle D2D factory creation failed");
        resetSubtitleTextResources();
        return false;
    }
    if (FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,
                                   __uuidof(IDWriteFactory),
                                   reinterpret_cast<IUnknown**>(subtitle_dwrite_factory_.GetAddressOf()))) || !subtitle_dwrite_factory_) {
        LOG_WARNING("OpenGL subtitle DirectWrite factory creation failed");
        resetSubtitleTextResources();
        return false;
    }

    subtitle::SubtitleDirectWriteCollectionSummary font_collection_summary =
        subtitle::buildDirectWriteSubtitleFontCollection(subtitle_dwrite_factory_.Get(),
                                                         subtitle_dwrite_font_collection_.ReleaseAndGetAddressOf());
    if (font_collection_summary.collection_created) {
        LOG_INFO("OpenGL subtitle DirectWrite custom font collection enabled: registered="
                 << font_collection_summary.registered_font_file_count
                 << ", added=" << font_collection_summary.added_font_file_count);
    } else if (font_collection_summary.registered_font_file_count > 0) {
        LOG_WARNING("OpenGL subtitle DirectWrite custom font collection unavailable: "
                    << font_collection_summary.error
                    << " (registered=" << font_collection_summary.registered_font_file_count
                    << ", added=" << font_collection_summary.added_font_file_count << ")");
    }

    const D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
        D2D1_RENDER_TARGET_TYPE_DEFAULT,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
        96.0f,
        96.0f,
        D2D1_RENDER_TARGET_USAGE_NONE,
        D2D1_FEATURE_LEVEL_DEFAULT);
    if (FAILED(subtitle_d2d_factory_->CreateDCRenderTarget(&props, subtitle_d2d_target_.GetAddressOf())) || !subtitle_d2d_target_) {
        LOG_WARNING("OpenGL subtitle D2D DC render target creation failed");
        resetSubtitleTextResources();
        return false;
    }
    subtitle_d2d_target_->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
    subtitle_d2d_target_->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
    if (FAILED(subtitle_d2d_target_->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f), subtitle_fill_brush_.GetAddressOf())) ||
        FAILED(subtitle_d2d_target_->CreateSolidColorBrush(D2D1::ColorF(0.0f, 0.0f, 0.0f, 1.0f), subtitle_shadow_brush_.GetAddressOf())) ||
        FAILED(subtitle_d2d_target_->CreateSolidColorBrush(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.58f), subtitle_background_brush_.GetAddressOf()))) {
        LOG_WARNING("OpenGL subtitle brush creation failed");
        resetSubtitleTextResources();
        return false;
    }
    return true;
}

void OpenGLVideoRenderer::Impl::resetSubtitleTextResources() {
    clearSubtitleBitmapCache();
    subtitle_background_brush_.Reset();
    subtitle_shadow_brush_.Reset();
    subtitle_fill_brush_.Reset();
    subtitle_d2d_target_.Reset();
    subtitle_dwrite_font_collection_.Reset();
    subtitle_dwrite_factory_.Reset();
    subtitle_d2d_factory_.Reset();
    rendered_subtitle_clock_seconds_ = -1.0;
}

void OpenGLVideoRenderer::Impl::clearSubtitleBitmapCache() {
    subtitle_bitmap_cache_.clear();
    subtitle_bitmap_cache_use_token_ = 0;
}

ID2D1Bitmap* OpenGLVideoRenderer::Impl::getCachedSubtitleBitmap(const subtitle::SubtitleBitmap& bitmap) {
    if (!subtitle_d2d_target_ || bitmap.width <= 0 || bitmap.height <= 0) {
        return nullptr;
    }

    const size_t pixel_count = static_cast<size_t>(bitmap.width) * static_cast<size_t>(bitmap.height);
    if (bitmap.rgba.size() < pixel_count * 4u) {
        return nullptr;
    }

    const uint64_t key = hashSubtitleBitmap(bitmap);
    auto create_bitmap = [&](SubtitleBitmapCacheEntry& entry) -> bool {
        if (entry.premul_bgra.empty()) {
            entry.premul_bgra = premultiplySubtitleBitmapBgra(bitmap);
        }
        entry.d2d_bitmap.Reset();
        const D2D1_BITMAP_PROPERTIES bitmap_props = D2D1::BitmapProperties(
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
            96.0f,
            96.0f);
        const UINT32 pitch = static_cast<UINT32>(static_cast<size_t>(entry.width) * 4u);
        return SUCCEEDED(subtitle_d2d_target_->CreateBitmap(D2D1::SizeU(static_cast<UINT32>(entry.width),
                                                                        static_cast<UINT32>(entry.height)),
                                                            entry.premul_bgra.data(),
                                                            pitch,
                                                            &bitmap_props,
                                                            entry.d2d_bitmap.GetAddressOf())) &&
               entry.d2d_bitmap != nullptr;
    };

    for (SubtitleBitmapCacheEntry& entry : subtitle_bitmap_cache_) {
        if (entry.key != key || entry.width != bitmap.width || entry.height != bitmap.height) {
            continue;
        }
        entry.last_use_token = ++subtitle_bitmap_cache_use_token_;
        if (!entry.d2d_bitmap && !create_bitmap(entry)) {
            return nullptr;
        }
        return entry.d2d_bitmap.Get();
    }

    if (subtitle_bitmap_cache_.size() >= kSubtitleBitmapCacheLimit) {
        auto lru_it = std::min_element(subtitle_bitmap_cache_.begin(),
                                       subtitle_bitmap_cache_.end(),
                                       [](const SubtitleBitmapCacheEntry& lhs, const SubtitleBitmapCacheEntry& rhs) {
                                           return lhs.last_use_token < rhs.last_use_token;
                                       });
        if (lru_it != subtitle_bitmap_cache_.end()) {
            subtitle_bitmap_cache_.erase(lru_it);
        }
    }

    SubtitleBitmapCacheEntry entry;
    entry.key = key;
    entry.width = bitmap.width;
    entry.height = bitmap.height;
    entry.last_use_token = ++subtitle_bitmap_cache_use_token_;
    if (!create_bitmap(entry)) {
        return nullptr;
    }
    subtitle_bitmap_cache_.push_back(std::move(entry));
    return subtitle_bitmap_cache_.back().d2d_bitmap.Get();
}

bool OpenGLVideoRenderer::Impl::renderSubtitleItemsWithD2D(const std::vector<subtitle::SubtitleItem>& items,
                                                           const PendingVideoFrame* frame,
                                                           int drawable_width,
                                                           int drawable_height,
                                                           const SDL_Rect& video_rect) {
    (void)frame;
    if (!ensureSubtitleTextResources()) {
        return false;
    }

    GdiSurface surface;
    if (!createGdiSurface(drawable_width, drawable_height, surface)) {
        return false;
    }

    const RECT bind_rect{0, 0, drawable_width, drawable_height};
    HRESULT hr = subtitle_d2d_target_->BindDC(surface.dc, &bind_rect);
    if (FAILED(hr)) {
        LOG_WARNING("OpenGL subtitle D2D BindDC failed: hr=" << static_cast<long>(hr));
        destroyGdiSurface(surface);
        return false;
    }

    std::vector<subtitle::SubtitleItem> sorted_items = items;
    std::stable_sort(sorted_items.begin(), sorted_items.end(), [](const subtitle::SubtitleItem& lhs, const subtitle::SubtitleItem& rhs) {
        if (lhs.layer != rhs.layer) {
            return lhs.layer < rhs.layer;
        }
        return lhs.index < rhs.index;
    });

    const float fallback_margin_x = std::max(24.0f, static_cast<float>(video_rect.w) * 0.06f);
    const float fallback_margin_y = std::max(18.0f, static_cast<float>(video_rect.h) * 0.05f);
    const int reserved_bottom = kControlPanelHeight + kControlPanelInset + 8;
    const float top_limit = static_cast<float>(video_rect.y) + fallback_margin_y;
    const float bottom_limit = static_cast<float>(video_rect.y + video_rect.h) - fallback_margin_y - static_cast<float>(reserved_bottom);
    if (bottom_limit <= top_limit) {
        destroyGdiSurface(surface);
        return true;
    }

    float next_bottom_y = bottom_limit;
    float next_top_y = top_limit;
    float next_middle_y = top_limit + (bottom_limit - top_limit) * 0.5f;
    const float available_height = std::max(1.0f, bottom_limit - top_limit);
    const float fallback_font_size = std::clamp(static_cast<float>(video_rect.h) * kSubtitleFontScale,
                                                kMinSubtitleFontSize,
                                                kMaxSubtitleFontSize);

    subtitle_d2d_target_->BeginDraw();
    subtitle_d2d_target_->SetTransform(D2D1::Matrix3x2F::Identity());
    subtitle_d2d_target_->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f));
    const double subtitle_clock_seconds = subtitle_clock_seconds_.load();

    for (const subtitle::SubtitleItem& item : sorted_items) {
        const double item_elapsed_seconds = subtitle_clock_seconds - item.start_seconds;
        const double item_duration_seconds = std::max(0.0, item.end_seconds - item.start_seconds);
        const subtitle::SubtitleStyle item_style = resolveAnimatedSubtitleStyle(item.style,
                                                                                item.animation,
                                                                                item_elapsed_seconds,
                                                                                item_duration_seconds);
        if (item.is_vector_drawing && !item.drawing_commands.empty()) {
            const int alignment = clampSubtitleAlignment(item_style.alignment);
            const float script_scale_x = item.play_res_x > 0 ? static_cast<float>(video_rect.w) / static_cast<float>(item.play_res_x) : 1.0f;
            const float script_scale_y = item.play_res_y > 0 ? static_cast<float>(video_rect.h) / static_cast<float>(item.play_res_y) : 1.0f;
            const SubtitleAnimationState animation_state = resolveSubtitleAnimationState(item, subtitle_clock_seconds);
            if (animation_state.opacity <= 0.001f) {
                continue;
            }
            const float glyph_scale_x = subtitlePercentScale(item_style.scale_x_percent);
            const float glyph_scale_y = subtitlePercentScale(item_style.scale_y_percent);
            const float margin_left = item.play_res_x > 0
                ? std::max(4.0f, static_cast<float>(item_style.margin_l) * script_scale_x)
                : std::max(fallback_margin_x, static_cast<float>(item_style.margin_l));
            const float margin_right = item.play_res_x > 0
                ? std::max(4.0f, static_cast<float>(item_style.margin_r) * script_scale_x)
                : std::max(fallback_margin_x, static_cast<float>(item_style.margin_r));

            float font_size = fallback_font_size;
            if (item.play_res_y > 0 && item_style.font_size > 0.0) {
                font_size = static_cast<float>(item_style.font_size) * script_scale_y;
            } else if (item_style.font_size > 0.0) {
                font_size = std::max(fallback_font_size, static_cast<float>(item_style.font_size));
            }
            font_size = std::clamp(font_size, 10.0f, std::max(36.0f, static_cast<float>(video_rect.h) * 0.35f));

            ParsedAssVectorShape drawing_shape{};
            const float unit_scale = subtitleDrawingUnitScale(item.drawing_scale);
            if (!parseAssVectorShape(item.drawing_commands,
                                     script_scale_x * unit_scale,
                                     script_scale_y * unit_scale,
                                     true,
                                     drawing_shape) ||
                !drawing_shape.valid) {
                continue;
            }

            const float content_width = std::max(1.0f, drawing_shape.bounds.width());
            const float content_height = std::max(1.0f, drawing_shape.bounds.height());
            const float anchor_local_x = subtitleAnchorLocalX(content_width, alignment);
            const float anchor_local_y = subtitleAnchorLocalY(content_height, alignment);
            subtitle::SubtitleStyle preview_style = item_style;
            preview_style.has_rotation_origin = false;
            const SubtitleAffineTransform preview_transform = buildSubtitleAffineTransform(
                preview_style,
                glyph_scale_x,
                glyph_scale_y,
                script_scale_x,
                script_scale_y,
                video_rect,
                D2D1::Point2F(anchor_local_x, anchor_local_y));
            const SubtitleLocalBounds local_bounds = computeTransformedSubtitleBounds(
                drawing_shape.bounds.left,
                drawing_shape.bounds.top,
                drawing_shape.bounds.right,
                drawing_shape.bounds.bottom,
                preview_transform);
            const float transformed_height = std::max(1.0f, local_bounds.height());
            const float gap = std::max(6.0f, std::max(font_size, content_height) * 0.22f * glyph_scale_y);

            float draw_x = static_cast<float>(video_rect.x) + margin_left;
            if (animation_state.has_position) {
                const float anchor_x = static_cast<float>(video_rect.x) + static_cast<float>(animation_state.position_x) * script_scale_x;
                draw_x = anchor_x - anchor_local_x;
            } else {
                switch (subtitleHorizontalGroup(alignment)) {
                case 2:
                    draw_x = static_cast<float>(video_rect.x + video_rect.w) - margin_right - content_width;
                    break;
                case 1:
                    draw_x = static_cast<float>(video_rect.x) + (static_cast<float>(video_rect.w) - content_width) * 0.5f;
                    break;
                default:
                    draw_x = static_cast<float>(video_rect.x) + margin_left;
                    break;
                }
            }
            draw_x = clampSubtitleCoordinate(draw_x,
                                             static_cast<float>(video_rect.x) - local_bounds.left,
                                             static_cast<float>(video_rect.x + video_rect.w) - local_bounds.right);

            float draw_y = top_limit;
            if (animation_state.has_position) {
                const float anchor_y = static_cast<float>(video_rect.y) + static_cast<float>(animation_state.position_y) * script_scale_y;
                draw_y = anchor_y - anchor_local_y;
            } else {
                switch (subtitleVerticalGroup(alignment)) {
                case 2:
                    draw_y = next_top_y - local_bounds.top;
                    next_top_y += transformed_height + gap;
                    break;
                case 1:
                    draw_y = next_middle_y - local_bounds.centerY();
                    next_middle_y += transformed_height + gap;
                    break;
                default:
                    draw_y = next_bottom_y - local_bounds.bottom;
                    next_bottom_y = draw_y + local_bounds.top - gap;
                    break;
                }
            }
            draw_y = clampSubtitleCoordinate(draw_y, top_limit - local_bounds.top, bottom_limit - local_bounds.bottom);

            const float outline_radius_x = std::max(0.0f, static_cast<float>(item_style.outline_x) * script_scale_x);
            const float outline_radius_y = std::max(0.0f, static_cast<float>(item_style.outline_y) * script_scale_y);
            const float shadow_offset_x = static_cast<float>(item_style.shadow_x) * script_scale_x;
            const float shadow_offset_y = static_cast<float>(item_style.shadow_y) * script_scale_y;
            const D2D1_POINT_2F anchor_point = D2D1::Point2F(draw_x + anchor_local_x, draw_y + anchor_local_y);
            const SubtitleAffineTransform item_transform_state = buildSubtitleAffineTransform(
                item_style,
                glyph_scale_x,
                glyph_scale_y,
                script_scale_x,
                script_scale_y,
                video_rect,
                anchor_point);
            const D2D1_MATRIX_3X2_F item_transform = subtitleTransformMatrix(item_transform_state);
            const D2D1_RECT_F item_clip_rect = D2D1::RectF(
                static_cast<float>(video_rect.x) + static_cast<float>(item_style.clip_x1) * script_scale_x,
                static_cast<float>(video_rect.y) + static_cast<float>(item_style.clip_y1) * script_scale_y,
                static_cast<float>(video_rect.x) + static_cast<float>(item_style.clip_x2) * script_scale_x,
                static_cast<float>(video_rect.y) + static_cast<float>(item_style.clip_y2) * script_scale_y);
            const std::vector<D2D1_RECT_F> item_clip_regions = item_style.has_clip
                ? buildSubtitleClipRegions(video_rect, item_clip_rect, item_style.inverse_clip)
                : std::vector<D2D1_RECT_F>{};
            const bool has_rect_item_clip = !item_clip_regions.empty();
            if (item_style.has_clip && !has_rect_item_clip) {
                continue;
            }

            ComPtr<ID2D1PathGeometry> vector_clip_path;
            ComPtr<ID2D1PathGeometry> inverse_clip_path;
            ID2D1Geometry* vector_clip_geometry = nullptr;
            const bool wants_vector_clip = item_style.has_vector_clip && !item_style.vector_clip_commands.empty();
            if (wants_vector_clip) {
                if (!buildAssClipGeometry(subtitle_d2d_factory_.Get(),
                                          item_style.vector_clip_commands,
                                          item_style.vector_clip_scale,
                                          video_rect,
                                          script_scale_x,
                                          script_scale_y,
                                          vector_clip_path.GetAddressOf()) ||
                    !vector_clip_path) {
                    continue;
                }
                if (item_style.inverse_clip) {
                    if (!buildInverseClipGeometry(subtitle_d2d_factory_.Get(),
                                                  vector_clip_path.Get(),
                                                  video_rect,
                                                  inverse_clip_path.GetAddressOf()) ||
                        !inverse_clip_path) {
                        continue;
                    }
                    vector_clip_geometry = inverse_clip_path.Get();
                } else {
                    vector_clip_geometry = vector_clip_path.Get();
                }
            }
            const bool has_vector_item_clip = vector_clip_geometry != nullptr;
            ComPtr<ID2D1Layer> item_clip_layer;
            if (has_vector_item_clip) {
                const HRESULT clip_hr = subtitle_d2d_target_->CreateLayer(item_clip_layer.GetAddressOf());
                if (FAILED(clip_hr) || !item_clip_layer) {
                    continue;
                }
            }

            const auto push_vector_clip = [&]() {
                if (!has_vector_item_clip) {
                    return;
                }
                subtitle_d2d_target_->PushLayer(D2D1::LayerParameters(D2D1::InfiniteRect(), vector_clip_geometry), item_clip_layer.Get());
            };
            const auto pop_vector_clip = [&]() {
                if (has_vector_item_clip) {
                    subtitle_d2d_target_->PopLayer();
                }
            };

            ComPtr<ID2D1PathGeometry> drawing_geometry;
            if (!buildAssVectorGeometry(subtitle_d2d_factory_.Get(), drawing_shape, draw_x, draw_y, drawing_geometry.GetAddressOf()) || !drawing_geometry) {
                continue;
            }
            ComPtr<ID2D1PathGeometry> shadow_geometry;
            const bool has_shadow = std::abs(shadow_offset_x) > 0.001f || std::abs(shadow_offset_y) > 0.001f;
            if (has_shadow &&
                (!buildAssVectorGeometry(subtitle_d2d_factory_.Get(),
                                         drawing_shape,
                                         draw_x + shadow_offset_x,
                                         draw_y + shadow_offset_y,
                                         shadow_geometry.GetAddressOf()) ||
                 !shadow_geometry)) {
                continue;
            }

            const auto create_color_brush = [&](const subtitle::SubtitleColor& color,
                                                ComPtr<ID2D1SolidColorBrush>& out_brush) -> bool {
                return SUCCEEDED(subtitle_d2d_target_->CreateSolidColorBrush(
                                     toD2DColor(applySubtitleOpacity(color, animation_state.opacity)),
                                     out_brush.GetAddressOf())) &&
                       out_brush;
            };

            const subtitle::SubtitleColor shadow_color = item_style.background_color.a > 0
                ? item_style.background_color
                : subtitle::SubtitleColor(0, 0, 0, 184);
            ComPtr<ID2D1SolidColorBrush> fill_brush;
            ComPtr<ID2D1SolidColorBrush> outline_brush;
            ComPtr<ID2D1SolidColorBrush> shadow_brush;
            if (!create_color_brush(item_style.primary_color, fill_brush) ||
                !create_color_brush(item_style.outline_color, outline_brush) ||
                !create_color_brush(shadow_color, shadow_brush)) {
                continue;
            }

            const float outline_stroke_width = std::max(1.0f, std::max(outline_radius_x, outline_radius_y));
            const auto draw_geometry_region = [&](ID2D1Geometry* geometry,
                                                  ID2D1Brush* brush,
                                                  bool fill_geometry,
                                                  const D2D1_RECT_F* item_clip_region) {
                subtitle_d2d_target_->SetTransform(D2D1::Matrix3x2F::Identity());
                if (item_clip_region) {
                    subtitle_d2d_target_->PushAxisAlignedClip(*item_clip_region, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
                }
                push_vector_clip();
                subtitle_d2d_target_->SetTransform(item_transform);
                if (fill_geometry) {
                    subtitle_d2d_target_->FillGeometry(geometry, brush);
                } else {
                    subtitle_d2d_target_->DrawGeometry(geometry, brush, outline_stroke_width);
                }
                subtitle_d2d_target_->SetTransform(D2D1::Matrix3x2F::Identity());
                pop_vector_clip();
                if (item_clip_region) {
                    subtitle_d2d_target_->PopAxisAlignedClip();
                }
            };

            const auto draw_geometry_all_regions = [&](ID2D1Geometry* geometry,
                                                       ID2D1Brush* brush,
                                                       bool fill_geometry) {
                if (has_rect_item_clip) {
                    for (const D2D1_RECT_F& item_clip_region : item_clip_regions) {
                        draw_geometry_region(geometry, brush, fill_geometry, &item_clip_region);
                    }
                } else {
                    draw_geometry_region(geometry, brush, fill_geometry, nullptr);
                }
            };

            if (has_shadow && shadow_geometry) {
                draw_geometry_all_regions(shadow_geometry.Get(), shadow_brush.Get(), true);
                if (outline_stroke_width > 0.0f) {
                    draw_geometry_all_regions(shadow_geometry.Get(), shadow_brush.Get(), false);
                }
            }
            if (outline_stroke_width > 0.0f) {
                draw_geometry_all_regions(drawing_geometry.Get(), outline_brush.Get(), false);
            }
            draw_geometry_all_regions(drawing_geometry.Get(), fill_brush.Get(), true);
            continue;
        }

        if (item.is_bitmap) {
            if (item.bitmap_rects.empty()) {
                continue;
            }

            const float scale_x = item.play_res_x > 0
                ? static_cast<float>(video_rect.w) / static_cast<float>(item.play_res_x)
                : 1.0f;
            const float scale_y = item.play_res_y > 0
                ? static_cast<float>(video_rect.h) / static_cast<float>(item.play_res_y)
                : 1.0f;
            subtitle_d2d_target_->SetTransform(D2D1::Matrix3x2F::Identity());
            for (const subtitle::SubtitleBitmap& bitmap : item.bitmap_rects) {
                if (bitmap.width <= 0 || bitmap.height <= 0) {
                    continue;
                }

                ID2D1Bitmap* d2d_bitmap = getCachedSubtitleBitmap(bitmap);
                if (!d2d_bitmap) {
                    continue;
                }

                const float dst_left = static_cast<float>(video_rect.x) + static_cast<float>(bitmap.x) * scale_x;
                const float dst_top = static_cast<float>(video_rect.y) + static_cast<float>(bitmap.y) * scale_y;
                const float dst_right = dst_left + static_cast<float>(bitmap.width) * scale_x;
                const float dst_bottom = dst_top + static_cast<float>(bitmap.height) * scale_y;
                if (dst_right <= dst_left + 0.5f || dst_bottom <= dst_top + 0.5f) {
                    continue;
                }

                const D2D1_RECT_F src_rect = D2D1::RectF(0.0f,
                                                         0.0f,
                                                         static_cast<float>(bitmap.width),
                                                         static_cast<float>(bitmap.height));
                const D2D1_RECT_F dst_rect = D2D1::RectF(dst_left, dst_top, dst_right, dst_bottom);
                subtitle_d2d_target_->DrawBitmap(d2d_bitmap,
                                                 &dst_rect,
                                                 1.0f,
                                                 D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,
                                                 &src_rect);
            }
            continue;
        }

        if (item.text.empty()) {
            continue;
        }

        const std::wstring text_wide = utf8ToWide(item.text);
        if (text_wide.empty()) {
            continue;
        }

        const int alignment = clampSubtitleAlignment(item_style.alignment);
        const float script_scale_x = item.play_res_x > 0 ? static_cast<float>(video_rect.w) / static_cast<float>(item.play_res_x) : 1.0f;
        const float script_scale_y = item.play_res_y > 0 ? static_cast<float>(video_rect.h) / static_cast<float>(item.play_res_y) : 1.0f;
        const SubtitleAnimationState animation_state = resolveSubtitleAnimationState(item, subtitle_clock_seconds);
        if (animation_state.opacity <= 0.001f) {
            continue;
        }
        const bool has_anchor_position = animation_state.has_position || item_style.has_position;
        const float glyph_scale_x = subtitlePercentScale(item_style.scale_x_percent);
        const float glyph_scale_y = subtitlePercentScale(item_style.scale_y_percent);
        const float margin_left = item.play_res_x > 0
            ? std::max(4.0f, static_cast<float>(item_style.margin_l) * script_scale_x)
            : std::max(fallback_margin_x, static_cast<float>(item_style.margin_l));
        const float margin_right = item.play_res_x > 0
            ? std::max(4.0f, static_cast<float>(item_style.margin_r) * script_scale_x)
            : std::max(fallback_margin_x, static_cast<float>(item_style.margin_r));

        float font_size = fallback_font_size;
        if (item.play_res_y > 0 && item_style.font_size > 0.0) {
            font_size = static_cast<float>(item_style.font_size) * script_scale_y;
        } else if (item_style.font_size > 0.0) {
            font_size = std::max(fallback_font_size, static_cast<float>(item_style.font_size));
        }
        font_size = std::clamp(font_size, 10.0f, std::max(36.0f, static_cast<float>(video_rect.h) * 0.35f));

        const float available_width = std::max(48.0f, static_cast<float>(video_rect.w) - margin_left - margin_right);
        const float layout_height = std::max(24.0f, available_height / glyph_scale_y);
        float layout_width = std::max(24.0f, available_width / glyph_scale_x);
        if (has_anchor_position) {
            layout_width = std::min(layout_width, std::max(font_size * 8.0f, (available_width * 0.45f) / glyph_scale_x));
        }

        subtitle::ensureSubtitleFontsRegistered(item.source_path);

        ComPtr<IDWriteTextFormat> text_format;
        const auto create_text_format = [&](const wchar_t* font_family) -> HRESULT {
            HRESULT hr = E_FAIL;
            if (subtitle_dwrite_font_collection_) {
                text_format.Reset();
                hr = subtitle_dwrite_factory_->CreateTextFormat(font_family,
                                                                subtitle_dwrite_font_collection_.Get(),
                                                                item_style.bold ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL,
                                                                item_style.italic ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL,
                                                                DWRITE_FONT_STRETCH_NORMAL,
                                                                font_size,
                                                                L"zh-CN",
                                                                text_format.GetAddressOf());
                if (SUCCEEDED(hr) && text_format) {
                    return hr;
                }
            }
            text_format.Reset();
            return subtitle_dwrite_factory_->CreateTextFormat(font_family,
                                                              nullptr,
                                                              item_style.bold ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL,
                                                              item_style.italic ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL,
                                                              DWRITE_FONT_STRETCH_NORMAL,
                                                              font_size,
                                                              L"zh-CN",
                                                              text_format.GetAddressOf());
        };

        HRESULT format_hr = E_FAIL;
        for (const std::wstring& candidate_family : subtitle::buildSubtitleFontFallbackFamilies(item_style.font_family)) {
            if (candidate_family.empty()) {
                continue;
            }
            text_format.Reset();
            format_hr = create_text_format(candidate_family.c_str());
            if (SUCCEEDED(format_hr) && text_format) {
                break;
            }
        }
        if (FAILED(format_hr) || !text_format) {
            continue;
        }

        text_format->SetTextAlignment(toTextAlignment(alignment));
        text_format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
        text_format->SetWordWrapping(toWordWrapping(item_style.wrap_style));

        ComPtr<IDWriteTextLayout> text_layout;
        hr = subtitle_dwrite_factory_->CreateTextLayout(text_wide.c_str(),
                                                        static_cast<UINT32>(text_wide.size()),
                                                        text_format.Get(),
                                                        layout_width,
                                                        layout_height,
                                                        text_layout.GetAddressOf());
        if (FAILED(hr) || !text_layout) {
            continue;
        }

        const DWRITE_TEXT_RANGE full_range{0, static_cast<UINT32>(text_wide.size())};
        text_layout->SetUnderline(item_style.underline, full_range);
        text_layout->SetStrikethrough(item_style.strikeout, full_range);
        ComPtr<IDWriteTextLayout1> text_layout1;
        const bool has_text_layout1 = SUCCEEDED(text_layout.As(&text_layout1)) && text_layout1;
        if (has_text_layout1) {
            const float display_spacing = item.play_res_x > 0
                ? static_cast<float>(item_style.spacing) * script_scale_x
                : static_cast<float>(item_style.spacing);
            const float layout_spacing = display_spacing / glyph_scale_x;
            text_layout1->SetCharacterSpacing(layout_spacing * 0.5f, layout_spacing * 0.5f, 0.0f, full_range);
        }

        for (const subtitle::SubtitleTextRun& run : item.runs) {
            const subtitle::SubtitleStyle run_style = resolveAnimatedSubtitleStyle(run.style,
                                                                                   item.animation,
                                                                                   item_elapsed_seconds,
                                                                                   item_duration_seconds);
            if (run.length == 0) {
                continue;
            }
            const UINT32 start_index = static_cast<UINT32>(std::min(run.start, static_cast<size_t>(text_wide.size())));
            const UINT32 length = static_cast<UINT32>(std::min(run.length, static_cast<size_t>(text_wide.size()) - start_index));
            if (length == 0) {
                continue;
            }
            const DWRITE_TEXT_RANGE range{start_index, length};
            for (const std::wstring& candidate_family : subtitle::buildSubtitleFontFallbackFamilies(run_style.font_family)) {
                if (!candidate_family.empty() && SUCCEEDED(text_layout->SetFontFamilyName(candidate_family.c_str(), range))) {
                    break;
                }
            }
            float run_font_size = font_size;
            if (run_style.font_size > 0.0) {
                run_font_size = item.play_res_y > 0 ? static_cast<float>(run_style.font_size) * script_scale_y : static_cast<float>(run_style.font_size);
                run_font_size = std::clamp(run_font_size, 10.0f, std::max(36.0f, static_cast<float>(video_rect.h) * 0.35f));
            }
            text_layout->SetFontSize(run_font_size, range);
            text_layout->SetFontWeight(run_style.bold ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL, range);
            text_layout->SetFontStyle(run_style.italic ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL, range);
            text_layout->SetUnderline(run_style.underline, range);
            text_layout->SetStrikethrough(run_style.strikeout, range);
            if (has_text_layout1 && std::abs(run_style.spacing - item_style.spacing) > 0.001) {
                const float run_display_spacing = item.play_res_x > 0
                    ? static_cast<float>(run_style.spacing) * script_scale_x
                    : static_cast<float>(run_style.spacing);
                const float run_layout_spacing = run_display_spacing / glyph_scale_x;
                text_layout1->SetCharacterSpacing(run_layout_spacing * 0.5f,
                                                  run_layout_spacing * 0.5f,
                                                  0.0f,
                                                  range);
            }
        }

        DWRITE_TEXT_METRICS metrics{};
        if (FAILED(text_layout->GetMetrics(&metrics))) {
            continue;
        }

        const float text_height = std::max(1.0f, metrics.height);
        const float anchor_local_x = subtitleAnchorLocalX(layout_width, alignment);
        const float anchor_local_y = subtitleAnchorLocalY(text_height, alignment);
        subtitle::SubtitleStyle preview_style = item_style;
        preview_style.has_rotation_origin = false;
        const SubtitleAffineTransform preview_transform = buildSubtitleAffineTransform(
            preview_style,
            glyph_scale_x,
            glyph_scale_y,
            script_scale_x,
            script_scale_y,
            video_rect,
            D2D1::Point2F(anchor_local_x, anchor_local_y));
        const SubtitleLocalBounds local_bounds = computeTransformedSubtitleBounds(
            metrics.left,
            metrics.top,
            metrics.left + std::max(1.0f, metrics.widthIncludingTrailingWhitespace),
            metrics.top + text_height,
            preview_transform);
        const float transformed_height = std::max(1.0f, local_bounds.height());
        const float gap = std::max(6.0f, font_size * 0.22f * glyph_scale_y);

        float draw_x = static_cast<float>(video_rect.x) + margin_left;
        if (animation_state.has_position) {
            const float anchor_x = static_cast<float>(video_rect.x) + static_cast<float>(animation_state.position_x) * script_scale_x;
            draw_x = anchor_x - anchor_local_x;
        } else {
            switch (subtitleHorizontalGroup(alignment)) {
            case 2:
                draw_x = static_cast<float>(video_rect.x + video_rect.w) - margin_right - layout_width;
                break;
            case 1:
                draw_x = static_cast<float>(video_rect.x) + (static_cast<float>(video_rect.w) - layout_width) * 0.5f;
                break;
            default:
                draw_x = static_cast<float>(video_rect.x) + margin_left;
                break;
            }
        }
        draw_x = clampSubtitleCoordinate(draw_x,
                                         static_cast<float>(video_rect.x) - local_bounds.left,
                                         static_cast<float>(video_rect.x + video_rect.w) - local_bounds.right);

        float draw_y = top_limit;
        if (animation_state.has_position) {
            const float anchor_y = static_cast<float>(video_rect.y) + static_cast<float>(animation_state.position_y) * script_scale_y;
            draw_y = anchor_y - anchor_local_y;
        } else {
            switch (subtitleVerticalGroup(alignment)) {
            case 2:
                draw_y = next_top_y - local_bounds.top;
                next_top_y += transformed_height + gap;
                break;
            case 1:
                draw_y = next_middle_y - local_bounds.centerY();
                next_middle_y += transformed_height + gap;
                break;
            default:
                draw_y = next_bottom_y - local_bounds.bottom;
                next_bottom_y = draw_y + local_bounds.top - gap;
                break;
            }
        }
        draw_y = clampSubtitleCoordinate(draw_y, top_limit - local_bounds.top, bottom_limit - local_bounds.bottom);

        const float actual_left = draw_x + metrics.left;
        const float actual_top = draw_y + metrics.top;
        const bool draw_box = item_style.border_style == 3;
        const float outline_radius_x = draw_box ? 0.0f : std::max(0.0f, static_cast<float>(item_style.outline_x) * script_scale_x);
        const float outline_radius_y = draw_box ? 0.0f : std::max(0.0f, static_cast<float>(item_style.outline_y) * script_scale_y);
        const float shadow_offset_x = static_cast<float>(item_style.shadow_x) * script_scale_x;
        const float shadow_offset_y = static_cast<float>(item_style.shadow_y) * script_scale_y;
        const subtitle::SubtitleColor box_color_source = item_style.background_color.a > 0
            ? item_style.background_color
            : subtitle::SubtitleColor(5, 5, 5, 148);
        const D2D1_COLOR_F box_color = toD2DColor(applySubtitleOpacity(box_color_source, animation_state.opacity));
        const D2D1_POINT_2F anchor_point = D2D1::Point2F(draw_x + anchor_local_x, draw_y + anchor_local_y);
        const SubtitleAffineTransform item_transform_state = buildSubtitleAffineTransform(
            item_style,
            glyph_scale_x,
            glyph_scale_y,
            script_scale_x,
            script_scale_y,
            video_rect,
            anchor_point);
        const D2D1_MATRIX_3X2_F item_transform = subtitleTransformMatrix(item_transform_state);
        const D2D1_RECT_F item_clip_rect = D2D1::RectF(
            static_cast<float>(video_rect.x) + static_cast<float>(item_style.clip_x1) * script_scale_x,
            static_cast<float>(video_rect.y) + static_cast<float>(item_style.clip_y1) * script_scale_y,
            static_cast<float>(video_rect.x) + static_cast<float>(item_style.clip_x2) * script_scale_x,
            static_cast<float>(video_rect.y) + static_cast<float>(item_style.clip_y2) * script_scale_y);
        const std::vector<D2D1_RECT_F> item_clip_regions = item_style.has_clip
            ? buildSubtitleClipRegions(video_rect, item_clip_rect, item_style.inverse_clip)
            : std::vector<D2D1_RECT_F>{};
        const bool has_rect_item_clip = !item_clip_regions.empty();
        if (item_style.has_clip && !has_rect_item_clip) {
            continue;
        }

        ComPtr<ID2D1PathGeometry> vector_clip_path;
        ComPtr<ID2D1PathGeometry> inverse_clip_path;
        ID2D1Geometry* vector_clip_geometry = nullptr;
        const bool wants_vector_clip = item_style.has_vector_clip && !item_style.vector_clip_commands.empty();
        if (wants_vector_clip) {
            if (!buildAssClipGeometry(subtitle_d2d_factory_.Get(),
                                      item_style.vector_clip_commands,
                                      item_style.vector_clip_scale,
                                      video_rect,
                                      script_scale_x,
                                      script_scale_y,
                                      vector_clip_path.GetAddressOf()) ||
                !vector_clip_path) {
                continue;
            }
            if (item_style.inverse_clip) {
                if (!buildInverseClipGeometry(subtitle_d2d_factory_.Get(),
                                              vector_clip_path.Get(),
                                              video_rect,
                                              inverse_clip_path.GetAddressOf()) ||
                    !inverse_clip_path) {
                    continue;
                }
                vector_clip_geometry = inverse_clip_path.Get();
            } else {
                vector_clip_geometry = vector_clip_path.Get();
            }
        }
        const bool has_vector_item_clip = vector_clip_geometry != nullptr;
        ComPtr<ID2D1Layer> item_clip_layer;
        if (has_vector_item_clip) {
            const HRESULT clip_hr = subtitle_d2d_target_->CreateLayer(item_clip_layer.GetAddressOf());
            if (FAILED(clip_hr) || !item_clip_layer) {
                continue;
            }
        }

        const auto push_vector_clip = [&]() {
            if (!has_vector_item_clip) {
                return;
            }
            subtitle_d2d_target_->PushLayer(D2D1::LayerParameters(D2D1::InfiniteRect(), vector_clip_geometry), item_clip_layer.Get());
        };

        const auto pop_vector_clip = [&]() {
            if (has_vector_item_clip) {
                subtitle_d2d_target_->PopLayer();
            }
        };

        ComPtr<ID2D1SolidColorBrush> transparent_brush;
        hr = subtitle_d2d_target_->CreateSolidColorBrush(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f),
                                                         transparent_brush.GetAddressOf());
        if (FAILED(hr) || !transparent_brush) {
            continue;
        }

        struct RunBrushPass {
            DWRITE_TEXT_RANGE range{};
            ComPtr<ID2D1SolidColorBrush> fill_brush;
            ComPtr<ID2D1SolidColorBrush> outline_brush;
            ComPtr<ID2D1SolidColorBrush> shadow_brush;
            ComPtr<ID2D1SolidColorBrush> highlight_brush;
            std::vector<KaraokeHighlightClip> highlight_clips;
        };

        std::vector<RunBrushPass> run_passes;
        run_passes.reserve(std::max<size_t>(1, item.runs.size()));

        const auto create_color_brush = [&](const subtitle::SubtitleColor& color,
                                            ComPtr<ID2D1SolidColorBrush>& out_brush) -> bool {
            return SUCCEEDED(subtitle_d2d_target_->CreateSolidColorBrush(
                                 toD2DColor(applySubtitleOpacity(color, animation_state.opacity)),
                                 out_brush.GetAddressOf())) &&
                   out_brush;
        };

        const auto append_run_pass = [&](const subtitle::SubtitleTextRun& run, UINT32 start_index, UINT32 length) -> bool {
            RunBrushPass pass{};
            pass.range = DWRITE_TEXT_RANGE{start_index, length};
            const subtitle::SubtitleStyle run_style = resolveAnimatedSubtitleStyle(run.style,
                                                                                   item.animation,
                                                                                   item_elapsed_seconds,
                                                                                   item_duration_seconds);

            const KaraokeRunVisualState karaoke_state =
                resolveKaraokeRunVisualState(run_style, run, item_elapsed_seconds);
            const subtitle::SubtitleColor shadow_color = run_style.background_color.a > 0
                ? run_style.background_color
                : subtitle::SubtitleColor(0, 0, 0, 184);
            if (!create_color_brush(karaoke_state.base_fill_color, pass.fill_brush) ||
                !create_color_brush(run_style.outline_color, pass.outline_brush) ||
                !create_color_brush(shadow_color, pass.shadow_brush)) {
                return false;
            }

            if (karaoke_state.has_highlight_overlay &&
                create_color_brush(karaoke_state.overlay_fill_color, pass.highlight_brush)) {
                std::vector<DWRITE_HIT_TEST_METRICS> hit_metrics;
                if (collectTextRangeMetrics(text_layout.Get(), start_index, length, hit_metrics)) {
                    pass.highlight_clips = buildKaraokeHighlightClips(hit_metrics,
                                                                      karaoke_state.overlay_progress,
                                                                      draw_x,
                                                                      draw_y,
                                                                      item_transform_state);
                }
            }

            run_passes.push_back(std::move(pass));
            return true;
        };

        if (!item.runs.empty()) {
            for (const subtitle::SubtitleTextRun& run : item.runs) {
                if (run.length == 0) {
                    continue;
                }
                const UINT32 start_index =
                    static_cast<UINT32>(std::min(run.start, static_cast<size_t>(text_wide.size())));
                const UINT32 length = static_cast<UINT32>(
                    std::min(run.length, static_cast<size_t>(text_wide.size()) - start_index));
                if (length == 0 || !append_run_pass(run, start_index, length)) {
                    run_passes.clear();
                    break;
                }
            }
        }
        if (run_passes.empty()) {
            subtitle::SubtitleTextRun fallback_run{};
            fallback_run.start = 0;
            fallback_run.length = text_wide.size();
            fallback_run.style = item_style;
            if (!append_run_pass(fallback_run, 0, static_cast<UINT32>(text_wide.size()))) {
                continue;
            }
        }

        enum class TextPassKind { Fill, Outline, Shadow, Highlight };
        const auto apply_pass_effects = [&](TextPassKind kind) {
            for (const RunBrushPass& pass : run_passes) {
                IUnknown* effect = nullptr;
                switch (kind) {
                case TextPassKind::Fill:
                    effect = pass.fill_brush.Get();
                    break;
                case TextPassKind::Outline:
                    effect = pass.outline_brush.Get();
                    break;
                case TextPassKind::Shadow:
                    effect = pass.shadow_brush.Get();
                    break;
                case TextPassKind::Highlight:
                    effect = pass.highlight_brush ? pass.highlight_brush.Get() : transparent_brush.Get();
                    break;
                }
                text_layout->SetDrawingEffect(effect, pass.range);
            }
        };

        const auto draw_text_pass = [&](float pass_offset_x,
                                        float pass_offset_y,
                                        const std::vector<KaraokeHighlightClip>* clips) {
            const auto draw_with_item_clip = [&](const D2D1_RECT_F* item_clip_region) {
                subtitle_d2d_target_->SetTransform(D2D1::Matrix3x2F::Identity());
                if (item_clip_region) {
                    subtitle_d2d_target_->PushAxisAlignedClip(*item_clip_region, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
                }

                push_vector_clip();

                if (clips && !clips->empty()) {
                    for (const KaraokeHighlightClip& clip : *clips) {
                        subtitle_d2d_target_->PushAxisAlignedClip(clip.world_rect, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
                        subtitle_d2d_target_->SetTransform(item_transform);
                        subtitle_d2d_target_->DrawTextLayout(D2D1::Point2F(draw_x + pass_offset_x, draw_y + pass_offset_y),
                                                             text_layout.Get(),
                                                             transparent_brush.Get());
                        subtitle_d2d_target_->SetTransform(D2D1::Matrix3x2F::Identity());
                        subtitle_d2d_target_->PopAxisAlignedClip();
                    }
                } else {
                    subtitle_d2d_target_->SetTransform(item_transform);
                    subtitle_d2d_target_->DrawTextLayout(D2D1::Point2F(draw_x + pass_offset_x, draw_y + pass_offset_y),
                                                         text_layout.Get(),
                                                         transparent_brush.Get());
                    subtitle_d2d_target_->SetTransform(D2D1::Matrix3x2F::Identity());
                }

                pop_vector_clip();
                if (item_clip_region) {
                    subtitle_d2d_target_->PopAxisAlignedClip();
                }
            };

            if (has_rect_item_clip) {
                for (const D2D1_RECT_F& item_clip_region : item_clip_regions) {
                    draw_with_item_clip(&item_clip_region);
                }
                return;
            }
            draw_with_item_clip(nullptr);
        };

        if (draw_box) {
            const auto draw_box_region = [&](const D2D1_RECT_F* item_clip_region) {
                subtitle_d2d_target_->SetTransform(D2D1::Matrix3x2F::Identity());
                if (item_clip_region) {
                    subtitle_d2d_target_->PushAxisAlignedClip(*item_clip_region, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
                }
                push_vector_clip();
                subtitle_d2d_target_->SetTransform(item_transform);
                const D2D1_ROUNDED_RECT background = D2D1::RoundedRect(
                    D2D1::RectF(std::max(static_cast<float>(video_rect.x), actual_left - kSubtitleHorizontalPadding),
                                std::max(top_limit, actual_top - kSubtitleVerticalPadding),
                                std::min(static_cast<float>(video_rect.x + video_rect.w),
                                         actual_left + std::max(1.0f, metrics.widthIncludingTrailingWhitespace) + kSubtitleHorizontalPadding),
                                std::min(bottom_limit,
                                         actual_top + text_height + kSubtitleVerticalPadding)),
                    8.0f,
                    8.0f);
                subtitle_background_brush_->SetColor(box_color);
                subtitle_d2d_target_->FillRoundedRectangle(&background, subtitle_background_brush_.Get());
                subtitle_d2d_target_->SetTransform(D2D1::Matrix3x2F::Identity());
                pop_vector_clip();
                if (item_clip_region) {
                    subtitle_d2d_target_->PopAxisAlignedClip();
                }
            };

            if (has_rect_item_clip) {
                for (const D2D1_RECT_F& item_clip_region : item_clip_regions) {
                    draw_box_region(&item_clip_region);
                }
            } else {
                draw_box_region(nullptr);
            }
        }

        if (outline_radius_x > 0.0f || outline_radius_y > 0.0f) {
            static const float kOutlineOffsets[8][2] = {
                {-1.0f, 0.0f},
                {1.0f, 0.0f},
                {0.0f, -1.0f},
                {0.0f, 1.0f},
                {-1.0f, -1.0f},
                {1.0f, -1.0f},
                {-1.0f, 1.0f},
                {1.0f, 1.0f},
            };
            apply_pass_effects(TextPassKind::Outline);
            for (const auto& offset : kOutlineOffsets) {
                draw_text_pass(offset[0] * outline_radius_x,
                               offset[1] * outline_radius_y,
                               nullptr);
            }
        }

        if (std::abs(shadow_offset_x) > 0.001f || std::abs(shadow_offset_y) > 0.001f) {
            apply_pass_effects(TextPassKind::Shadow);
            draw_text_pass(shadow_offset_x, shadow_offset_y, nullptr);
        }

        apply_pass_effects(TextPassKind::Fill);
        draw_text_pass(0.0f, 0.0f, nullptr);

        apply_pass_effects(TextPassKind::Highlight);
        for (const RunBrushPass& pass : run_passes) {
            if (!pass.highlight_brush || pass.highlight_clips.empty()) {
                continue;
            }
            draw_text_pass(0.0f, 0.0f, &pass.highlight_clips);
        }
    }

    hr = subtitle_d2d_target_->EndDraw();
    if (hr == D2DERR_RECREATE_TARGET) {
        resetSubtitleTextResources();
        destroyGdiSurface(surface);
        return false;
    }
    if (FAILED(hr)) {
        LOG_WARNING("OpenGL subtitle D2D draw failed: hr=" << static_cast<long>(hr));
        destroyGdiSurface(surface);
        return false;
    }

    const size_t pixel_bytes = static_cast<size_t>(drawable_width) * static_cast<size_t>(drawable_height) * 4u;
    subtitle_pixels_.resize(pixel_bytes);
    std::memcpy(subtitle_pixels_.data(), surface.bits, pixel_bytes);
    destroyGdiSurface(surface);
    return true;
}
#endif

bool OpenGLVideoRenderer::Impl::updateSubtitleTextureIfNeeded(const PendingVideoFrame* frame, int drawable_width, int drawable_height) {
    const SDL_Rect video_rect = frame && frame->valid
        ? computeRenderRect(drawable_width, drawable_height, frame->width, frame->height)
        : SDL_Rect{0, 0, drawable_width, drawable_height};
    const uint64_t generation = subtitle_generation_.load();
    const double subtitle_clock_seconds = subtitle_clock_seconds_.load();
    const bool animated_subtitles = subtitle_has_animated_content_.load();
    if (generation == rendered_subtitle_generation_ &&
        drawable_width == subtitle_cached_drawable_width_ &&
        drawable_height == subtitle_cached_drawable_height_ &&
        subtitle_cached_video_rect_.x == video_rect.x && subtitle_cached_video_rect_.y == video_rect.y &&
        subtitle_cached_video_rect_.w == video_rect.w && subtitle_cached_video_rect_.h == video_rect.h &&
        (!animated_subtitles || std::abs(subtitle_clock_seconds - rendered_subtitle_clock_seconds_) <= 0.0005)) {
        return true;
    }

    subtitle_texture_valid_ = false;
    const SubtitleSnapshot subtitle_state = snapshotSubtitleState();
    if (subtitle_state.items.empty()) {
        subtitle_cached_drawable_width_ = drawable_width;
        subtitle_cached_drawable_height_ = drawable_height;
        subtitle_cached_video_rect_ = video_rect;
        rendered_subtitle_generation_ = generation;
        rendered_subtitle_clock_seconds_ = subtitle_clock_seconds;
        return true;
    }

#if defined(_WIN32)
    if (!renderSubtitleItemsWithD2D(subtitle_state.items, frame, drawable_width, drawable_height, video_rect)) {
        return false;
    }

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
    if (!subtitle_texture_valid_) {
        return false;
    }

    subtitle_cached_drawable_width_ = drawable_width;
    subtitle_cached_drawable_height_ = drawable_height;
    subtitle_cached_video_rect_ = video_rect;
    rendered_subtitle_generation_ = generation;
    rendered_subtitle_clock_seconds_ = subtitle_clock_seconds;
    return true;
#else
    (void)subtitle_state;
    subtitle_cached_drawable_width_ = drawable_width;
    subtitle_cached_drawable_height_ = drawable_height;
    subtitle_cached_video_rect_ = video_rect;
    rendered_subtitle_generation_ = generation;
    rendered_subtitle_clock_seconds_ = subtitle_clock_seconds;
    subtitle_texture_valid_ = false;
    return true;
#endif
}

void OpenGLVideoRenderer::Impl::drawSubtitleOverlay(const PendingVideoFrame* frame, int drawable_width, int drawable_height) {
    if (!updateSubtitleTextureIfNeeded(frame, drawable_width, drawable_height) || !subtitle_texture_valid_ || subtitle_texture_ == 0) return;
    g_gl.UseProgram(0);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
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

OpenGLVideoRenderer::Impl::ControlLayout OpenGLVideoRenderer::Impl::computeControlLayout(int drawable_width, int drawable_height) const {
    const int safe_width = std::max(1, drawable_width);
    const int safe_height = std::max(1, drawable_height);
    ControlLayout layout{};
    layout.panel = {kControlPanelInset, std::max(0, safe_height - kControlPanelHeight - kControlPanelInset),
                    std::max(1, safe_width - (kControlPanelInset * 2)), kControlPanelHeight};
    const int progress_y = layout.panel.y + kControlPadding;
    layout.progress_track = {layout.panel.x + kControlPadding,
                             progress_y,
                             std::max(kMinProgressBarWidth, layout.panel.w - (kControlPadding * 2)),
                             kBarHeight};
    layout.progress_hit_box = {layout.progress_track.x,
                               std::max(layout.panel.y, layout.progress_track.y - 8),
                               layout.progress_track.w,
                               std::min(layout.panel.h, layout.progress_track.h + 16)};

    const int row_top = layout.progress_track.y + layout.progress_track.h + 12;
    const int row_height = std::max(28, layout.panel.y + layout.panel.h - kControlPadding - row_top);
    const int button_size = std::max(28, std::min(36, row_height));
    const int row_y = row_top + std::max(0, (row_height - button_size) / 2);
    layout.previous_item_button = {layout.panel.x + kControlPadding, row_y, button_size, button_size};
    layout.play_button = {layout.previous_item_button.x + layout.previous_item_button.w + kControlGap,
                          row_y,
                          button_size,
                          button_size};
    layout.next_item_button = {layout.play_button.x + layout.play_button.w + kControlGap,
                               row_y,
                               button_size,
                               button_size};

    const int volume_width = std::min(kVolumeBarWidth, std::max(72, layout.panel.w / 6));
    layout.fullscreen_button = {layout.panel.x + layout.panel.w - kControlPadding - button_size,
                                row_y,
                                button_size,
                                button_size};
    layout.volume_track = {layout.fullscreen_button.x - kControlGap - volume_width,
                           layout.play_button.y + (layout.play_button.h - kBarHeight) / 2,
                           std::max(1, volume_width),
                           kBarHeight};
    layout.mute_button = {layout.volume_track.x - kControlGap - button_size, row_y, button_size, button_size};
    layout.volume_hit_box = {layout.volume_track.x,
                             layout.play_button.y,
                             layout.volume_track.w,
                             layout.play_button.h};

    const int subtitle_text_width = std::max(48, std::min(96, layout.panel.w / 7));
    layout.subtitle_next_button = {layout.mute_button.x - kControlGap - button_size, row_y, button_size, button_size};
    layout.subtitle_text = {layout.subtitle_next_button.x - kControlGap - subtitle_text_width,
                            row_y,
                            subtitle_text_width,
                            button_size};
    layout.subtitle_previous_button = {layout.subtitle_text.x - kControlGap - button_size,
                                       row_y,
                                       button_size,
                                       button_size};

    const int time_x = layout.next_item_button.x + layout.next_item_button.w + kControlGap;
    const int min_time_width = 48;
    if (layout.subtitle_previous_button.x - kControlGap - time_x < min_time_width) {
        const int subtitle_group_width =
            button_size + kControlGap + subtitle_text_width + kControlGap + button_size;
        const int min_subtitle_start = time_x + min_time_width + kControlGap;
        const int max_subtitle_start = layout.mute_button.x - kControlGap - subtitle_group_width;
        const int subtitle_group_start = std::max(min_subtitle_start, max_subtitle_start);
        layout.subtitle_previous_button.x = subtitle_group_start;
        layout.subtitle_text.x = layout.subtitle_previous_button.x + button_size + kControlGap;
        layout.subtitle_next_button.x = layout.subtitle_text.x + subtitle_text_width + kControlGap;
    }

    layout.time_text = {time_x,
                        layout.play_button.y,
                        std::max(48, layout.subtitle_previous_button.x - kControlGap - time_x),
                        layout.play_button.h};
    return layout;
}

void OpenGLVideoRenderer::Impl::drawOsdOverlay(const PendingVideoFrame* frame, int drawable_width, int drawable_height) {
    const bool paused = overlay_paused_.load();
    double preview_progress = 0.0;
    float preview_volume = overlay_volume_.load();
    bool dragging_seek = false;
    bool dragging_volume = false;
    bool seek_preview_active = false;
    bool hover_panel = false;
    bool hover_previous_item_button = false;
    bool hover_play_button = false;
    bool hover_next_item_button = false;
    bool hover_subtitle_previous_button = false;
    bool hover_subtitle_next_button = false;
    bool hover_progress = false;
    bool hover_mute_button = false;
    bool hover_volume = false;
    bool hover_fullscreen_button = false;
    {
        std::lock_guard<std::mutex> lock(request_mutex_);
        preview_progress = seek_preview_ratio_;
        preview_volume = requests_.requested_volume;
        dragging_seek = dragging_seek_;
        dragging_volume = dragging_volume_;
        seek_preview_active = seek_preview_active_;
        hover_panel = hover_panel_;
        hover_previous_item_button = hover_previous_item_button_;
        hover_play_button = hover_play_button_;
        hover_next_item_button = hover_next_item_button_;
        hover_subtitle_previous_button = hover_subtitle_previous_button_;
        hover_subtitle_next_button = hover_subtitle_next_button_;
        hover_progress = hover_progress_;
        hover_mute_button = hover_mute_button_;
        hover_volume = hover_volume_;
        hover_fullscreen_button = hover_fullscreen_button_;
    }

    const uint64_t now_ms = monotonicMsNow();
    const uint64_t visible_until_ms = osd_visible_until_ms_.load();
    const bool show_osd = paused || hover_panel || now_ms <= visible_until_ms || dragging_seek || dragging_volume;
    if (!show_osd) {
        return;
    }

    float overlay_alpha = 1.0f;
    if (!paused && !hover_panel && !dragging_seek && !dragging_volume && now_ms <= visible_until_ms &&
        visible_until_ms - now_ms < kOsdFadeOutMs) {
        overlay_alpha = std::clamp(static_cast<float>(visible_until_ms - now_ms) / static_cast<float>(kOsdFadeOutMs),
                                   0.0f,
                                   1.0f);
    }

    double progress = 0.0;
    const double duration = overlay_duration_.load();
    double display_position = overlay_position_.load();
    if (duration > 0.001) {
        progress = clampRatio(display_position / duration);
    }
    if (seek_preview_active && duration > 0.001) {
        progress = clampRatio(preview_progress);
        display_position = preview_progress * duration;
    }
    const float volume = dragging_volume ? clampVolume(preview_volume) : clampVolume(overlay_volume_.load());
    const ControlLayout layout = computeControlLayout(drawable_width, drawable_height);
    const std::string time_text = formatOverlayTimeText(display_position, duration);
    std::vector<std::string> subtitle_track_labels;
    std::string subtitle_track_feedback_text;
    {
        std::lock_guard<std::mutex> subtitle_lock(subtitle_mutex_);
        subtitle_track_labels = subtitle_track_labels_;
        subtitle_track_feedback_text = subtitle_track_feedback_text_;
    }
    const int subtitle_track_count = std::max(0, subtitle_track_count_.load());
    const int subtitle_track_current_ordinal = subtitle_track_count > 0
                                                   ? std::clamp(subtitle_track_current_ordinal_.load(), 1, subtitle_track_count)
                                                   : 0;
    std::string subtitle_track_text =
        std::to_string(subtitle_track_current_ordinal) + " / " + std::to_string(subtitle_track_count);
    if (subtitle_track_current_ordinal > 0 &&
        subtitle_track_current_ordinal <= static_cast<int>(subtitle_track_labels.size())) {
        const std::string& current_label =
            subtitle_track_labels[static_cast<size_t>(subtitle_track_current_ordinal - 1)];
        if (!current_label.empty()) {
            subtitle_track_text += "  " + current_label;
        }
    }
    std::string subtitle_track_list_text;
    if (!subtitle_track_labels.empty()) {
        const size_t kMaxLabelCount = 4;
        for (size_t i = 0; i < subtitle_track_labels.size() && i < kMaxLabelCount; ++i) {
            if (!subtitle_track_list_text.empty()) {
                subtitle_track_list_text += " | ";
            }
            const bool is_current = static_cast<int>(i + 1) == subtitle_track_current_ordinal;
            subtitle_track_list_text += (is_current ? "*" : "") + std::to_string(i + 1) + ":" + subtitle_track_labels[i];
        }
        if (subtitle_track_labels.size() > kMaxLabelCount) {
            subtitle_track_list_text += " | ...";
        }
    }
    int glyph_height = std::max(12, layout.time_text.h - 10);
    while (glyph_height > 10 && measureOverlayTextWidth(time_text, glyph_height) > layout.time_text.w) {
        --glyph_height;
    }
    int subtitle_glyph_height = std::max(10, layout.subtitle_text.h - 12);
    while (subtitle_glyph_height > 8 &&
           measureOverlayTextWidth(subtitle_track_text, subtitle_glyph_height) > layout.subtitle_text.w) {
        --subtitle_glyph_height;
    }
    const int text_y = layout.time_text.y + std::max(0, (layout.time_text.h - glyph_height) / 2);
    const int subtitle_text_width = measureOverlayTextWidth(subtitle_track_text, subtitle_glyph_height);
    const int subtitle_text_x = layout.subtitle_text.x +
                                std::max(0, (layout.subtitle_text.w - subtitle_text_width) / 2);
    const int subtitle_text_y = layout.subtitle_text.y +
                                std::max(0, (layout.subtitle_text.h - subtitle_glyph_height) / 2);
    const bool show_track_feedback =
        !subtitle_track_feedback_text.empty() && now_ms <= subtitle_track_feedback_until_ms_.load();
    const int progress_fill_width =
        std::max(0, static_cast<int>(std::lround(progress * static_cast<double>(layout.progress_track.w))));
    const int volume_fill_width =
        std::max(0, static_cast<int>(std::lround(volume * static_cast<double>(layout.volume_track.w))));
    const float progress_knob_x = static_cast<float>(layout.progress_track.x + std::clamp(progress_fill_width, 0, layout.progress_track.w));
    const float progress_knob_y = static_cast<float>(layout.progress_track.y + layout.progress_track.h / 2);
    const float volume_knob_x = static_cast<float>(layout.volume_track.x + std::clamp(volume_fill_width, 0, layout.volume_track.w));
    const float volume_knob_y = static_cast<float>(layout.volume_track.y + layout.volume_track.h / 2);

    g_gl.UseProgram(0);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    drawFilledRect(drawable_width, drawable_height, layout.panel.x, layout.panel.y, layout.panel.w, layout.panel.h, 0.02f, 0.02f, 0.03f, 0.82f * overlay_alpha);
    drawFilledRect(drawable_width, drawable_height, layout.panel.x, layout.panel.y, layout.panel.w, 1, 0.96f, 0.68f, 0.18f, 0.42f * overlay_alpha);

    const float button_base_r = 0.16f;
    const float button_base_g = 0.17f;
    const float button_base_b = 0.18f;
    const auto drawButtonBackground = [&](const SDL_Rect& rect, bool hovered) {
        drawFilledCircle(drawable_width,
                         drawable_height,
                         static_cast<float>(rect.x + rect.w / 2),
                         static_cast<float>(rect.y + rect.h / 2),
                         static_cast<float>(rect.w) * 0.5f,
                         button_base_r,
                         button_base_g,
                         hovered ? 0.22f : button_base_b,
                         (hovered ? 0.42f : 0.30f) * overlay_alpha);
    };

    drawButtonBackground(layout.previous_item_button, hover_previous_item_button);
    drawButtonBackground(layout.play_button, hover_play_button);
    drawButtonBackground(layout.next_item_button, hover_next_item_button);
    drawButtonBackground(layout.subtitle_previous_button, hover_subtitle_previous_button);
    drawButtonBackground(layout.subtitle_next_button, hover_subtitle_next_button);
    drawButtonBackground(layout.mute_button, hover_mute_button);
    drawButtonBackground(layout.fullscreen_button, hover_fullscreen_button);

    // Previous item icon: |<
    {
        const float cx = static_cast<float>(layout.previous_item_button.x + layout.previous_item_button.w / 2);
        const float cy = static_cast<float>(layout.previous_item_button.y + layout.previous_item_button.h / 2);
        const float w = static_cast<float>(layout.previous_item_button.w);
        const float h = static_cast<float>(layout.previous_item_button.h);
        const float icon_left = cx - w * 0.18f;
        const float icon_right = cx + w * 0.20f;
        drawFilledRect(drawable_width,
                       drawable_height,
                       static_cast<int>(std::lround(icon_right - w * 0.08f)),
                       static_cast<int>(std::lround(cy - h * 0.20f)),
                       std::max(2, static_cast<int>(std::lround(w * 0.08f))),
                       std::max(8, static_cast<int>(std::lround(h * 0.40f))),
                       1.0f,
                       1.0f,
                       1.0f,
                       0.92f * overlay_alpha);
        drawFilledTriangle(drawable_width,
                           drawable_height,
                           icon_left,
                           cy,
                           icon_right,
                           cy - h * 0.26f,
                           icon_right,
                           cy + h * 0.26f,
                           1.0f,
                           1.0f,
                           1.0f,
                           0.92f * overlay_alpha);
    }

    drawFilledRect(drawable_width,
                   drawable_height,
                   layout.progress_track.x,
                   layout.progress_track.y,
                   layout.progress_track.w,
                   layout.progress_track.h,
                   hover_progress ? 0.24f : 0.18f,
                   hover_progress ? 0.24f : 0.18f,
                   hover_progress ? 0.26f : 0.18f,
                   0.82f * overlay_alpha);
    drawFilledRect(drawable_width,
                   drawable_height,
                   layout.progress_track.x,
                   layout.progress_track.y,
                   progress_fill_width,
                   layout.progress_track.h,
                   0.96f,
                   0.68f,
                   0.18f,
                   0.96f * overlay_alpha);
    drawFilledCircle(drawable_width,
                     drawable_height,
                     progress_knob_x,
                     progress_knob_y,
                     hover_progress || dragging_seek ? 6.0f : 5.0f,
                     1.0f,
                     0.95f,
                     0.82f,
                     0.98f * overlay_alpha);

    if (paused) {
        const float left = static_cast<float>(layout.play_button.x) + static_cast<float>(layout.play_button.w) * 0.36f;
        const float right = static_cast<float>(layout.play_button.x) + static_cast<float>(layout.play_button.w) * 0.72f;
        const float top = static_cast<float>(layout.play_button.y) + static_cast<float>(layout.play_button.h) * 0.24f;
        const float bottom = static_cast<float>(layout.play_button.y) + static_cast<float>(layout.play_button.h) * 0.76f;
        const float middle_y = static_cast<float>(layout.play_button.y) + static_cast<float>(layout.play_button.h) * 0.5f;
        drawFilledTriangle(drawable_width, drawable_height, left, top, right, middle_y, left, bottom, 1.0f, 1.0f, 1.0f, 0.94f * overlay_alpha);
    } else {
        const int bar_width = std::max(4, layout.play_button.w / 6);
        const int bar_height = std::max(12, layout.play_button.h - layout.play_button.h / 3);
        const int top = layout.play_button.y + (layout.play_button.h - bar_height) / 2;
        const int left_bar = layout.play_button.x + layout.play_button.w / 2 - bar_width - 2;
        const int right_bar = layout.play_button.x + layout.play_button.w / 2 + 2;
        drawFilledRect(drawable_width, drawable_height, left_bar, top, bar_width, bar_height, 1.0f, 1.0f, 1.0f, 0.94f * overlay_alpha);
        drawFilledRect(drawable_width, drawable_height, right_bar, top, bar_width, bar_height, 1.0f, 1.0f, 1.0f, 0.94f * overlay_alpha);
    }

    // Next item icon: >|
    {
        const float cx = static_cast<float>(layout.next_item_button.x + layout.next_item_button.w / 2);
        const float cy = static_cast<float>(layout.next_item_button.y + layout.next_item_button.h / 2);
        const float w = static_cast<float>(layout.next_item_button.w);
        const float h = static_cast<float>(layout.next_item_button.h);
        const float icon_left = cx - w * 0.20f;
        const float icon_right = cx + w * 0.18f;
        drawFilledRect(drawable_width,
                       drawable_height,
                       static_cast<int>(std::lround(icon_left)),
                       static_cast<int>(std::lround(cy - h * 0.20f)),
                       std::max(2, static_cast<int>(std::lround(w * 0.08f))),
                       std::max(8, static_cast<int>(std::lround(h * 0.40f))),
                       1.0f,
                       1.0f,
                       1.0f,
                       0.92f * overlay_alpha);
        drawFilledTriangle(drawable_width,
                           drawable_height,
                           icon_right,
                           cy,
                           icon_left,
                           cy - h * 0.26f,
                           icon_left,
                           cy + h * 0.26f,
                           1.0f,
                           1.0f,
                           1.0f,
                           0.92f * overlay_alpha);
    }

    // Subtitle track previous/next icons and current track text.
    {
        const auto drawArrow = [&](const SDL_Rect& rect, bool to_left) {
            const float cx = static_cast<float>(rect.x + rect.w / 2);
            const float cy = static_cast<float>(rect.y + rect.h / 2);
            const float w = static_cast<float>(rect.w);
            const float h = static_cast<float>(rect.h);
            if (to_left) {
                drawFilledTriangle(drawable_width,
                                   drawable_height,
                                   cx - w * 0.18f,
                                   cy,
                                   cx + w * 0.14f,
                                   cy - h * 0.23f,
                                   cx + w * 0.14f,
                                   cy + h * 0.23f,
                                   0.95f,
                                   0.95f,
                                   0.95f,
                                   0.90f * overlay_alpha);
            } else {
                drawFilledTriangle(drawable_width,
                                   drawable_height,
                                   cx + w * 0.18f,
                                   cy,
                                   cx - w * 0.14f,
                                   cy - h * 0.23f,
                                   cx - w * 0.14f,
                                   cy + h * 0.23f,
                                   0.95f,
                                   0.95f,
                                   0.95f,
                                   0.90f * overlay_alpha);
            }
        };
        drawArrow(layout.subtitle_previous_button, true);
        drawArrow(layout.subtitle_next_button, false);
        drawOverlayText(drawable_width,
                        drawable_height,
                        subtitle_text_x,
                        subtitle_text_y,
                        subtitle_glyph_height,
                        subtitle_track_text,
                        subtitle_track_count > 0 ? 0.92f : 0.62f,
                        subtitle_track_count > 0 ? 0.92f : 0.62f,
                        subtitle_track_count > 0 ? 0.92f : 0.62f,
                        0.90f * overlay_alpha);

        if (!subtitle_track_list_text.empty()) {
            const int subtitle_list_glyph_height = std::max(8, subtitle_glyph_height - 2);
            const int subtitle_list_text_width = measureOverlayTextWidth(subtitle_track_list_text, subtitle_list_glyph_height);
            const int subtitle_list_text_x = std::max(8,
                                                      std::min(layout.panel.x + layout.panel.w - subtitle_list_text_width - 8,
                                                               layout.subtitle_text.x));
            const int subtitle_list_text_y = std::max(8, layout.panel.y - subtitle_list_glyph_height - 8);
            drawOverlayText(drawable_width,
                            drawable_height,
                            subtitle_list_text_x,
                            subtitle_list_text_y,
                            subtitle_list_glyph_height,
                            subtitle_track_list_text,
                            0.82f,
                            0.84f,
                            0.88f,
                            0.86f * overlay_alpha);
        }
    }

    if (show_track_feedback) {
        const int feedback_glyph_height = std::max(11, subtitle_glyph_height);
        const int feedback_text_width = measureOverlayTextWidth(subtitle_track_feedback_text, feedback_glyph_height);
        const int feedback_text_x = std::max(10, (drawable_width - feedback_text_width) / 2);
        const int feedback_text_y = std::max(10, layout.panel.y - feedback_glyph_height - 30);
        drawOverlayText(drawable_width,
                        drawable_height,
                        feedback_text_x,
                        feedback_text_y,
                        feedback_glyph_height,
                        subtitle_track_feedback_text,
                        0.98f,
                        0.78f,
                        0.35f,
                        0.96f * overlay_alpha);
    }

    drawOverlayText(drawable_width,
                    drawable_height,
                    layout.time_text.x,
                    text_y,
                    glyph_height,
                    time_text,
                    seek_preview_active ? 0.96f : 0.94f,
                    seek_preview_active ? 0.76f : 0.94f,
                    seek_preview_active ? 0.34f : 0.94f,
                    0.92f * overlay_alpha);

    drawFilledRect(drawable_width,
                   drawable_height,
                   layout.volume_track.x,
                   layout.volume_track.y,
                   layout.volume_track.w,
                   layout.volume_track.h,
                   hover_volume ? 0.24f : 0.18f,
                   hover_volume ? 0.24f : 0.18f,
                   hover_volume ? 0.26f : 0.18f,
                   0.82f * overlay_alpha);
    drawFilledRect(drawable_width,
                   drawable_height,
                   layout.volume_track.x,
                   layout.volume_track.y,
                   volume_fill_width,
                   layout.volume_track.h,
                   0.42f,
                   0.82f,
                   0.58f,
                   0.96f * overlay_alpha);
    drawFilledCircle(drawable_width,
                     drawable_height,
                     volume_knob_x,
                     volume_knob_y,
                     hover_volume || dragging_volume ? 5.5f : 4.5f,
                    0.90f,
                    0.96f,
                    0.92f,
                    0.98f * overlay_alpha);

    // Mute/unmute icon.
    {
        const bool muted = volume <= 0.0001f;
        const float cx = static_cast<float>(layout.mute_button.x + layout.mute_button.w / 2);
        const float cy = static_cast<float>(layout.mute_button.y + layout.mute_button.h / 2);
        const float w = static_cast<float>(layout.mute_button.w);
        const float h = static_cast<float>(layout.mute_button.h);
        drawFilledRect(drawable_width,
                       drawable_height,
                       static_cast<int>(std::lround(cx - w * 0.23f)),
                       static_cast<int>(std::lround(cy - h * 0.12f)),
                       std::max(2, static_cast<int>(std::lround(w * 0.12f))),
                       std::max(6, static_cast<int>(std::lround(h * 0.24f))),
                       0.95f,
                       0.95f,
                       0.95f,
                       0.90f * overlay_alpha);
        drawFilledTriangle(drawable_width,
                           drawable_height,
                           cx - w * 0.08f,
                           cy,
                           cx + w * 0.18f,
                           cy - h * 0.22f,
                           cx + w * 0.18f,
                           cy + h * 0.22f,
                           0.95f,
                           0.95f,
                           0.95f,
                           0.90f * overlay_alpha);
        if (!muted) {
            drawFilledRect(drawable_width,
                           drawable_height,
                           static_cast<int>(std::lround(cx + w * 0.23f)),
                           static_cast<int>(std::lround(cy - h * 0.14f)),
                           std::max(2, static_cast<int>(std::lround(w * 0.04f))),
                           std::max(6, static_cast<int>(std::lround(h * 0.28f))),
                           0.95f,
                           0.95f,
                           0.95f,
                           0.86f * overlay_alpha);
            drawFilledRect(drawable_width,
                           drawable_height,
                           static_cast<int>(std::lround(cx + w * 0.29f)),
                           static_cast<int>(std::lround(cy - h * 0.20f)),
                           std::max(2, static_cast<int>(std::lround(w * 0.04f))),
                           std::max(8, static_cast<int>(std::lround(h * 0.40f))),
                           0.95f,
                           0.95f,
                           0.95f,
                           0.74f * overlay_alpha);
        } else {
            const int line_w = std::max(2, layout.mute_button.w / 10);
            const int step_count = std::max(6, layout.mute_button.w / 3);
            for (int step = 0; step < step_count; ++step) {
                const float t = static_cast<float>(step) / static_cast<float>(std::max(1, step_count - 1));
                const int px = static_cast<int>(std::lround(layout.mute_button.x + layout.mute_button.w * (0.30f + t * 0.40f)));
                const int py = static_cast<int>(std::lround(layout.mute_button.y + layout.mute_button.h * (0.70f - t * 0.40f)));
                drawFilledRect(drawable_width, drawable_height, px, py, line_w, line_w + 1, 0.96f, 0.32f, 0.32f, 0.90f * overlay_alpha);
            }
        }
    }

    // Fullscreen icon (four corner L strokes).
    {
        const int stroke = std::max(2, layout.fullscreen_button.w / 12);
        const int corner = std::max(6, layout.fullscreen_button.w / 4);
        const SDL_Rect& r = layout.fullscreen_button;
        const float cr = 0.95f;
        const float cg = 0.95f;
        const float cb = 0.95f;
        const float ca = 0.90f * overlay_alpha;
        drawFilledRect(drawable_width, drawable_height, r.x + stroke, r.y + stroke, corner, stroke, cr, cg, cb, ca);
        drawFilledRect(drawable_width, drawable_height, r.x + stroke, r.y + stroke, stroke, corner, cr, cg, cb, ca);
        drawFilledRect(drawable_width, drawable_height, r.x + r.w - stroke - corner, r.y + stroke, corner, stroke, cr, cg, cb, ca);
        drawFilledRect(drawable_width, drawable_height, r.x + r.w - stroke * 2, r.y + stroke, stroke, corner, cr, cg, cb, ca);
        drawFilledRect(drawable_width, drawable_height, r.x + stroke, r.y + r.h - stroke * 2, corner, stroke, cr, cg, cb, ca);
        drawFilledRect(drawable_width, drawable_height, r.x + stroke, r.y + r.h - stroke - corner, stroke, corner, cr, cg, cb, ca);
        drawFilledRect(drawable_width, drawable_height, r.x + r.w - stroke - corner, r.y + r.h - stroke * 2, corner, stroke, cr, cg, cb, ca);
        drawFilledRect(drawable_width, drawable_height, r.x + r.w - stroke * 2, r.y + r.h - stroke - corner, stroke, corner, cr, cg, cb, ca);
    }

    if (paused) {
        const SDL_Rect video_rect = frame && frame->valid
            ? computeRenderRect(drawable_width, drawable_height, frame->width, frame->height)
            : SDL_Rect{0, 0, drawable_width, drawable_height};
        const float center_x = static_cast<float>(video_rect.x + video_rect.w / 2);
        const float center_y = static_cast<float>(video_rect.y + video_rect.h / 2);
        const float badge_radius = std::max(24.0f, std::min(static_cast<float>(video_rect.w), static_cast<float>(video_rect.h)) * 0.06f);
        drawFilledCircle(drawable_width, drawable_height, center_x, center_y, badge_radius, 0.02f, 0.02f, 0.03f, 0.32f * overlay_alpha);
        drawFilledTriangle(drawable_width,
                           drawable_height,
                           center_x - badge_radius * 0.22f,
                           center_y - badge_radius * 0.34f,
                           center_x + badge_radius * 0.34f,
                           center_y,
                           center_x - badge_radius * 0.22f,
                           center_y + badge_radius * 0.34f,
                           1.0f,
                           1.0f,
                           1.0f,
                           0.72f * overlay_alpha);
    }
    glDisable(GL_BLEND);
}

void OpenGLVideoRenderer::Impl::pumpEvents() {
    SDL_Event event{};
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_QUIT: should_quit_.store(true); break;
        case SDL_KEYDOWN:
            handleKeyDown(static_cast<int>(event.key.keysym.sym),
                          static_cast<unsigned short>(event.key.keysym.mod),
                          event.key.repeat != 0);
            break;
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
                markOutputColorBindingDirty();
                requestRedraw();
            }
            if (event.window.event == SDL_WINDOWEVENT_MOVED || event.window.event == SDL_WINDOWEVENT_SHOWN ||
                event.window.event == SDL_WINDOWEVENT_RESTORED || event.window.event == SDL_WINDOWEVENT_MAXIMIZED) {
                markOutputColorBindingDirty();
            }
#if defined(SDL_WINDOWEVENT_DISPLAY_CHANGED)
            if (event.window.event == SDL_WINDOWEVENT_DISPLAY_CHANGED) {
                markOutputColorBindingDirty();
                requestRedraw();
            }
#endif
            if (event.window.event == SDL_WINDOWEVENT_LEAVE || event.window.event == SDL_WINDOWEVENT_HIDDEN) {
                {
                    std::lock_guard<std::mutex> lock(request_mutex_);
                    hover_panel_ = false;
                    hover_previous_item_button_ = false;
                    hover_play_button_ = false;
                    hover_next_item_button_ = false;
                    hover_subtitle_previous_button_ = false;
                    hover_subtitle_next_button_ = false;
                    hover_progress_ = false;
                    hover_mute_button_ = false;
                    hover_volume_ = false;
                    hover_fullscreen_button_ = false;
                }
                requestRedraw();
            }
            break;
        case SDL_DROPFILE:
            {
                std::lock_guard<std::mutex> lock(request_mutex_);
                open_file_path_ = event.drop.file ? event.drop.file : "";
                open_file_requested_ = !open_file_path_.empty();
                if (event.drop.file) {
                    SDL_free(event.drop.file);
                }
            }
            break;
        case SDL_MOUSEBUTTONDOWN:
            handleMouseButtonDown(event.button.x, event.button.y, event.button.button);
            break;
        case SDL_MOUSEMOTION:
            handleMouseMotion(event.motion.x, event.motion.y);
            break;
        case SDL_MOUSEBUTTONUP:
            handleMouseButtonUp(event.button.button);
            break;
        default: break;
        }
    }
}

void OpenGLVideoRenderer::Impl::handleEvents() {
    if (!window_) {
        return;
    }
    if (event_thread_id_ != std::thread::id{} && event_thread_id_ != std::this_thread::get_id()) {
        if (!event_thread_guard_reported_.exchange(true)) {
            LOG_WARNING("OpenGL renderer handleEvents called from non-event thread; ignoring event pump");
        }
        return;
    }

    // SDL events are pumped on the main thread to avoid thread-affinity stalls
    // during mouse/window interaction on some platforms.
    pumpEvents();

    if (fullscreen_toggle_requested_.exchange(false)) {
        const bool next_fullscreen = !fullscreen_.load();
        if (SDL_SetWindowFullscreen(window_, next_fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0) == 0) {
            fullscreen_.store(next_fullscreen);
        } else {
            LOG_WARNING("OpenGL fullscreen toggle failed: " << SDL_GetError());
        }
        updateWindowSizeFromSdl(window_, width_, height_);
        minimized_.store(false);
        markOutputColorBindingDirty();
        requestRedraw();
    }
}

void OpenGLVideoRenderer::Impl::handleKeyDown(int key_code, unsigned short modifiers, bool is_repeat) {
    if (is_repeat) return;
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

void OpenGLVideoRenderer::Impl::handleMouseButtonDown(int mouse_x, int mouse_y, uint8_t button) {
    if (button != SDL_BUTTON_LEFT) return;

    showOsdFor();
    bool request_fullscreen_toggle = false;
    {
        std::lock_guard<std::mutex> lock(request_mutex_);
        const ControlLayout layout = computeControlLayout(width_.load(), height_.load());
        hover_panel_ = pointInRect(mouse_x, mouse_y, layout.panel);
        hover_previous_item_button_ = pointInRect(mouse_x, mouse_y, layout.previous_item_button);
        hover_play_button_ = pointInRect(mouse_x, mouse_y, layout.play_button);
        hover_next_item_button_ = pointInRect(mouse_x, mouse_y, layout.next_item_button);
        hover_subtitle_previous_button_ = pointInRect(mouse_x, mouse_y, layout.subtitle_previous_button);
        hover_subtitle_next_button_ = pointInRect(mouse_x, mouse_y, layout.subtitle_next_button);
        hover_progress_ = pointInRect(mouse_x, mouse_y, layout.progress_hit_box);
        hover_mute_button_ = pointInRect(mouse_x, mouse_y, layout.mute_button);
        hover_volume_ = pointInRect(mouse_x, mouse_y, layout.volume_hit_box);
        hover_fullscreen_button_ = pointInRect(mouse_x, mouse_y, layout.fullscreen_button);
        if (hover_previous_item_button_) {
            requests_.previous_item_requested = true;
        } else if (hover_play_button_) {
            requests_.toggle_pause_requested = true;
        } else if (hover_next_item_button_) {
            requests_.next_item_requested = true;
        } else if (hover_subtitle_previous_button_) {
            requests_.previous_subtitle_track_requested = true;
        } else if (hover_subtitle_next_button_) {
            requests_.next_subtitle_track_requested = true;
        } else if (hover_mute_button_) {
            const float current = clampVolume(overlay_volume_.load());
            float next = current > 0.0001f ? 0.0f : std::max(0.5f, clampVolume(last_nonzero_volume_));
            if (current > 0.0001f) {
                last_nonzero_volume_ = current;
            }
            requests_.requested_volume = next;
            requests_.volume_change_requested = true;
            overlay_volume_.store(next);
        } else if (hover_fullscreen_button_) {
            request_fullscreen_toggle = true;
        } else if (overlay_duration_.load() > 0.001 && hover_progress_) {
            dragging_seek_ = true;
            seek_preview_active_ = true;
            seek_preview_ratio_ = clampRatio(static_cast<double>(mouse_x - layout.progress_track.x) /
                                             static_cast<double>(std::max(1, layout.progress_track.w)));
        } else if (hover_volume_) {
            dragging_volume_ = true;
            const float next_volume = static_cast<float>(clampRatio(static_cast<double>(mouse_x - layout.volume_track.x) /
                                                                    static_cast<double>(std::max(1, layout.volume_track.w))));
            requests_.requested_volume = next_volume;
            requests_.volume_change_requested = true;
            overlay_volume_.store(next_volume);
        }
    }
    if (request_fullscreen_toggle) {
        requestFullscreenToggle();
    }
    requestRedraw();
}

void OpenGLVideoRenderer::Impl::handleMouseMotion(int mouse_x, int mouse_y) {
    showOsdFor();
    {
        std::lock_guard<std::mutex> lock(request_mutex_);
        const ControlLayout layout = computeControlLayout(width_.load(), height_.load());
        hover_panel_ = pointInRect(mouse_x, mouse_y, layout.panel);
        hover_previous_item_button_ = pointInRect(mouse_x, mouse_y, layout.previous_item_button);
        hover_play_button_ = pointInRect(mouse_x, mouse_y, layout.play_button);
        hover_next_item_button_ = pointInRect(mouse_x, mouse_y, layout.next_item_button);
        hover_subtitle_previous_button_ = pointInRect(mouse_x, mouse_y, layout.subtitle_previous_button);
        hover_subtitle_next_button_ = pointInRect(mouse_x, mouse_y, layout.subtitle_next_button);
        hover_progress_ = pointInRect(mouse_x, mouse_y, layout.progress_hit_box);
        hover_mute_button_ = pointInRect(mouse_x, mouse_y, layout.mute_button);
        hover_volume_ = pointInRect(mouse_x, mouse_y, layout.volume_hit_box);
        hover_fullscreen_button_ = pointInRect(mouse_x, mouse_y, layout.fullscreen_button);
        if (dragging_seek_) {
            seek_preview_ratio_ = clampRatio(static_cast<double>(mouse_x - layout.progress_track.x) /
                                             static_cast<double>(std::max(1, layout.progress_track.w)));
        }
        if (dragging_volume_) {
            const float next_volume = static_cast<float>(clampRatio(static_cast<double>(mouse_x - layout.volume_track.x) /
                                                                    static_cast<double>(std::max(1, layout.volume_track.w))));
            requests_.requested_volume = next_volume;
            requests_.volume_change_requested = true;
            overlay_volume_.store(next_volume);
        }
    }
    requestRedraw();
}

void OpenGLVideoRenderer::Impl::handleMouseButtonUp(uint8_t button) {
    if (button != SDL_BUTTON_LEFT) return;

    showOsdFor();
    {
        std::lock_guard<std::mutex> lock(request_mutex_);
        if (dragging_seek_) {
            requests_.seek_ratio = seek_preview_ratio_;
            requests_.seek_requested = true;
            seek_preview_active_ = false;
            dragging_seek_ = false;
        }
        if (dragging_volume_) {
            dragging_volume_ = false;
        }
    }
    requestRedraw();
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
    if (should_show_osd) {
        showOsdFor();
        requestRedraw();
    }
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
bool OpenGLVideoRenderer::Impl::consumePreviousSubtitleTrackRequest() { return consumeFlagRequest(requests_.previous_subtitle_track_requested); }
bool OpenGLVideoRenderer::Impl::consumeNextSubtitleTrackRequest() { return consumeFlagRequest(requests_.next_subtitle_track_requested); }
bool OpenGLVideoRenderer::Impl::consumeSubtitleDelayChangeRequest(double& delta_seconds) { return consumeDoubleRequest(requests_.subtitle_delay_change_requested, requests_.subtitle_delay_delta_seconds, delta_seconds); }
bool OpenGLVideoRenderer::Impl::consumeAudioDelayChangeRequest(double& delta_seconds) { return consumeDoubleRequest(requests_.audio_delay_change_requested, requests_.audio_delay_delta_seconds, delta_seconds); }
bool OpenGLVideoRenderer::Impl::consumeNextChapterRequest() { return consumeFlagRequest(requests_.next_chapter_requested); }
bool OpenGLVideoRenderer::Impl::consumePreviousChapterRequest() { return consumeFlagRequest(requests_.previous_chapter_requested); }
bool OpenGLVideoRenderer::Impl::consumeNextItemRequest() { return consumeFlagRequest(requests_.next_item_requested); }
bool OpenGLVideoRenderer::Impl::consumePreviousItemRequest() { return consumeFlagRequest(requests_.previous_item_requested); }
bool OpenGLVideoRenderer::Impl::consumeOpenFileRequest(std::string& path) {
    std::lock_guard<std::mutex> lock(request_mutex_);
    if (!open_file_requested_) return false;
    path = open_file_path_;
    open_file_path_.clear();
    open_file_requested_ = false;
    return !path.empty();
}
void OpenGLVideoRenderer::Impl::setOverlayState(double position, double duration, float volume, bool paused) {
    overlay_position_.store(std::max(0.0, position));
    overlay_duration_.store(std::max(0.0, duration));
    overlay_volume_.store(clampVolume(volume));
    overlay_paused_.store(paused);
    if (paused) requestRedraw();
}

void OpenGLVideoRenderer::Impl::setSubtitleClock(double subtitle_time_seconds) {
    const double previous = subtitle_clock_seconds_.exchange(std::max(0.0, subtitle_time_seconds));
    if (overlay_paused_.load() &&
        subtitle_has_animated_content_.load() &&
        std::abs(previous - subtitle_time_seconds) > 0.0005) {
        requestRedraw();
    }
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
    item.style.secondary_color = subtitle::SubtitleColor(255, 210, 64, 252);
    item.style.outline_color = subtitle::SubtitleColor(0, 0, 0, 255);
    item.style.background_color = subtitle::SubtitleColor(4, 4, 4, 148);
    item.style.alignment = 2;
    item.style.margin_l = 24;
    item.style.margin_r = 24;
    item.style.margin_v = 24;
    item.style.border_style = 3;
    item.style.outline = 2.0;
    item.style.shadow = 2.0;
    item.style.outline_x = 2.0;
    item.style.outline_y = 2.0;
    item.style.shadow_x = 2.0;
    item.style.shadow_y = 2.0;
    setSubtitleItems({item});
}

void OpenGLVideoRenderer::Impl::setSubtitleItems(const std::vector<subtitle::SubtitleItem>& items) {
    {
        std::lock_guard<std::mutex> lock(subtitle_mutex_);
        subtitle_items_ = items;
    }
    subtitle_has_animated_content_.store(subtitleItemsHaveAnimatedRuns(items));
    subtitle_generation_.fetch_add(1);
    requestRedrawIfPaused();
}

void OpenGLVideoRenderer::Impl::setSubtitleTrackState(int current_ordinal, int track_count) {
    const int safe_track_count = std::max(0, track_count);
    const int safe_current_ordinal = safe_track_count > 0 ? std::clamp(current_ordinal, 1, safe_track_count) : 0;
    const int previous_current_ordinal = subtitle_track_current_ordinal_.load();
    const int previous_track_count = subtitle_track_count_.load();
    subtitle_track_current_ordinal_.store(safe_current_ordinal);
    subtitle_track_count_.store(safe_track_count);
    if (safe_current_ordinal != previous_current_ordinal || safe_track_count != previous_track_count) {
        std::string feedback_text = "Subtitle Track: none";
        if (safe_current_ordinal > 0 && safe_track_count > 0) {
            feedback_text = "Subtitle Track " + std::to_string(safe_current_ordinal) + "/" +
                            std::to_string(safe_track_count);
            std::lock_guard<std::mutex> lock(subtitle_mutex_);
            if (safe_current_ordinal <= static_cast<int>(subtitle_track_labels_.size())) {
                const std::string& label = subtitle_track_labels_[static_cast<size_t>(safe_current_ordinal - 1)];
                if (!label.empty()) {
                    feedback_text += ": " + label;
                }
            }
            subtitle_track_feedback_text_ = feedback_text;
        } else {
            std::lock_guard<std::mutex> lock(subtitle_mutex_);
            subtitle_track_feedback_text_ = feedback_text;
        }
        subtitle_track_feedback_until_ms_.store(monotonicMsNow() + 1600);
        showOsdFor();
    }
    requestRedrawIfPaused();
}

void OpenGLVideoRenderer::Impl::setSubtitleTrackCatalog(const std::vector<std::string>& track_labels, int current_ordinal) {
    const int safe_track_count = static_cast<int>(track_labels.size());
    const int safe_current_ordinal =
        safe_track_count > 0 ? std::clamp(current_ordinal, 1, safe_track_count) : 0;
    {
        std::lock_guard<std::mutex> lock(subtitle_mutex_);
        subtitle_track_labels_ = track_labels;
    }
    subtitle_track_count_.store(safe_track_count);
    subtitle_track_current_ordinal_.store(safe_current_ordinal);
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
#if defined(_WIN32)
    diagnostics.opengl_native_interop_active = native_startup_allowed_.load() && !native_session_disabled_.load();
    diagnostics.opengl_native_interop_startup_disabled = native_diagnostics_.native_direct_startup_disabled;
    diagnostics.opengl_native_interop_disable_rule = native_diagnostics_.native_direct_disable_rule;
#endif
    diagnostics.opengl_native_interop_frames = native_interop_frames_.load();
    diagnostics.opengl_native_interop_disable_events = native_interop_disable_events_.load();
    diagnostics.opengl_present_wait_timeouts = present_wait_timeouts_.load();
    diagnostics.opengl_present_mode_requested = openGLPresentModeName(present_mode_requested_);
    diagnostics.opengl_present_mode_active = present_mode_active_;
    diagnostics.opengl_hdr_bridge_requested = hdr_bridge_requested_.load();
    diagnostics.opengl_hdr_bridge_active = hdr_bridge_active_.load();
    diagnostics.opengl_hdr_bridge_mode = openGLHdrBridgeModeName(hdr_bridge_mode_);
    diagnostics.opengl_hdr_bridge_decision =
        openGLHdrBridgeDecisionName(static_cast<OpenGLHdrBridgeDecision>(hdr_bridge_decision_.load()));
    diagnostics.opengl_output_lut_configured = output_lut_configured_.load();
    diagnostics.opengl_output_lut_active = output_lut_active_.load();
    diagnostics.opengl_output_lut_size = output_lut_size_.load();
    diagnostics.opengl_output_lut_reload_count = output_lut_reload_count_.load();
    diagnostics.opengl_output_display_index = output_display_index_.load();
    diagnostics.opengl_output_icc_profile_available = output_icc_profile_available_.load();
    {
        std::lock_guard<std::mutex> lock(output_color_mutex_);
        diagnostics.opengl_output_lut_path = output_lut_path_.empty() ? std::string("none") : output_lut_path_;
        diagnostics.opengl_output_lut_error = output_lut_error_.empty() ? std::string("none") : output_lut_error_;
        diagnostics.opengl_output_lut_source = output_lut_source_;
        diagnostics.opengl_output_display_name = output_display_name_;
        diagnostics.opengl_output_display_device_name = output_display_device_name_;
        diagnostics.opengl_output_icc_profile_source = output_icc_profile_source_;
        diagnostics.opengl_output_icc_profile_path = output_icc_profile_path_.empty() ? std::string("none") : output_icc_profile_path_;
        diagnostics.opengl_output_icc_profile_description = output_icc_profile_description_;
        diagnostics.opengl_output_binding_error = output_binding_error_.empty() ? std::string("none") : output_binding_error_;
    }
    return diagnostics;
}

void OpenGLVideoRenderer::Impl::resetDiagnostics() {
    frame_copy_frames_.store(0);
    frame_copy_bytes_.store(0);
    frame_copy_time_us_total_.store(0);
    frame_copy_time_us_max_.store(0);
    native_interop_frames_.store(0);
    native_interop_disable_events_.store(0);
    present_wait_timeouts_.store(0);
    hdr_bridge_requested_.store(false);
    hdr_bridge_active_.store(false);
    hdr_bridge_decision_.store(static_cast<int>(OpenGLHdrBridgeDecision::NotEvaluated));
    output_lut_active_.store(false);
    output_lut_reload_count_.store(0);
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
    return format == AV_PIX_FMT_YUV420P || format == AV_PIX_FMT_NV12 ||
           format == AV_PIX_FMT_P010LE || format == AV_PIX_FMT_P016LE;
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
cbuffer ColorMatrix : register(b0) { float4 coeffR; float4 coeffG; float4 coeffB; float4 colorConfig0; };
Texture2D texY : register(t0); Texture2D texUV : register(t1); SamplerState samp : register(s0);
struct PSIn { float4 pos : SV_POSITION; float2 uv : TEXCOORD0; };

float3 decodeGamma22(float3 value) {
    return pow(max(value, float3(0.0, 0.0, 0.0)), float3(2.2, 2.2, 2.2));
}

float3 encodeGamma22(float3 value) {
    return pow(max(value, float3(0.0, 0.0, 0.0)), float3(1.0 / 2.2, 1.0 / 2.2, 1.0 / 2.2));
}

float3 decodePq(float3 value) {
    const float m1 = 0.1593017578125;
    const float m2 = 78.84375;
    const float c1 = 0.8359375;
    const float c2 = 18.8515625;
    const float c3 = 18.6875;
    float3 vp = pow(max(value, float3(0.0, 0.0, 0.0)), float3(1.0 / m2, 1.0 / m2, 1.0 / m2));
    float3 num = max(vp - float3(c1, c1, c1), float3(0.0, 0.0, 0.0));
    float3 den = max(float3(c2, c2, c2) - float3(c3, c3, c3) * vp, float3(1e-6, 1e-6, 1e-6));
    return pow(num / den, float3(1.0 / m1, 1.0 / m1, 1.0 / m1));
}

float decodeHlgComponent(float value) {
    return value <= 0.5 ? (value * value) / 3.0 : (exp((value - 0.55991073) / 0.17883277) + 0.28466892) / 12.0;
}

float3 decodeHlg(float3 value) {
    return float3(decodeHlgComponent(value.x), decodeHlgComponent(value.y), decodeHlgComponent(value.z));
}

float3 bt2020ToBt709(float3 value) {
    return float3(
        dot(value, float3(1.6605, -0.5876, -0.0728)),
        dot(value, float3(-0.1246, 1.1329, -0.0083)),
        dot(value, float3(-0.0182, -0.1006, 1.1187)));
}

float3 postProcessRgb(float3 encodedRgb) {
    int transferMode = (int)(colorConfig0.x + 0.5);
    int gamutMode = (int)(colorConfig0.y + 0.5);
    int hdrOutputMode = (int)(colorConfig0.z + 0.5);
    float3 working = max(encodedRgb, float3(0.0, 0.0, 0.0));
    bool linearized = false;
    if (transferMode == 1) {
        working = decodePq(working) * (10000.0 / 203.0);
        linearized = true;
    } else if (transferMode == 2) {
        working = decodeHlg(working) * (1000.0 / 203.0);
        linearized = true;
    }
    if (transferMode != 0 && hdrOutputMode == 1) {
        return saturate(max(encodedRgb, float3(0.0, 0.0, 0.0)));
    }
    if (gamutMode == 1) {
        if (!linearized) {
            working = decodeGamma22(working);
            linearized = true;
        }
        working = max(bt2020ToBt709(working), float3(0.0, 0.0, 0.0));
    }
    if (transferMode != 0) {
        if (!linearized) {
            working = decodeGamma22(working);
            linearized = true;
        }
        working = working / (float3(1.0, 1.0, 1.0) + working);
    }
    return linearized ? saturate(encodeGamma22(working)) : saturate(working);
}

float4 main(PSIn input) : SV_Target {
    float y = texY.Sample(samp, input.uv).r;
    float2 uv = texUV.Sample(samp, input.uv).rg;
    float3 rgb = float3(dot(float4(y, uv.x, uv.y, 1.0), coeffR), dot(float4(y, uv.x, uv.y, 1.0), coeffG), dot(float4(y, uv.x, uv.y, 1.0), coeffB));
    return float4(postProcessRgb(rgb), 1.0);
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
    native_interop_disable_events_.fetch_add(1);
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

bool OpenGLVideoRenderer::Impl::updateNativeGlTexture(const AVFrame* frame, bool hdr_bridge_active) {
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
    const ColorMatrixConstants matrix = buildNativeColorMatrix(frame,
                                                               desc.Format == DXGI_FORMAT_P010 || desc.Format == DXGI_FORMAT_P016,
                                                               hdr_bridge_active);
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

    HANDLE object_handle = interop_object_handle_;
    if (!wgl_dx_lock_objects_nv_(interop_device_handle_, 1, &object_handle)) { disableNativeInterop("wglDXLockObjectsNV failed after D3D11 render", &desc, index, S_OK, S_OK); return false; }
    interop_object_locked_ = true;
    return true;
}

bool OpenGLVideoRenderer::Impl::initializeNativeInterop() {
    native_override_mode_ = readOpenGLNativeInteropOverride();
    native_diagnostics_ = probeOpenGLDiagnosticsWithCurrentContext(native_override_mode_);
    logOpenGLStartupDiagnostics(native_diagnostics_);

    if (!native_diagnostics_.native_direct_allowed) {
        native_startup_allowed_.store(false);
        LOG_WARNING("OpenGL native D3D11 interop startup disabled: rule=" << native_diagnostics_.native_direct_disable_rule
                    << " reason=\"" << native_diagnostics_.native_direct_disable_reason << "\" fallback=copyback-to-software");
        return false;
    }
    if (!loadWglInteropFunctions()) {
        native_startup_allowed_.store(false);
        LOG_WARNING("OpenGL native D3D11 interop unavailable after diagnostics probe: missing WGL_NV_DX_interop entry points, gl_renderer=\"" << gl_renderer_
                    << "\" fallback=copyback-to-software");
        return false;
    }
    if (!createNativeD3DDevice() || !createNativeD3DShaders()) { destroyNativeInterop(); return false; }
    interop_device_handle_ = wgl_dx_open_device_nv_(native_d3d_device_.Get());
    if (!interop_device_handle_) {
        LOG_WARNING("OpenGL native D3D11 interop device open failed; d3d_adapter=\"" << native_diagnostics_.d3d11.adapter_name
                    << "\" gl_renderer=\"" << gl_renderer_ << "\" likely adapter/driver interop mismatch, fallback=copyback-to-software");
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

OpenGLDiagnosticsSnapshot OpenGLVideoRenderer::probeSystemDiagnostics() {
#if defined(_WIN32)
    return probeOpenGLDiagnosticsWithTemporaryContext(readOpenGLNativeInteropOverride());
#else
    return {};
#endif
}

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
bool OpenGLVideoRenderer::consumePreviousSubtitleTrackRequest() { return impl_->consumePreviousSubtitleTrackRequest(); }
bool OpenGLVideoRenderer::consumeNextSubtitleTrackRequest() { return impl_->consumeNextSubtitleTrackRequest(); }
bool OpenGLVideoRenderer::consumeSubtitleDelayChangeRequest(double& delta_seconds) { return impl_->consumeSubtitleDelayChangeRequest(delta_seconds); }
bool OpenGLVideoRenderer::consumeAudioDelayChangeRequest(double& delta_seconds) { return impl_->consumeAudioDelayChangeRequest(delta_seconds); }
bool OpenGLVideoRenderer::consumeNextChapterRequest() { return impl_->consumeNextChapterRequest(); }
bool OpenGLVideoRenderer::consumePreviousChapterRequest() { return impl_->consumePreviousChapterRequest(); }
bool OpenGLVideoRenderer::consumeNextItemRequest() { return impl_->consumeNextItemRequest(); }
bool OpenGLVideoRenderer::consumePreviousItemRequest() { return impl_->consumePreviousItemRequest(); }
bool OpenGLVideoRenderer::consumeOpenFileRequest(std::string& path) { return impl_->consumeOpenFileRequest(path); }
void OpenGLVideoRenderer::setOverlayState(double position, double duration, float volume, bool paused) { impl_->setOverlayState(position, duration, volume, paused); }
void OpenGLVideoRenderer::setSubtitleClock(double subtitle_time_seconds) { impl_->setSubtitleClock(subtitle_time_seconds); }
void OpenGLVideoRenderer::setSubtitleText(const std::string& text) { impl_->setSubtitleText(text); }
void OpenGLVideoRenderer::setSubtitleItems(const std::vector<subtitle::SubtitleItem>& items) { impl_->setSubtitleItems(items); }
void OpenGLVideoRenderer::setSubtitleTrackState(int current_ordinal, int track_count) { impl_->setSubtitleTrackState(current_ordinal, track_count); }
void OpenGLVideoRenderer::setSubtitleTrackCatalog(const std::vector<std::string>& track_labels, int current_ordinal) {
    impl_->setSubtitleTrackCatalog(track_labels, current_ordinal);
}
void OpenGLVideoRenderer::setHotkeyManager(const input::HotkeyManager& hotkey_manager) { impl_->setHotkeyManager(hotkey_manager); }
RendererDiagnostics OpenGLVideoRenderer::getDiagnostics() const { return impl_->getDiagnostics(); }
void OpenGLVideoRenderer::resetDiagnostics() { impl_->resetDiagnostics(); }
bool OpenGLVideoRenderer::supportsNativeFrameFormat(AVPixelFormat format) const { return impl_->supportsNativeFrameFormat(format); }
bool OpenGLVideoRenderer::supportsDirectFrameFormat(AVPixelFormat format) const { return impl_->supportsDirectFrameFormat(format); }
void* OpenGLVideoRenderer::nativeDeviceHandle() const { return impl_->nativeDeviceHandle(); }
const char* OpenGLVideoRenderer::rendererBackendName() const { return "OpenGL"; }

}  // namespace vp::render

