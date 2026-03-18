#include "render/d3d11_video_renderer.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#if __has_include(<SDL2/SDL.h>)
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#elif __has_include(<SDL.h>)
#include <SDL.h>
#include <SDL_syswm.h>
#else
#error "SDL2 headers not found"
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <d3d11.h>
#include <d3d11_1.h>
#include <d3d11_3.h>
#include <d3d11_4.h>
#include <d2d1_1.h>
#include <d2d1helper.h>
#include <d3dcompiler.h>
#include <dwrite.h>
#include <dxgi1_2.h>
#include <wrl/client.h>

extern "C" {
#include <libavutil/hwcontext_d3d11va.h>
#include <libavutil/pixdesc.h>
}

#include "logger.h"

namespace vp::render {

using Microsoft::WRL::ComPtr;

namespace {

constexpr int kMinWindowWidth = 320;
constexpr int kMinWindowHeight = 180;
constexpr int kControlPanelInset = 8;
constexpr int kControlPanelHeight = 44;
constexpr int kControlPadding = 10;
constexpr int kControlGap = 14;
constexpr int kBarHeight = 8;
constexpr int kVolumeBarWidth = 96;
constexpr int kMinProgressBarWidth = 80;
constexpr float kVolumeStep = 0.05f;
constexpr double kSeekStepSeconds = 5.0;
constexpr double kSeekStepSecondsCtrl = 30.0;
constexpr double kSpeedStep = 0.1;
constexpr float kMinSubtitleFontSize = 20.0f;
constexpr float kMaxSubtitleFontSize = 72.0f;
constexpr float kSubtitleFontScale = 0.055f;
constexpr float kSubtitleHorizontalPadding = 18.0f;
constexpr float kSubtitleVerticalPadding = 10.0f;
constexpr float kSubtitleShadowOffset = 2.0f;

struct WindowSize {
    int width;
    int height;
};

struct VideoVertex {
    float x;
    float y;
    float u;
    float v;
};

struct ColorVertex {
    float x;
    float y;
    float r;
    float g;
    float b;
    float a;
};

struct Color {
    float r;
    float g;
    float b;
    float a;
};

struct ColorMatrixConstants {
    float coeff_r[4];
    float coeff_g[4];
    float coeff_b[4];
    float reserved[4];
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

bool isLimitedRange(const AVFrame* frame) {
    return !frame || frame->color_range != AVCOL_RANGE_JPEG;
}

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

ColorMatrixConstants buildColorMatrix(const AVFrame* frame, bool high_bit_depth) {
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

HRESULT compileShader(const char* source, const char* entry, const char* profile, ComPtr<ID3DBlob>& blob) {
    UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(DEBUG_MODE)
    flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
    ComPtr<ID3DBlob> errors;
    const HRESULT hr = D3DCompile(source, std::strlen(source), nullptr, nullptr, nullptr, entry, profile, flags, 0,
                                  blob.GetAddressOf(), errors.GetAddressOf());
    if (FAILED(hr) && errors) {
        LOG_ERROR("D3D11 shader compile failed: " << static_cast<const char*>(errors->GetBufferPointer()));
    }
    return hr;
}

std::wstring decodeSubtitleText(const std::string& text) {
    if (text.empty()) {
        return {};
    }

    auto convert = [&](UINT code_page, DWORD flags) -> std::wstring {
        const int wide_length = MultiByteToWideChar(code_page,
                                                    flags,
                                                    text.data(),
                                                    static_cast<int>(text.size()),
                                                    nullptr,
                                                    0);
        if (wide_length <= 0) {
            return {};
        }
        std::wstring wide(static_cast<size_t>(wide_length), L'\0');
        const int converted = MultiByteToWideChar(code_page,
                                                  flags,
                                                  text.data(),
                                                  static_cast<int>(text.size()),
                                                  wide.data(),
                                                  wide_length);
        if (converted != wide_length) {
            return {};
        }
        return wide;
    };

    std::wstring wide = convert(CP_UTF8, MB_ERR_INVALID_CHARS);
    if (!wide.empty()) {
        return wide;
    }
    return convert(CP_ACP, 0);
}

size_t countUtf8CodePoints(const std::string& text) {
    size_t count = 0;
    for (unsigned char ch : text) {
        if ((ch & 0xC0u) != 0x80u) {
            ++count;
        }
    }
    return count;
}

subtitle::SubtitleStyle makePlainSubtitleStyle() {
    subtitle::SubtitleStyle style;
    style.style_name = "plain";
    style.font_family = "Microsoft YaHei UI";
    style.font_size = 36.0;
    style.bold = true;
    style.primary_color = subtitle::SubtitleColor(255, 255, 255, 250);
    style.outline_color = subtitle::SubtitleColor(0, 0, 0, 255);
    style.background_color = subtitle::SubtitleColor(5, 5, 5, 148);
    style.alignment = 2;
    style.margin_l = 24;
    style.margin_r = 24;
    style.margin_v = 24;
    style.border_style = 3;
    style.outline = 0.0;
    style.shadow = 2.0;
    return style;
}

int clampSubtitleAlignment(int alignment) {
    return alignment >= 1 && alignment <= 9 ? alignment : 2;
}

int subtitleHorizontalGroup(int alignment) {
    return (clampSubtitleAlignment(alignment) - 1) % 3;
}

int subtitleVerticalGroup(int alignment) {
    return (clampSubtitleAlignment(alignment) - 1) / 3;
}

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

float scaleSubtitleMetric(double value, int script_extent, float target_extent, float fallback) {
    if (script_extent > 0 && value > 0.0) {
        return static_cast<float>(value * static_cast<double>(target_extent) / static_cast<double>(script_extent));
    }
    if (value > 0.0) {
        return static_cast<float>(value);
    }
    return fallback;
}

}  // namespace

class D3D11VideoRenderer::Impl {
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
    bool supportsNativeFrameFormat(AVPixelFormat format) const;
    void* nativeDeviceHandle() const;

private:
    struct ControlLayout {
        SDL_Rect panel;
        SDL_Rect progress_track;
        SDL_Rect volume_track;
    };

    bool createDeviceResources();
    bool createSwapChainForWindow();
    bool createBackBuffer();
    bool createShaders();
    bool createBlendState();
    bool createTextResources();
    bool createTextTarget();
    bool ensureSubtitleTextFormat(float font_size);
    bool ensureSoftwareTextures(int width, int height);
    bool ensureOverlayBuffer(size_t vertex_count);
    bool resizeSwapChainLocked(int width, int height);
    bool ensureNativeShaderResourcesLocked(ID3D11Texture2D* texture, intptr_t index, DXGI_FORMAT format);
    bool bindSoftwareFrameLocked(const AVFrame* frame);
    bool bindNativeFrameLocked(const AVFrame* frame);
    void appendRect(std::vector<ColorVertex>& vertices, int x, int y, int w, int h, Color color) const;
    ControlLayout computeControlLayout(int window_width, int window_height) const;
    void drawSubtitleLocked(const ControlLayout& layout);
    bool renderLocked();
    void redrawIfPaused();
    void resetRequests();
    void toggleFullscreen();

    SDL_Window* window_{nullptr};
    std::atomic<int> width_{0};
    std::atomic<int> height_{0};
    std::atomic<bool> should_quit_{false};
    std::atomic<bool> fullscreen_{false};
    std::atomic<bool> minimized_{false};
    bool initialized_{false};

    mutable std::mutex render_mutex_;
    mutable std::mutex request_mutex_;
    mutable std::mutex subtitle_mutex_;

    std::atomic<double> overlay_position_{0.0};
    std::atomic<double> overlay_duration_{0.0};
    std::atomic<float> overlay_volume_{1.0f};
    std::atomic<bool> overlay_paused_{false};
    std::vector<subtitle::SubtitleItem> subtitle_items_;
    input::HotkeyManager hotkey_manager_{};

    bool toggle_pause_requested_{false};
    bool seek_requested_{false};
    double seek_ratio_{0.0};
    bool seek_delta_requested_{false};
    double seek_delta_seconds_{0.0};
    bool volume_change_requested_{false};
    float requested_volume_{1.0f};
    bool speed_change_requested_{false};
    double speed_delta_{0.0};
    bool speed_reset_requested_{false};
    bool subtitle_toggle_requested_{false};
    bool ab_repeat_start_requested_{false};
    bool ab_repeat_end_requested_{false};
    bool ab_repeat_clear_requested_{false};
    bool screenshot_requested_{false};
    bool step_frame_backward_requested_{false};
    bool step_frame_forward_requested_{false};
    bool subtitle_delay_change_requested_{false};
    double subtitle_delay_delta_seconds_{0.0};
    bool audio_delay_change_requested_{false};
    double audio_delay_delta_seconds_{0.0};
    bool next_chapter_requested_{false};
    bool previous_chapter_requested_{false};
    bool next_item_requested_{false};
    bool previous_item_requested_{false};
    float last_nonzero_volume_{1.0f};
    bool dragging_seek_{false};
    bool dragging_volume_{false};
    bool seek_preview_active_{false};
    double seek_preview_ratio_{0.0};

    AVFrame* current_frame_{nullptr};
    bool frame_available_{false};

    ComPtr<ID3D11Device> device_;
    ComPtr<ID3D11DeviceContext> context_;
    ComPtr<ID3D11Device3> device3_;
    ComPtr<IDXGISwapChain1> swap_chain_;
    ComPtr<ID3D11RenderTargetView> render_target_view_;
    ComPtr<ID3D11VertexShader> video_vertex_shader_;
    ComPtr<ID3D11PixelShader> yuv420_pixel_shader_;
    ComPtr<ID3D11PixelShader> nv12_pixel_shader_;
    ComPtr<ID3D11InputLayout> video_input_layout_;
    ComPtr<ID3D11Buffer> video_vertex_buffer_;
    ComPtr<ID3D11Buffer> color_matrix_buffer_;
    ComPtr<ID3D11SamplerState> sampler_state_;
    ComPtr<ID3D11VertexShader> color_vertex_shader_;
    ComPtr<ID3D11PixelShader> color_pixel_shader_;
    ComPtr<ID3D11InputLayout> color_input_layout_;
    ComPtr<ID3D11Buffer> overlay_vertex_buffer_;
    size_t overlay_vertex_capacity_{0};
    ComPtr<ID3D11BlendState> alpha_blend_state_;
    ComPtr<ID2D1Factory1> d2d_factory_;
    ComPtr<ID2D1Device> d2d_device_;
    ComPtr<ID2D1DeviceContext> d2d_context_;
    ComPtr<ID2D1Bitmap1> d2d_target_bitmap_;
    ComPtr<IDWriteFactory> dwrite_factory_;
    ComPtr<IDWriteTextFormat> subtitle_text_format_;
    ComPtr<ID2D1SolidColorBrush> subtitle_fill_brush_;
    ComPtr<ID2D1SolidColorBrush> subtitle_shadow_brush_;
    ComPtr<ID2D1SolidColorBrush> subtitle_background_brush_;
    float subtitle_font_size_{0.0f};

    int software_width_{0};
    int software_height_{0};
    ComPtr<ID3D11Texture2D> tex_y_;
    ComPtr<ID3D11Texture2D> tex_u_;
    ComPtr<ID3D11Texture2D> tex_v_;
    ComPtr<ID3D11ShaderResourceView> srv_y_;
    ComPtr<ID3D11ShaderResourceView> srv_u_;
    ComPtr<ID3D11ShaderResourceView> srv_v_;

    ID3D11Texture2D* native_texture_ptr_{nullptr};
    intptr_t native_texture_index_{0};
    DXGI_FORMAT native_texture_format_{DXGI_FORMAT_UNKNOWN};
    ComPtr<ID3D11ShaderResourceView1> native_srv_y_;
    ComPtr<ID3D11ShaderResourceView1> native_srv_uv_;
};

bool D3D11VideoRenderer::Impl::init(const VideoRendererConfig& config) {
    close();
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        LOG_ERROR("SDL init failed: " << SDL_GetError());
        return false;
    }

    const WindowSize window_size = computeInitialWindowSize(config.width, config.height);
    width_.store(window_size.width);
    height_.store(window_size.height);
    window_ = SDL_CreateWindow(config.title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                               width_.load(), height_.load(), SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!window_) {
        LOG_ERROR("SDL window creation failed: " << SDL_GetError());
        SDL_Quit();
        return false;
    }
    SDL_SetWindowMinimumSize(window_, kMinWindowWidth, kMinWindowHeight);

    current_frame_ = av_frame_alloc();
    if (!current_frame_) {
        LOG_ERROR("Failed to allocate render frame cache");
        close();
        return false;
    }

    resetRequests();
    if (!createDeviceResources() || !createTextResources() || !createSwapChainForWindow() || !createBackBuffer() || !createShaders() ||
        !createBlendState()) {
        close();
        return false;
    }
    initialized_ = true;
    LOG_INFO("Native D3D11 renderer initialized: window=" << width_.load() << "x" << height_.load());
    return true;
}

void D3D11VideoRenderer::Impl::close() {
    std::lock_guard<std::mutex> render_lock(render_mutex_);
    initialized_ = false;
    frame_available_ = false;
    native_texture_ptr_ = nullptr;
    native_texture_index_ = 0;
    native_texture_format_ = DXGI_FORMAT_UNKNOWN;
    native_srv_y_.Reset();
    native_srv_uv_.Reset();
    tex_y_.Reset();
    tex_u_.Reset();
    tex_v_.Reset();
    srv_y_.Reset();
    srv_u_.Reset();
    srv_v_.Reset();
    subtitle_font_size_ = 0.0f;
    if (d2d_context_) {
        d2d_context_->SetTarget(nullptr);
    }
    subtitle_background_brush_.Reset();
    subtitle_shadow_brush_.Reset();
    subtitle_fill_brush_.Reset();
    subtitle_text_format_.Reset();
    d2d_target_bitmap_.Reset();
    dwrite_factory_.Reset();
    d2d_context_.Reset();
    d2d_device_.Reset();
    d2d_factory_.Reset();
    alpha_blend_state_.Reset();
    overlay_vertex_buffer_.Reset();
    color_input_layout_.Reset();
    color_pixel_shader_.Reset();
    color_vertex_shader_.Reset();
    sampler_state_.Reset();
    color_matrix_buffer_.Reset();
    video_vertex_buffer_.Reset();
    video_input_layout_.Reset();
    yuv420_pixel_shader_.Reset();
    nv12_pixel_shader_.Reset();
    video_vertex_shader_.Reset();
    render_target_view_.Reset();
    swap_chain_.Reset();
    context_.Reset();
    device3_.Reset();
    device_.Reset();
    if (current_frame_) {
        av_frame_free(&current_frame_);
    }
    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }
    SDL_Quit();
}

bool D3D11VideoRenderer::Impl::createDeviceResources() {
    UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if defined(DEBUG_MODE)
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
    const D3D_FEATURE_LEVEL requested_levels[] = {D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0};
    D3D_FEATURE_LEVEL actual_level = D3D_FEATURE_LEVEL_11_0;
    HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags, requested_levels,
                                   2u, D3D11_SDK_VERSION,
                                   device_.GetAddressOf(), &actual_level, context_.GetAddressOf());
#if defined(DEBUG_MODE)
    if (FAILED(hr)) {
        flags &= ~D3D11_CREATE_DEVICE_DEBUG;
        hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags, requested_levels,
                               2u, D3D11_SDK_VERSION,
                               device_.GetAddressOf(), &actual_level, context_.GetAddressOf());
    }
#endif
    if (FAILED(hr) || !device_ || !context_) {
        LOG_ERROR("D3D11CreateDevice failed");
        return false;
    }
    device_.As(&device3_);
    ComPtr<ID3D11Multithread> multithread;
    if (SUCCEEDED(context_.As(&multithread)) && multithread) {
        multithread->SetMultithreadProtected(TRUE);
    }
    D3D11_SAMPLER_DESC sampler_desc{};
    sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.MaxLOD = D3D11_FLOAT32_MAX;
    if (FAILED(device_->CreateSamplerState(&sampler_desc, sampler_state_.GetAddressOf()))) {
        LOG_ERROR("CreateSamplerState failed");
        return false;
    }
    return true;
}

bool D3D11VideoRenderer::Impl::createSwapChainForWindow() {
    SDL_SysWMinfo wm_info{};
    SDL_VERSION(&wm_info.version);
    if (!SDL_GetWindowWMInfo(window_, &wm_info)) {
        LOG_ERROR("SDL_GetWindowWMInfo failed: " << SDL_GetError());
        return false;
    }
    HWND hwnd = wm_info.info.win.window;
    if (!hwnd) {
        LOG_ERROR("Failed to resolve HWND for SDL window");
        return false;
    }

    ComPtr<IDXGIDevice> dxgi_device;
    ComPtr<IDXGIAdapter> adapter;
    ComPtr<IDXGIFactory2> factory;
    if (FAILED(device_.As(&dxgi_device)) || FAILED(dxgi_device->GetAdapter(adapter.GetAddressOf())) ||
        FAILED(adapter->GetParent(IID_PPV_ARGS(factory.GetAddressOf())))) {
        LOG_ERROR("Failed to resolve DXGI factory for D3D11 renderer");
        return false;
    }

    DXGI_SWAP_CHAIN_DESC1 swap_desc{};
    swap_desc.Width = static_cast<UINT>(std::max(1, width_.load()));
    swap_desc.Height = static_cast<UINT>(std::max(1, height_.load()));
    swap_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    swap_desc.SampleDesc.Count = 1;
    swap_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swap_desc.BufferCount = 2;
    swap_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swap_desc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
    if (FAILED(factory->CreateSwapChainForHwnd(device_.Get(), hwnd, &swap_desc, nullptr, nullptr,
                                               swap_chain_.GetAddressOf()))) {
        LOG_ERROR("CreateSwapChainForHwnd failed");
        return false;
    }
    factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);
    return true;
}

bool D3D11VideoRenderer::Impl::createBackBuffer() {
    render_target_view_.Reset();
    ComPtr<ID3D11Texture2D> back_buffer;
    if (FAILED(swap_chain_->GetBuffer(0, IID_PPV_ARGS(back_buffer.GetAddressOf()))) ||
        FAILED(device_->CreateRenderTargetView(back_buffer.Get(), nullptr, render_target_view_.GetAddressOf()))) {
        LOG_ERROR("Failed to create D3D11 render target view");
        return false;
    }
    return createTextTarget();
}


bool D3D11VideoRenderer::Impl::resizeSwapChainLocked(int width, int height) {
    if (!swap_chain_) {
        return false;
    }
    context_->OMSetRenderTargets(0, nullptr, nullptr);
    if (d2d_context_) {
        d2d_context_->SetTarget(nullptr);
    }
    d2d_target_bitmap_.Reset();
    render_target_view_.Reset();
    const HRESULT hr = swap_chain_->ResizeBuffers(0, static_cast<UINT>(std::max(1, width)),
                                                  static_cast<UINT>(std::max(1, height)), DXGI_FORMAT_UNKNOWN, 0);
    if (FAILED(hr)) {
        LOG_ERROR("ResizeBuffers failed");
        return false;
    }
    return createBackBuffer();
}


bool D3D11VideoRenderer::Impl::createShaders() {
    static const char* kVideoVs = R"(
struct VSIn { float2 pos : POSITION; float2 uv : TEXCOORD0; };
struct VSOut { float4 pos : SV_POSITION; float2 uv : TEXCOORD0; };
VSOut main(VSIn input) { VSOut output; output.pos = float4(input.pos, 0.0, 1.0); output.uv = input.uv; return output; }
)";
    static const char* kYuv420Ps = R"(
cbuffer ColorMatrix : register(b0) { float4 coeffR; float4 coeffG; float4 coeffB; float4 reserved; };
Texture2D texY : register(t0); Texture2D texU : register(t1); Texture2D texV : register(t2); SamplerState samp : register(s0);
struct PSIn { float4 pos : SV_POSITION; float2 uv : TEXCOORD0; };
float4 main(PSIn input) : SV_Target {
    float y = texY.Sample(samp, input.uv).r;
    float u = texU.Sample(samp, input.uv).r;
    float v = texV.Sample(samp, input.uv).r;
    float3 rgb = float3(dot(float4(y, u, v, 1.0), coeffR), dot(float4(y, u, v, 1.0), coeffG), dot(float4(y, u, v, 1.0), coeffB));
    return float4(saturate(rgb), 1.0);
}
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
    static const char* kColorVs = R"(
struct VSIn { float2 pos : POSITION; float4 color : COLOR0; };
struct VSOut { float4 pos : SV_POSITION; float4 color : COLOR0; };
VSOut main(VSIn input) { VSOut output; output.pos = float4(input.pos, 0.0, 1.0); output.color = input.color; return output; }
)";
    static const char* kColorPs = R"(
struct PSIn { float4 pos : SV_POSITION; float4 color : COLOR0; };
float4 main(PSIn input) : SV_Target { return input.color; }
)";

    ComPtr<ID3DBlob> blob;
    if (FAILED(compileShader(kVideoVs, "main", "vs_4_0", blob)) ||
        FAILED(device_->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr,
                                           video_vertex_shader_.GetAddressOf()))) {
        return false;
    }
    const D3D11_INPUT_ELEMENT_DESC video_layout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    if (FAILED(device_->CreateInputLayout(video_layout, 2, blob->GetBufferPointer(), blob->GetBufferSize(),
                                          video_input_layout_.GetAddressOf()))) {
        return false;
    }
    if (FAILED(compileShader(kYuv420Ps, "main", "ps_4_0", blob)) ||
        FAILED(device_->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr,
                                          yuv420_pixel_shader_.GetAddressOf()))) {
        return false;
    }
    if (FAILED(compileShader(kNv12Ps, "main", "ps_4_0", blob)) ||
        FAILED(device_->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr,
                                          nv12_pixel_shader_.GetAddressOf()))) {
        return false;
    }
    if (FAILED(compileShader(kColorVs, "main", "vs_4_0", blob)) ||
        FAILED(device_->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr,
                                           color_vertex_shader_.GetAddressOf()))) {
        return false;
    }
    const D3D11_INPUT_ELEMENT_DESC color_layout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    if (FAILED(device_->CreateInputLayout(color_layout, 2, blob->GetBufferPointer(), blob->GetBufferSize(),
                                          color_input_layout_.GetAddressOf()))) {
        return false;
    }
    if (FAILED(compileShader(kColorPs, "main", "ps_4_0", blob)) ||
        FAILED(device_->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr,
                                          color_pixel_shader_.GetAddressOf()))) {
        return false;
    }

    D3D11_BUFFER_DESC video_vb_desc{};
    video_vb_desc.ByteWidth = sizeof(VideoVertex) * 6;
    video_vb_desc.Usage = D3D11_USAGE_DYNAMIC;
    video_vb_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    video_vb_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    if (FAILED(device_->CreateBuffer(&video_vb_desc, nullptr, video_vertex_buffer_.GetAddressOf()))) {
        return false;
    }

    D3D11_BUFFER_DESC constant_desc{};
    constant_desc.ByteWidth = sizeof(ColorMatrixConstants);
    constant_desc.Usage = D3D11_USAGE_DEFAULT;
    constant_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    if (FAILED(device_->CreateBuffer(&constant_desc, nullptr, color_matrix_buffer_.GetAddressOf()))) {
        return false;
    }
    return true;
}

bool D3D11VideoRenderer::Impl::createBlendState() {
    D3D11_BLEND_DESC desc{};
    desc.RenderTarget[0].BlendEnable = TRUE;
    desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    return SUCCEEDED(device_->CreateBlendState(&desc, alpha_blend_state_.GetAddressOf()));
}

bool D3D11VideoRenderer::Impl::createTextResources() {
    D2D1_FACTORY_OPTIONS factory_options{};
#if defined(DEBUG_MODE)
    factory_options.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#endif
    HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,
                                   __uuidof(ID2D1Factory1),
                                   &factory_options,
                                   reinterpret_cast<void**>(d2d_factory_.GetAddressOf()));
    if (FAILED(hr) || !d2d_factory_) {
        LOG_ERROR("Failed to create D2D factory for subtitle rendering");
        return false;
    }

    ComPtr<IDXGIDevice> dxgi_device;
    if (FAILED(device_.As(&dxgi_device)) ||
        FAILED(d2d_factory_->CreateDevice(dxgi_device.Get(), d2d_device_.GetAddressOf())) ||
        FAILED(d2d_device_->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, d2d_context_.GetAddressOf()))) {
        LOG_ERROR("Failed to create D2D device context for subtitle rendering");
        return false;
    }

    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,
                             __uuidof(IDWriteFactory),
                             reinterpret_cast<IUnknown**>(dwrite_factory_.GetAddressOf()));
    if (FAILED(hr) || !dwrite_factory_) {
        LOG_ERROR("Failed to create DirectWrite factory for subtitle rendering");
        return false;
    }

    if (FAILED(d2d_context_->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.98f), subtitle_fill_brush_.GetAddressOf())) ||
        FAILED(d2d_context_->CreateSolidColorBrush(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.82f), subtitle_shadow_brush_.GetAddressOf())) ||
        FAILED(d2d_context_->CreateSolidColorBrush(D2D1::ColorF(0.02f, 0.02f, 0.02f, 0.58f), subtitle_background_brush_.GetAddressOf()))) {
        LOG_ERROR("Failed to create subtitle brushes for D3D11 renderer");
        return false;
    }

    subtitle_font_size_ = 0.0f;
    return true;
}

bool D3D11VideoRenderer::Impl::createTextTarget() {
    if (!swap_chain_ || !d2d_context_) {
        return false;
    }
    d2d_context_->SetTarget(nullptr);
    d2d_target_bitmap_.Reset();

    ComPtr<IDXGISurface> dxgi_surface;
    if (FAILED(swap_chain_->GetBuffer(0, IID_PPV_ARGS(dxgi_surface.GetAddressOf())))) {
        LOG_ERROR("Failed to acquire DXGI surface for subtitle rendering");
        return false;
    }

    const D2D1_BITMAP_PROPERTIES1 bitmap_props = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE));
    if (FAILED(d2d_context_->CreateBitmapFromDxgiSurface(dxgi_surface.Get(),
                                                         &bitmap_props,
                                                         d2d_target_bitmap_.GetAddressOf())) ||
        !d2d_target_bitmap_) {
        LOG_ERROR("Failed to create D2D target bitmap for subtitle rendering");
        return false;
    }

    d2d_context_->SetTarget(d2d_target_bitmap_.Get());
    d2d_context_->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
    return true;
}

bool D3D11VideoRenderer::Impl::ensureSubtitleTextFormat(float font_size) {
    if (!dwrite_factory_) {
        return false;
    }
    if (subtitle_text_format_ && std::abs(subtitle_font_size_ - font_size) < 0.5f) {
        return true;
    }

    subtitle_text_format_.Reset();
    const auto create_format = [&](const wchar_t* font_family) -> HRESULT {
        return dwrite_factory_->CreateTextFormat(font_family,
                                                 nullptr,
                                                 DWRITE_FONT_WEIGHT_SEMI_BOLD,
                                                 DWRITE_FONT_STYLE_NORMAL,
                                                 DWRITE_FONT_STRETCH_NORMAL,
                                                 font_size,
                                                 L"zh-CN",
                                                 subtitle_text_format_.GetAddressOf());
    };

    HRESULT hr = create_format(L"Microsoft YaHei UI");
    if (FAILED(hr)) {
        subtitle_text_format_.Reset();
        hr = create_format(L"Segoe UI");
    }
    if (FAILED(hr) || !subtitle_text_format_) {
        LOG_ERROR("Failed to create DirectWrite subtitle text format");
        return false;
    }

    subtitle_font_size_ = font_size;
    subtitle_text_format_->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    subtitle_text_format_->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_FAR);
    subtitle_text_format_->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);
    return true;
}


bool D3D11VideoRenderer::Impl::ensureSoftwareTextures(int width, int height) {
    if (width == software_width_ && height == software_height_ && tex_y_ && tex_u_ && tex_v_) {
        return true;
    }
    software_width_ = width;
    software_height_ = height;
    tex_y_.Reset(); tex_u_.Reset(); tex_v_.Reset();
    srv_y_.Reset(); srv_u_.Reset(); srv_v_.Reset();

    auto create_plane = [this](int plane_width, int plane_height, ComPtr<ID3D11Texture2D>& texture,
                               ComPtr<ID3D11ShaderResourceView>& srv) -> bool {
        D3D11_TEXTURE2D_DESC desc{};
        desc.Width = static_cast<UINT>(plane_width);
        desc.Height = static_cast<UINT>(plane_height);
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        if (FAILED(device_->CreateTexture2D(&desc, nullptr, texture.GetAddressOf())) ||
            FAILED(device_->CreateShaderResourceView(texture.Get(), nullptr, srv.GetAddressOf()))) {
            return false;
        }
        return true;
    };

    return create_plane(width, height, tex_y_, srv_y_) &&
           create_plane(std::max(1, (width + 1) / 2), std::max(1, (height + 1) / 2), tex_u_, srv_u_) &&
           create_plane(std::max(1, (width + 1) / 2), std::max(1, (height + 1) / 2), tex_v_, srv_v_);
}

bool D3D11VideoRenderer::Impl::ensureOverlayBuffer(size_t vertex_count) {
    if (vertex_count == 0) {
        return true;
    }
    if (overlay_vertex_buffer_ && vertex_count <= overlay_vertex_capacity_) {
        return true;
    }
    overlay_vertex_capacity_ = std::max<size_t>(vertex_count, overlay_vertex_capacity_ * 2 + 64);
    D3D11_BUFFER_DESC desc{};
    desc.ByteWidth = static_cast<UINT>(overlay_vertex_capacity_ * sizeof(ColorVertex));
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    overlay_vertex_buffer_.Reset();
    return SUCCEEDED(device_->CreateBuffer(&desc, nullptr, overlay_vertex_buffer_.GetAddressOf()));
}

bool D3D11VideoRenderer::Impl::ensureNativeShaderResourcesLocked(ID3D11Texture2D* texture, intptr_t index, DXGI_FORMAT format) {
    if (!device3_ || !texture) {
        return false;
    }
    if (texture == native_texture_ptr_ && index == native_texture_index_ && format == native_texture_format_ &&
        native_srv_y_ && native_srv_uv_) {
        return true;
    }
    native_srv_y_.Reset();
    native_srv_uv_.Reset();
    native_texture_ptr_ = texture;
    native_texture_index_ = index;
    native_texture_format_ = format;

    D3D11_TEXTURE2D_DESC texture_desc{};
    texture->GetDesc(&texture_desc);
    const UINT first_slice = texture_desc.ArraySize > 1 ? static_cast<UINT>(std::max<intptr_t>(0, index)) : 0;
    const bool high_bit_depth = format == DXGI_FORMAT_P010 || format == DXGI_FORMAT_P016;
    const DXGI_FORMAT y_format = high_bit_depth ? DXGI_FORMAT_R16_UNORM : DXGI_FORMAT_R8_UNORM;
    const DXGI_FORMAT uv_format = high_bit_depth ? DXGI_FORMAT_R16G16_UNORM : DXGI_FORMAT_R8G8_UNORM;

    D3D11_SHADER_RESOURCE_VIEW_DESC1 y_desc{};
    y_desc.Format = y_format;
    if (texture_desc.ArraySize > 1) {
        y_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
        y_desc.Texture2DArray.MostDetailedMip = 0;
        y_desc.Texture2DArray.MipLevels = 1;
        y_desc.Texture2DArray.FirstArraySlice = first_slice;
        y_desc.Texture2DArray.ArraySize = 1;
        y_desc.Texture2DArray.PlaneSlice = 0;
    } else {
        y_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        y_desc.Texture2D.MostDetailedMip = 0;
        y_desc.Texture2D.MipLevels = 1;
        y_desc.Texture2D.PlaneSlice = 0;
    }
    D3D11_SHADER_RESOURCE_VIEW_DESC1 uv_desc = y_desc;
    uv_desc.Format = uv_format;
    if (texture_desc.ArraySize > 1) {
        uv_desc.Texture2DArray.PlaneSlice = 1;
    } else {
        uv_desc.Texture2D.PlaneSlice = 1;
    }
    return SUCCEEDED(device3_->CreateShaderResourceView1(texture, &y_desc, native_srv_y_.GetAddressOf())) &&
           SUCCEEDED(device3_->CreateShaderResourceView1(texture, &uv_desc, native_srv_uv_.GetAddressOf()));
}

bool D3D11VideoRenderer::Impl::bindSoftwareFrameLocked(const AVFrame* frame) {
    if (!frame || !frame->data[0] || !frame->data[1] || !frame->data[2] || frame->width <= 0 || frame->height <= 0) {
        return false;
    }
    if (!ensureSoftwareTextures(frame->width, frame->height)) {
        return false;
    }
    context_->UpdateSubresource(tex_y_.Get(), 0, nullptr, frame->data[0], frame->linesize[0], 0);
    context_->UpdateSubresource(tex_u_.Get(), 0, nullptr, frame->data[1], frame->linesize[1], 0);
    context_->UpdateSubresource(tex_v_.Get(), 0, nullptr, frame->data[2], frame->linesize[2], 0);
    const ColorMatrixConstants matrix = buildColorMatrix(frame, false);
    context_->UpdateSubresource(color_matrix_buffer_.Get(), 0, nullptr, &matrix, 0, 0);

    ID3D11ShaderResourceView* srvs[] = {srv_y_.Get(), srv_u_.Get(), srv_v_.Get()};
    context_->PSSetShader(yuv420_pixel_shader_.Get(), nullptr, 0);
    context_->PSSetShaderResources(0, 3, srvs);
    context_->PSSetConstantBuffers(0, 1, color_matrix_buffer_.GetAddressOf());
    return true;
}

bool D3D11VideoRenderer::Impl::bindNativeFrameLocked(const AVFrame* frame) {
    if (!frame || !device3_ || frame->format != AV_PIX_FMT_D3D11 || !frame->data[0]) {
        return false;
    }
    auto* texture = reinterpret_cast<ID3D11Texture2D*>(frame->data[0]);
    const intptr_t index = reinterpret_cast<intptr_t>(frame->data[1]);
    D3D11_TEXTURE2D_DESC desc{};
    texture->GetDesc(&desc);
    const bool supported = desc.Format == DXGI_FORMAT_NV12 || desc.Format == DXGI_FORMAT_P010 || desc.Format == DXGI_FORMAT_P016;
    if (!supported) {
        LOG_WARNING("D3D11 native frame format not supported for direct present: " << static_cast<int>(desc.Format));
        return false;
    }
    if (!ensureNativeShaderResourcesLocked(texture, index, desc.Format)) {
        return false;
    }
    const ColorMatrixConstants matrix = buildColorMatrix(frame, desc.Format == DXGI_FORMAT_P010 || desc.Format == DXGI_FORMAT_P016);
    context_->UpdateSubresource(color_matrix_buffer_.Get(), 0, nullptr, &matrix, 0, 0);
    ID3D11ShaderResourceView* srvs[] = {native_srv_y_.Get(), native_srv_uv_.Get()};
    context_->PSSetShader(nv12_pixel_shader_.Get(), nullptr, 0);
    context_->PSSetShaderResources(0, 2, srvs);
    context_->PSSetConstantBuffers(0, 1, color_matrix_buffer_.GetAddressOf());
    return true;
}

void D3D11VideoRenderer::Impl::appendRect(std::vector<ColorVertex>& vertices, int x, int y, int w, int h, Color color) const {
    const int window_width = std::max(1, width_.load());
    const int window_height = std::max(1, height_.load());
    if (w <= 0 || h <= 0) {
        return;
    }
    const float left = (static_cast<float>(x) / static_cast<float>(window_width)) * 2.0f - 1.0f;
    const float right = (static_cast<float>(x + w) / static_cast<float>(window_width)) * 2.0f - 1.0f;
    const float top = 1.0f - (static_cast<float>(y) / static_cast<float>(window_height)) * 2.0f;
    const float bottom = 1.0f - (static_cast<float>(y + h) / static_cast<float>(window_height)) * 2.0f;
    vertices.push_back({left, top, color.r, color.g, color.b, color.a});
    vertices.push_back({right, top, color.r, color.g, color.b, color.a});
    vertices.push_back({left, bottom, color.r, color.g, color.b, color.a});
    vertices.push_back({left, bottom, color.r, color.g, color.b, color.a});
    vertices.push_back({right, top, color.r, color.g, color.b, color.a});
    vertices.push_back({right, bottom, color.r, color.g, color.b, color.a});
}

D3D11VideoRenderer::Impl::ControlLayout D3D11VideoRenderer::Impl::computeControlLayout(int window_width, int window_height) const {
    const int safe_width = std::max(1, window_width);
    const int safe_height = std::max(1, window_height);
    ControlLayout layout{};
    layout.panel = {kControlPanelInset, std::max(0, safe_height - kControlPanelHeight - kControlPanelInset),
                    std::max(1, safe_width - (kControlPanelInset * 2)), kControlPanelHeight};
    const int track_y = layout.panel.y + (layout.panel.h - kBarHeight) / 2;
    const int volume_width = std::min(kVolumeBarWidth, std::max(56, layout.panel.w / 5));
    int progress_width = layout.panel.w - (kControlPadding * 2) - volume_width - kControlGap;
    progress_width = std::max(kMinProgressBarWidth, progress_width);
    layout.progress_track = {layout.panel.x + kControlPadding, track_y,
                             std::min(progress_width, std::max(1, layout.panel.w - (kControlPadding * 2))), kBarHeight};
    layout.volume_track = {layout.panel.x + layout.panel.w - kControlPadding - volume_width, track_y,
                           std::max(1, volume_width), kBarHeight};
    return layout;
}

void D3D11VideoRenderer::Impl::drawSubtitleLocked(const ControlLayout& layout) {
    if (!d2d_context_ || !dwrite_factory_ || !d2d_target_bitmap_) {
        return;
    }

    std::vector<subtitle::SubtitleItem> subtitle_items;
    {
        std::lock_guard<std::mutex> subtitle_lock(subtitle_mutex_);
        subtitle_items = subtitle_items_;
    }
    if (subtitle_items.empty()) {
        return;
    }

    std::stable_sort(subtitle_items.begin(), subtitle_items.end(), [](const subtitle::SubtitleItem& lhs, const subtitle::SubtitleItem& rhs) {
        if (lhs.layer != rhs.layer) {
            return lhs.layer < rhs.layer;
        }
        return lhs.index < rhs.index;
    });

    const int window_width = std::max(1, width_.load());
    const int window_height = std::max(1, height_.load());
    SDL_Rect video_rect{0, 0, window_width, window_height};
    if (current_frame_ && current_frame_->width > 0 && current_frame_->height > 0) {
        video_rect = computeRenderRect(window_width, window_height, current_frame_->width, current_frame_->height);
    }

    const float fallback_margin_x = std::max(24.0f, static_cast<float>(video_rect.w) * 0.06f);
    const float fallback_margin_y = std::max(18.0f, static_cast<float>(video_rect.h) * 0.05f);
    float bottom_limit = static_cast<float>(video_rect.y + video_rect.h) - fallback_margin_y;
    if (layout.panel.y > video_rect.y) {
        bottom_limit = std::min(bottom_limit, static_cast<float>(layout.panel.y) - fallback_margin_y);
    }
    const float top_limit = static_cast<float>(video_rect.y) + fallback_margin_y;
    if (bottom_limit <= top_limit) {
        return;
    }

    float next_bottom_y = bottom_limit;
    float next_top_y = top_limit;
    float next_middle_y = top_limit + (bottom_limit - top_limit) * 0.5f;
    const float available_height = std::max(1.0f, bottom_limit - top_limit);
    const float fallback_font_size = std::clamp(static_cast<float>(video_rect.h) * kSubtitleFontScale,
                                                kMinSubtitleFontSize,
                                                kMaxSubtitleFontSize);

    d2d_context_->SetTarget(d2d_target_bitmap_.Get());
    d2d_context_->SetTransform(D2D1::Matrix3x2F::Identity());
    d2d_context_->BeginDraw();

    bool recreate_target = false;
    for (const subtitle::SubtitleItem& item : subtitle_items) {
        if (item.text.empty()) {
            continue;
        }

        const std::wstring text = decodeSubtitleText(item.text);
        if (text.empty()) {
            continue;
        }

        const int alignment = clampSubtitleAlignment(item.style.alignment);
        const float scale_x = item.play_res_x > 0 ? static_cast<float>(video_rect.w) / static_cast<float>(item.play_res_x) : 1.0f;
        const float scale_y = item.play_res_y > 0 ? static_cast<float>(video_rect.h) / static_cast<float>(item.play_res_y) : 1.0f;
        const float average_scale = (scale_x + scale_y) * 0.5f;

        const float margin_left = item.play_res_x > 0
            ? std::max(4.0f, static_cast<float>(item.style.margin_l) * scale_x)
            : std::max(fallback_margin_x, static_cast<float>(item.style.margin_l));
        const float margin_right = item.play_res_x > 0
            ? std::max(4.0f, static_cast<float>(item.style.margin_r) * scale_x)
            : std::max(fallback_margin_x, static_cast<float>(item.style.margin_r));

        float font_size = fallback_font_size;
        if (item.play_res_y > 0 && item.style.font_size > 0.0) {
            font_size = static_cast<float>(item.style.font_size) * scale_y;
        } else if (item.style.font_size > 0.0) {
            font_size = std::max(fallback_font_size, static_cast<float>(item.style.font_size));
        }
        font_size = std::clamp(font_size, 10.0f, std::max(36.0f, static_cast<float>(video_rect.h) * 0.35f));

        const float available_width = std::max(48.0f, static_cast<float>(video_rect.w) - margin_left - margin_right);
        float layout_width = available_width;
        if (item.style.has_position) {
            layout_width = std::min(available_width, std::max(font_size * 8.0f, available_width * 0.45f));
        }

        ComPtr<IDWriteTextFormat> text_format;
        const auto create_text_format = [&](const wchar_t* font_family) -> HRESULT {
            return dwrite_factory_->CreateTextFormat(font_family,
                                                     nullptr,
                                                     item.style.bold ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL,
                                                     item.style.italic ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL,
                                                     DWRITE_FONT_STRETCH_NORMAL,
                                                     font_size,
                                                     L"zh-CN",
                                                     text_format.GetAddressOf());
        };

        HRESULT hr = E_FAIL;
        const std::wstring preferred_font = decodeSubtitleText(item.style.font_family);
        if (!preferred_font.empty()) {
            hr = create_text_format(preferred_font.c_str());
        }
        if (FAILED(hr)) {
            text_format.Reset();
            hr = create_text_format(L"Microsoft YaHei UI");
        }
        if (FAILED(hr)) {
            text_format.Reset();
            hr = create_text_format(L"Segoe UI");
        }
        if (FAILED(hr) || !text_format) {
            continue;
        }

        text_format->SetTextAlignment(toTextAlignment(alignment));
        text_format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
        text_format->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);

        ComPtr<IDWriteTextLayout> text_layout;
        hr = dwrite_factory_->CreateTextLayout(text.c_str(),
                                               static_cast<UINT32>(text.size()),
                                               text_format.Get(),
                                               layout_width,
                                               available_height,
                                               text_layout.GetAddressOf());
        if (FAILED(hr) || !text_layout) {
            continue;
        }

        const DWRITE_TEXT_RANGE full_range{0, static_cast<UINT32>(text.size())};
        text_layout->SetUnderline(item.style.underline, full_range);
        text_layout->SetStrikethrough(item.style.strikeout, full_range);

        for (const subtitle::SubtitleTextRun& run : item.runs) {
            if (run.length == 0) {
                continue;
            }
            const UINT32 start_index = static_cast<UINT32>(std::min(run.start, static_cast<size_t>(text.size())));
            const UINT32 length = static_cast<UINT32>(std::min(run.length, static_cast<size_t>(text.size()) - start_index));
            if (length == 0) {
                continue;
            }
            const DWRITE_TEXT_RANGE range{start_index, length};
            const std::wstring run_font = decodeSubtitleText(run.style.font_family);
            if (!run_font.empty()) {
                text_layout->SetFontFamilyName(run_font.c_str(), range);
            }
            float run_font_size = font_size;
            if (run.style.font_size > 0.0) {
                run_font_size = item.play_res_y > 0
                    ? static_cast<float>(run.style.font_size) * scale_y
                    : static_cast<float>(run.style.font_size);
                run_font_size = std::clamp(run_font_size,
                                           10.0f,
                                           std::max(36.0f, static_cast<float>(video_rect.h) * 0.35f));
            }
            text_layout->SetFontSize(run_font_size, range);
            text_layout->SetFontWeight(run.style.bold ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL, range);
            text_layout->SetFontStyle(run.style.italic ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL, range);
            text_layout->SetUnderline(run.style.underline, range);
            text_layout->SetStrikethrough(run.style.strikeout, range);
        }

        DWRITE_TEXT_METRICS metrics{};
        if (FAILED(text_layout->GetMetrics(&metrics))) {
            continue;
        }

        const float text_height = std::max(1.0f, metrics.height);
        const float gap = std::max(6.0f, font_size * 0.22f);

        float draw_x = static_cast<float>(video_rect.x) + margin_left;
        if (item.style.has_position) {
            const float anchor_x = static_cast<float>(video_rect.x) + static_cast<float>(item.style.position_x) * scale_x;
            switch (subtitleHorizontalGroup(alignment)) {
            case 0:
                draw_x = anchor_x;
                break;
            case 2:
                draw_x = anchor_x - layout_width;
                break;
            default:
                draw_x = anchor_x - layout_width * 0.5f;
                break;
            }
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
        draw_x = std::clamp(draw_x,
                            static_cast<float>(video_rect.x),
                            static_cast<float>(video_rect.x + video_rect.w) - layout_width);

        float draw_y = top_limit;
        if (item.style.has_position) {
            const float anchor_y = static_cast<float>(video_rect.y) + static_cast<float>(item.style.position_y) * scale_y;
            switch (subtitleVerticalGroup(alignment)) {
            case 2:
                draw_y = anchor_y;
                break;
            case 1:
                draw_y = anchor_y - text_height * 0.5f;
                break;
            default:
                draw_y = anchor_y - text_height;
                break;
            }
        } else {
            switch (subtitleVerticalGroup(alignment)) {
            case 2:
                draw_y = next_top_y;
                next_top_y += text_height + gap;
                break;
            case 1:
                draw_y = next_middle_y - text_height * 0.5f;
                next_middle_y += text_height + gap;
                break;
            default:
                draw_y = next_bottom_y - text_height;
                next_bottom_y = draw_y - gap;
                break;
            }
        }
        draw_y = std::clamp(draw_y, top_limit, bottom_limit - text_height);

        const float actual_left = draw_x + metrics.left;
        const float actual_top = draw_y + metrics.top;
        const bool draw_box = item.style.border_style == 3;
        const float outline_radius = draw_box ? 0.0f : std::max(0.0f, static_cast<float>(item.style.outline) * average_scale);
        const float shadow_offset = std::max(0.0f, static_cast<float>(item.style.shadow) * average_scale);

        const D2D1_COLOR_F fill_color = toD2DColor(item.style.primary_color);
        const D2D1_COLOR_F outline_color = toD2DColor(item.style.outline_color);
        const D2D1_COLOR_F shadow_color = item.style.background_color.a > 0
            ? toD2DColor(item.style.background_color)
            : D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.72f);
        const D2D1_COLOR_F box_color = item.style.background_color.a > 0
            ? toD2DColor(item.style.background_color)
            : D2D1::ColorF(0.02f, 0.02f, 0.02f, 0.58f);

        if (draw_box) {
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
            d2d_context_->FillRoundedRectangle(&background, subtitle_background_brush_.Get());
        }


        if (outline_radius > 0.0f) {
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
            subtitle_shadow_brush_->SetColor(outline_color);
            for (const auto& offset : kOutlineOffsets) {
                d2d_context_->DrawTextLayout(D2D1::Point2F(draw_x + offset[0] * outline_radius,
                                                           draw_y + offset[1] * outline_radius),
                                             text_layout.Get(),
                                             subtitle_shadow_brush_.Get());
            }
        }

        if (shadow_offset > 0.0f) {
            subtitle_shadow_brush_->SetColor(shadow_color);
            d2d_context_->DrawTextLayout(D2D1::Point2F(draw_x + shadow_offset, draw_y + shadow_offset),
                                         text_layout.Get(),
                                         subtitle_shadow_brush_.Get());
        }

        subtitle_fill_brush_->SetColor(fill_color);
        d2d_context_->DrawTextLayout(D2D1::Point2F(draw_x, draw_y),
                                     text_layout.Get(),
                                     subtitle_fill_brush_.Get());
    }

    const HRESULT hr = d2d_context_->EndDraw();
    if (hr == D2DERR_RECREATE_TARGET) {
        recreate_target = true;
    } else if (FAILED(hr)) {
        LOG_WARNING("D2D subtitle draw failed: " << static_cast<long>(hr));
    }
    if (recreate_target) {
        createTextTarget();
    }
}
bool D3D11VideoRenderer::Impl::renderLocked() {
    if (!initialized_ || !swap_chain_ || !render_target_view_ || minimized_.load()) {
        return false;
    }
    D3D11_VIEWPORT viewport{};
    viewport.Width = static_cast<float>(std::max(1, width_.load()));
    viewport.Height = static_cast<float>(std::max(1, height_.load()));
    viewport.MaxDepth = 1.0f;
    context_->RSSetViewports(1, &viewport);
    context_->OMSetRenderTargets(1, render_target_view_.GetAddressOf(), nullptr);
    const float clear_color[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    context_->ClearRenderTargetView(render_target_view_.Get(), clear_color);

    if (frame_available_ && current_frame_) {
        const SDL_Rect dst = computeRenderRect(width_.load(), height_.load(), current_frame_->width, current_frame_->height);
        const float left = (static_cast<float>(dst.x) / static_cast<float>(std::max(1, width_.load()))) * 2.0f - 1.0f;
        const float right = (static_cast<float>(dst.x + dst.w) / static_cast<float>(std::max(1, width_.load()))) * 2.0f - 1.0f;
        const float top = 1.0f - (static_cast<float>(dst.y) / static_cast<float>(std::max(1, height_.load()))) * 2.0f;
        const float bottom = 1.0f - (static_cast<float>(dst.y + dst.h) / static_cast<float>(std::max(1, height_.load()))) * 2.0f;
        const VideoVertex vertices[] = {{left, top, 0.0f, 0.0f}, {right, top, 1.0f, 0.0f}, {left, bottom, 0.0f, 1.0f},
                                        {left, bottom, 0.0f, 1.0f}, {right, top, 1.0f, 0.0f}, {right, bottom, 1.0f, 1.0f}};
        D3D11_MAPPED_SUBRESOURCE mapped{};
        if (SUCCEEDED(context_->Map(video_vertex_buffer_.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
            std::memcpy(mapped.pData, vertices, sizeof(vertices));
            context_->Unmap(video_vertex_buffer_.Get(), 0);
            const UINT stride = sizeof(VideoVertex);
            const UINT offset = 0;
            context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            context_->IASetInputLayout(video_input_layout_.Get());
            context_->IASetVertexBuffers(0, 1, video_vertex_buffer_.GetAddressOf(), &stride, &offset);
            context_->VSSetShader(video_vertex_shader_.Get(), nullptr, 0);
            context_->PSSetSamplers(0, 1, sampler_state_.GetAddressOf());
            const bool bound = current_frame_->format == AV_PIX_FMT_D3D11 ? bindNativeFrameLocked(current_frame_) : bindSoftwareFrameLocked(current_frame_);
            if (bound) {
                context_->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFFu);
                context_->Draw(6, 0);
                ID3D11ShaderResourceView* null_srvs[] = {nullptr, nullptr, nullptr};
                context_->PSSetShaderResources(0, 3, null_srvs);
            }
        }
    }
    std::vector<ColorVertex> overlay_vertices;
    const ControlLayout layout = computeControlLayout(width_.load(), height_.load());
    double progress = 0.0;
    const double duration = overlay_duration_.load();
    if (duration > 0.0) {
        progress = overlay_position_.load() / duration;
    }
    float volume = overlay_volume_.load();
    const bool paused = overlay_paused_.load();
    {
        std::lock_guard<std::mutex> request_lock(request_mutex_);
        if (seek_preview_active_) {
            progress = seek_preview_ratio_;
        }
        if (dragging_volume_) {
            volume = requested_volume_;
        }
    }
    progress = clampRatio(progress);
    volume = clampVolume(volume);
    appendRect(overlay_vertices, layout.panel.x, layout.panel.y, layout.panel.w, layout.panel.h, {0.0f, 0.0f, 0.0f, 0.55f});
    appendRect(overlay_vertices, layout.progress_track.x, layout.progress_track.y, layout.progress_track.w, layout.progress_track.h,
               {0.33f, 0.33f, 0.33f, 0.9f});
    appendRect(overlay_vertices, layout.progress_track.x, layout.progress_track.y,
               std::max(1, static_cast<int>(std::lround(progress * static_cast<double>(layout.progress_track.w)))), layout.progress_track.h,
               {0.96f, 0.96f, 0.96f, 0.92f});
    appendRect(overlay_vertices, layout.volume_track.x, layout.volume_track.y, layout.volume_track.w, layout.volume_track.h,
               {0.33f, 0.33f, 0.33f, 0.9f});
    appendRect(overlay_vertices, layout.volume_track.x, layout.volume_track.y,
               std::max(1, static_cast<int>(std::lround(volume * static_cast<double>(layout.volume_track.w)))), layout.volume_track.h,
               {0.51f, 0.86f, 0.62f, 0.92f});
    if (paused) {
        appendRect(overlay_vertices, layout.panel.x + layout.panel.w / 2 - 6, layout.panel.y + 8, 12, 6,
                   {1.0f, 0.8f, 0.4f, 0.92f});
    }
    if (!overlay_vertices.empty() && ensureOverlayBuffer(overlay_vertices.size())) {
        D3D11_MAPPED_SUBRESOURCE mapped{};
        if (SUCCEEDED(context_->Map(overlay_vertex_buffer_.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
            std::memcpy(mapped.pData, overlay_vertices.data(), overlay_vertices.size() * sizeof(ColorVertex));
            context_->Unmap(overlay_vertex_buffer_.Get(), 0);
            const UINT stride = sizeof(ColorVertex);
            const UINT offset = 0;
            context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            context_->IASetInputLayout(color_input_layout_.Get());
            context_->IASetVertexBuffers(0, 1, overlay_vertex_buffer_.GetAddressOf(), &stride, &offset);
            context_->VSSetShader(color_vertex_shader_.Get(), nullptr, 0);
            context_->PSSetShader(color_pixel_shader_.Get(), nullptr, 0);
            context_->OMSetBlendState(alpha_blend_state_.Get(), nullptr, 0xFFFFFFFFu);
            context_->Draw(static_cast<UINT>(overlay_vertices.size()), 0);
            context_->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFFu);
        }
    }

    context_->OMSetRenderTargets(0, nullptr, nullptr);
    context_->Flush();
    drawSubtitleLocked(layout);

    const HRESULT hr = swap_chain_->Present(1, 0);
    return SUCCEEDED(hr);
}


void D3D11VideoRenderer::Impl::redrawIfPaused() {
    if (!overlay_paused_.load()) {
        return;
    }
    std::lock_guard<std::mutex> render_lock(render_mutex_);
    renderLocked();
}

void D3D11VideoRenderer::Impl::resetRequests() {
    should_quit_.store(false);
    fullscreen_.store(false);
    minimized_.store(false);
    toggle_pause_requested_ = false;
    seek_requested_ = false;
    seek_ratio_ = 0.0;
    seek_delta_requested_ = false;
    seek_delta_seconds_ = 0.0;
    volume_change_requested_ = false;
    requested_volume_ = 1.0f;
    speed_change_requested_ = false;
    speed_delta_ = 0.0;
    speed_reset_requested_ = false;
    subtitle_toggle_requested_ = false;
    ab_repeat_start_requested_ = false;
    ab_repeat_end_requested_ = false;
    ab_repeat_clear_requested_ = false;
    screenshot_requested_ = false;
    step_frame_backward_requested_ = false;
    step_frame_forward_requested_ = false;
    subtitle_delay_change_requested_ = false;
    subtitle_delay_delta_seconds_ = 0.0;
    audio_delay_change_requested_ = false;
    audio_delay_delta_seconds_ = 0.0;
    next_chapter_requested_ = false;
    previous_chapter_requested_ = false;
    next_item_requested_ = false;
    previous_item_requested_ = false;
    last_nonzero_volume_ = 1.0f;
    dragging_seek_ = false;
    dragging_volume_ = false;
    seek_preview_active_ = false;
    seek_preview_ratio_ = 0.0;
}

void D3D11VideoRenderer::Impl::toggleFullscreen() {
    if (!window_) {
        return;
    }
    const bool next = !fullscreen_.load();
    if (SDL_SetWindowFullscreen(window_, next ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0) != 0) {
        LOG_WARNING("SDL_SetWindowFullscreen failed: " << SDL_GetError());
        return;
    }
    fullscreen_.store(next);
    updateWindowSizeFromSdl(window_, width_, height_);
    std::lock_guard<std::mutex> render_lock(render_mutex_);
    resizeSwapChainLocked(width_.load(), height_.load());
    renderLocked();
}

void D3D11VideoRenderer::Impl::renderFrame(const core::VideoFrame& frame) {
    if (!frame.valid || !frame.frame) {
        return;
    }
    std::lock_guard<std::mutex> render_lock(render_mutex_);
    if (!current_frame_) {
        current_frame_ = av_frame_alloc();
        if (!current_frame_) {
            return;
        }
    }
    av_frame_unref(current_frame_);
    if (av_frame_ref(current_frame_, frame.frame) < 0) {
        return;
    }
    frame_available_ = true;
}

void D3D11VideoRenderer::Impl::present() {
    std::lock_guard<std::mutex> render_lock(render_mutex_);
    renderLocked();
}

void D3D11VideoRenderer::Impl::clear() {
    std::lock_guard<std::mutex> render_lock(render_mutex_);
    frame_available_ = false;
    if (current_frame_) {
        av_frame_unref(current_frame_);
    }
    renderLocked();
}

void D3D11VideoRenderer::Impl::handleEvents() {
    if (!window_) {
        return;
    }
    bool needs_redraw = false;
    SDL_Event event{};
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_QUIT:
            should_quit_.store(true);
            break;
        case SDL_KEYDOWN: {
            const SDL_Keycode key_code = event.key.keysym.sym;
            if (key_code == SDLK_ESCAPE) {
                if (fullscreen_.load()) {
                    toggleFullscreen();
                } else {
                    should_quit_.store(true);
                }
                break;
            }
            if (key_code == SDLK_RETURN || key_code == SDLK_KP_ENTER) {
                toggleFullscreen();
                break;
            }
            std::optional<input::PlayerAction> action = hotkey_manager_.actionForKey(key_code);
            if (!action && key_code == SDLK_EQUALS) action = input::PlayerAction::VolumeUp;
            if (!action && key_code == SDLK_MINUS) action = input::PlayerAction::VolumeDown;
            if (!action) break;
            std::lock_guard<std::mutex> request_lock(request_mutex_);
            switch (*action) {
            case input::PlayerAction::PlayPause: toggle_pause_requested_ = true; break;
            case input::PlayerAction::SeekBackward:
            case input::PlayerAction::SeekForward: seek_delta_seconds_ += ((*action == input::PlayerAction::SeekForward) ? 1.0 : -1.0) * (((event.key.keysym.mod & KMOD_CTRL) != 0) ? kSeekStepSecondsCtrl : kSeekStepSeconds); seek_delta_requested_ = true; break;
            case input::PlayerAction::VolumeUp:
            case input::PlayerAction::VolumeDown: requested_volume_ = clampVolume(overlay_volume_.load() + ((*action == input::PlayerAction::VolumeUp) ? kVolumeStep : -kVolumeStep)); volume_change_requested_ = true; overlay_volume_.store(requested_volume_); needs_redraw = true; break;
            case input::PlayerAction::ToggleMute: requested_volume_ = overlay_volume_.load() > 0.0001f ? 0.0f : std::max(0.5f, last_nonzero_volume_); volume_change_requested_ = true; overlay_volume_.store(requested_volume_); needs_redraw = true; break;
            case input::PlayerAction::SpeedDown:
            case input::PlayerAction::SpeedUp: speed_delta_ += (*action == input::PlayerAction::SpeedUp) ? kSpeedStep : -kSpeedStep; speed_change_requested_ = true; break;
            case input::PlayerAction::SpeedReset: speed_reset_requested_ = true; break;
            case input::PlayerAction::ToggleSubtitle: subtitle_toggle_requested_ = true; break;
            case input::PlayerAction::SetABRepeatStart: ab_repeat_start_requested_ = true; break;
            case input::PlayerAction::SetABRepeatEnd: ab_repeat_end_requested_ = true; break;
            case input::PlayerAction::ClearABRepeat: ab_repeat_clear_requested_ = true; break;
            case input::PlayerAction::TakeScreenshot: screenshot_requested_ = true; break;
            case input::PlayerAction::StepFrameBackward: step_frame_backward_requested_ = true; break;
            case input::PlayerAction::StepFrameForward: step_frame_forward_requested_ = true; break;
            case input::PlayerAction::SubtitleDelayDown:
            case input::PlayerAction::SubtitleDelayUp: if ((event.key.keysym.mod & KMOD_CTRL) != 0) { audio_delay_delta_seconds_ += (*action == input::PlayerAction::SubtitleDelayUp) ? 0.1 : -0.1; audio_delay_change_requested_ = true; } else { subtitle_delay_delta_seconds_ += (*action == input::PlayerAction::SubtitleDelayUp) ? 0.1 : -0.1; subtitle_delay_change_requested_ = true; } break;
            case input::PlayerAction::PreviousChapter: previous_chapter_requested_ = true; break;
            case input::PlayerAction::NextChapter: next_chapter_requested_ = true; break;
            case input::PlayerAction::PreviousItem: previous_item_requested_ = true; break;
            case input::PlayerAction::NextItem: next_item_requested_ = true; break;
            case input::PlayerAction::ToggleFullscreen: toggleFullscreen(); break;
            case input::PlayerAction::Quit: should_quit_.store(true); break;
            case input::PlayerAction::SeekTo10Percent: case input::PlayerAction::SeekTo20Percent: case input::PlayerAction::SeekTo30Percent: case input::PlayerAction::SeekTo40Percent: case input::PlayerAction::SeekTo50Percent: case input::PlayerAction::SeekTo60Percent: case input::PlayerAction::SeekTo70Percent: case input::PlayerAction::SeekTo80Percent: case input::PlayerAction::SeekTo90Percent: {
                const int ordinal = static_cast<int>(*action) - static_cast<int>(input::PlayerAction::SeekTo10Percent) + 1;
                seek_ratio_ = ordinal / 10.0; seek_requested_ = true; seek_preview_active_ = false; break; }
            }
            break;
        }
        case SDL_WINDOWEVENT:
            if (event.window.event == SDL_WINDOWEVENT_MINIMIZED || event.window.event == SDL_WINDOWEVENT_HIDDEN) {
                minimized_.store(true);
            } else if (event.window.event == SDL_WINDOWEVENT_RESTORED || event.window.event == SDL_WINDOWEVENT_SHOWN || event.window.event == SDL_WINDOWEVENT_MAXIMIZED || event.window.event == SDL_WINDOWEVENT_RESIZED || event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                minimized_.store(false);
                updateWindowSizeFromSdl(window_, width_, height_);
                std::lock_guard<std::mutex> render_lock(render_mutex_);
                resizeSwapChainLocked(width_.load(), height_.load());
                needs_redraw = true;
            }
            break;
        case SDL_MOUSEBUTTONDOWN:
            if (event.button.button == SDL_BUTTON_LEFT) {
                const ControlLayout layout = computeControlLayout(width_.load(), height_.load());
                if (pointInRect(event.button.x, event.button.y, layout.progress_track)) {
                    dragging_seek_ = true; seek_preview_active_ = true; seek_preview_ratio_ = clampRatio(static_cast<double>(event.button.x - layout.progress_track.x) / static_cast<double>(layout.progress_track.w)); needs_redraw = true;
                } else if (pointInRect(event.button.x, event.button.y, layout.volume_track)) {
                    dragging_volume_ = true; requested_volume_ = static_cast<float>(clampRatio(static_cast<double>(event.button.x - layout.volume_track.x) / static_cast<double>(layout.volume_track.w))); volume_change_requested_ = true; overlay_volume_.store(requested_volume_); needs_redraw = true;
                }
            }
            break;
        case SDL_MOUSEMOTION:
            if (dragging_seek_) { const ControlLayout layout = computeControlLayout(width_.load(), height_.load()); seek_preview_ratio_ = clampRatio(static_cast<double>(event.motion.x - layout.progress_track.x) / static_cast<double>(layout.progress_track.w)); needs_redraw = true; }
            if (dragging_volume_) { const ControlLayout layout = computeControlLayout(width_.load(), height_.load()); requested_volume_ = static_cast<float>(clampRatio(static_cast<double>(event.motion.x - layout.volume_track.x) / static_cast<double>(layout.volume_track.w))); volume_change_requested_ = true; overlay_volume_.store(requested_volume_); needs_redraw = true; }
            break;
        case SDL_MOUSEBUTTONUP:
            if (event.button.button == SDL_BUTTON_LEFT) {
                if (dragging_seek_) { seek_ratio_ = seek_preview_ratio_; seek_requested_ = true; seek_preview_active_ = false; dragging_seek_ = false; needs_redraw = true; }
                if (dragging_volume_) { dragging_volume_ = false; needs_redraw = true; }
            }
            break;
        default:
            break;
        }
    }
    if (needs_redraw) {
        redrawIfPaused();
    }
}

#define VP_CONSUME_FLAG(flag) std::lock_guard<std::mutex> request_lock(request_mutex_); if (!(flag)) return false; (flag) = false; return true;

bool D3D11VideoRenderer::Impl::consumeTogglePauseRequest() { VP_CONSUME_FLAG(toggle_pause_requested_); }
bool D3D11VideoRenderer::Impl::consumeSeekRequest(double& normalized_position) { std::lock_guard<std::mutex> request_lock(request_mutex_); if (!seek_requested_) return false; normalized_position = seek_ratio_; seek_requested_ = false; return true; }
bool D3D11VideoRenderer::Impl::consumeSeekDeltaRequest(double& delta_seconds) { std::lock_guard<std::mutex> request_lock(request_mutex_); if (!seek_delta_requested_) return false; delta_seconds = seek_delta_seconds_; seek_delta_seconds_ = 0.0; seek_delta_requested_ = false; return true; }
bool D3D11VideoRenderer::Impl::consumeVolumeChangeRequest(float& volume) { std::lock_guard<std::mutex> request_lock(request_mutex_); if (!volume_change_requested_) return false; volume = requested_volume_; volume_change_requested_ = false; if (volume > 0.0001f) last_nonzero_volume_ = volume; return true; }
bool D3D11VideoRenderer::Impl::consumeSpeedChangeRequest(double& speed_delta) { std::lock_guard<std::mutex> request_lock(request_mutex_); if (!speed_change_requested_) return false; speed_delta = speed_delta_; speed_delta_ = 0.0; speed_change_requested_ = false; return true; }
bool D3D11VideoRenderer::Impl::consumeResetSpeedRequest() { VP_CONSUME_FLAG(speed_reset_requested_); }
bool D3D11VideoRenderer::Impl::consumeToggleSubtitleRequest() { VP_CONSUME_FLAG(subtitle_toggle_requested_); }
bool D3D11VideoRenderer::Impl::consumeSetABRepeatStartRequest() { VP_CONSUME_FLAG(ab_repeat_start_requested_); }
bool D3D11VideoRenderer::Impl::consumeSetABRepeatEndRequest() { VP_CONSUME_FLAG(ab_repeat_end_requested_); }
bool D3D11VideoRenderer::Impl::consumeClearABRepeatRequest() { VP_CONSUME_FLAG(ab_repeat_clear_requested_); }
bool D3D11VideoRenderer::Impl::consumeScreenshotRequest() { VP_CONSUME_FLAG(screenshot_requested_); }
bool D3D11VideoRenderer::Impl::consumeStepFrameBackwardRequest() { VP_CONSUME_FLAG(step_frame_backward_requested_); }
bool D3D11VideoRenderer::Impl::consumeStepFrameForwardRequest() { VP_CONSUME_FLAG(step_frame_forward_requested_); }
bool D3D11VideoRenderer::Impl::consumeSubtitleDelayChangeRequest(double& delta_seconds) { std::lock_guard<std::mutex> request_lock(request_mutex_); if (!subtitle_delay_change_requested_) return false; delta_seconds = subtitle_delay_delta_seconds_; subtitle_delay_delta_seconds_ = 0.0; subtitle_delay_change_requested_ = false; return true; }
bool D3D11VideoRenderer::Impl::consumeAudioDelayChangeRequest(double& delta_seconds) { std::lock_guard<std::mutex> request_lock(request_mutex_); if (!audio_delay_change_requested_) return false; delta_seconds = audio_delay_delta_seconds_; audio_delay_delta_seconds_ = 0.0; audio_delay_change_requested_ = false; return true; }
bool D3D11VideoRenderer::Impl::consumeNextChapterRequest() { VP_CONSUME_FLAG(next_chapter_requested_); }
bool D3D11VideoRenderer::Impl::consumePreviousChapterRequest() { VP_CONSUME_FLAG(previous_chapter_requested_); }
bool D3D11VideoRenderer::Impl::consumeNextItemRequest() { VP_CONSUME_FLAG(next_item_requested_); }
bool D3D11VideoRenderer::Impl::consumePreviousItemRequest() { VP_CONSUME_FLAG(previous_item_requested_); }

#undef VP_CONSUME_FLAG

void D3D11VideoRenderer::Impl::setOverlayState(double position, double duration, float volume, bool paused) { overlay_position_.store(std::max(0.0, position)); overlay_duration_.store(std::max(0.0, duration)); overlay_volume_.store(clampVolume(volume)); overlay_paused_.store(paused); if (paused) redrawIfPaused(); }
void D3D11VideoRenderer::Impl::setSubtitleText(const std::string& text) {
    if (text.empty()) {
        setSubtitleItems({});
        return;
    }

    subtitle::SubtitleItem item{};
    item.text = text;
    item.raw_text = text;
    item.style = makePlainSubtitleStyle();
    const size_t visible_length = countUtf8CodePoints(text);
    if (visible_length > 0) {
        item.runs.push_back(subtitle::SubtitleTextRun{0, visible_length, item.style});
    }
    setSubtitleItems({item});
}
void D3D11VideoRenderer::Impl::setSubtitleItems(const std::vector<subtitle::SubtitleItem>& items) {
    {
        std::lock_guard<std::mutex> subtitle_lock(subtitle_mutex_);
        subtitle_items_ = items;
    }
    if (overlay_paused_.load()) {
        redrawIfPaused();
    }
}
void D3D11VideoRenderer::Impl::setHotkeyManager(const input::HotkeyManager& hotkey_manager) { std::lock_guard<std::mutex> request_lock(request_mutex_); hotkey_manager_ = hotkey_manager; }
bool D3D11VideoRenderer::Impl::supportsNativeFrameFormat(AVPixelFormat format) const { return format == AV_PIX_FMT_D3D11 && device3_ != nullptr; }
void* D3D11VideoRenderer::Impl::nativeDeviceHandle() const { return device_.Get(); }

D3D11VideoRenderer::D3D11VideoRenderer() : impl_(std::make_unique<Impl>()) {}
D3D11VideoRenderer::~D3D11VideoRenderer() = default;
bool D3D11VideoRenderer::init(const VideoRendererConfig& config) { return impl_->init(config); }
void D3D11VideoRenderer::close() { impl_->close(); }
void D3D11VideoRenderer::renderFrame(const core::VideoFrame& frame) { impl_->renderFrame(frame); }
void D3D11VideoRenderer::present() { impl_->present(); }
void D3D11VideoRenderer::clear() { impl_->clear(); }
void D3D11VideoRenderer::handleEvents() { impl_->handleEvents(); }
bool D3D11VideoRenderer::shouldQuit() const { return impl_->shouldQuit(); }
bool D3D11VideoRenderer::consumeTogglePauseRequest() { return impl_->consumeTogglePauseRequest(); }
bool D3D11VideoRenderer::consumeSeekRequest(double& normalized_position) { return impl_->consumeSeekRequest(normalized_position); }
bool D3D11VideoRenderer::consumeSeekDeltaRequest(double& delta_seconds) { return impl_->consumeSeekDeltaRequest(delta_seconds); }
bool D3D11VideoRenderer::consumeVolumeChangeRequest(float& volume) { return impl_->consumeVolumeChangeRequest(volume); }
bool D3D11VideoRenderer::consumeSpeedChangeRequest(double& speed_delta) { return impl_->consumeSpeedChangeRequest(speed_delta); }
bool D3D11VideoRenderer::consumeResetSpeedRequest() { return impl_->consumeResetSpeedRequest(); }
bool D3D11VideoRenderer::consumeToggleSubtitleRequest() { return impl_->consumeToggleSubtitleRequest(); }
bool D3D11VideoRenderer::consumeSetABRepeatStartRequest() { return impl_->consumeSetABRepeatStartRequest(); }
bool D3D11VideoRenderer::consumeSetABRepeatEndRequest() { return impl_->consumeSetABRepeatEndRequest(); }
bool D3D11VideoRenderer::consumeClearABRepeatRequest() { return impl_->consumeClearABRepeatRequest(); }
bool D3D11VideoRenderer::consumeScreenshotRequest() { return impl_->consumeScreenshotRequest(); }
bool D3D11VideoRenderer::consumeStepFrameBackwardRequest() { return impl_->consumeStepFrameBackwardRequest(); }
bool D3D11VideoRenderer::consumeStepFrameForwardRequest() { return impl_->consumeStepFrameForwardRequest(); }
bool D3D11VideoRenderer::consumeSubtitleDelayChangeRequest(double& delta_seconds) { return impl_->consumeSubtitleDelayChangeRequest(delta_seconds); }
bool D3D11VideoRenderer::consumeAudioDelayChangeRequest(double& delta_seconds) { return impl_->consumeAudioDelayChangeRequest(delta_seconds); }
bool D3D11VideoRenderer::consumeNextChapterRequest() { return impl_->consumeNextChapterRequest(); }
bool D3D11VideoRenderer::consumePreviousChapterRequest() { return impl_->consumePreviousChapterRequest(); }
bool D3D11VideoRenderer::consumeNextItemRequest() { return impl_->consumeNextItemRequest(); }
bool D3D11VideoRenderer::consumePreviousItemRequest() { return impl_->consumePreviousItemRequest(); }
void D3D11VideoRenderer::setOverlayState(double position, double duration, float volume, bool paused) { impl_->setOverlayState(position, duration, volume, paused); }
void D3D11VideoRenderer::setSubtitleText(const std::string& text) { impl_->setSubtitleText(text); }
void D3D11VideoRenderer::setSubtitleItems(const std::vector<subtitle::SubtitleItem>& items) { impl_->setSubtitleItems(items); }
void D3D11VideoRenderer::setHotkeyManager(const input::HotkeyManager& hotkey_manager) { impl_->setHotkeyManager(hotkey_manager); }
bool D3D11VideoRenderer::supportsNativeFrameFormat(AVPixelFormat format) const { return impl_->supportsNativeFrameFormat(format); }
void* D3D11VideoRenderer::nativeDeviceHandle() const { return impl_->nativeDeviceHandle(); }
const char* D3D11VideoRenderer::rendererBackendName() const { return "D3D11"; }

}  // namespace vp::render







