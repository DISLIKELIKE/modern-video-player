#include "display.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cstddef>
#include <cmath>
#include <cstring>
#include <iostream>
#include <sstream>
#include <vector>

extern "C" {
#include <libavutil/frame.h>
}

namespace vp {

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

std::string toLowerAscii(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return text;
}

using Glyph5x7 = std::array<uint8_t, 7>;

const Glyph5x7& glyphFor(char ch) {
    static const Glyph5x7 kSpace{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    static const Glyph5x7 kUnknown{0x1F, 0x11, 0x15, 0x15, 0x15, 0x11, 0x1F};
    static const Glyph5x7 kDot{0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C};
    static const Glyph5x7 kComma{0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x08};
    static const Glyph5x7 kColon{0x00, 0x04, 0x00, 0x00, 0x04, 0x00, 0x00};
    static const Glyph5x7 kSemicolon{0x00, 0x04, 0x00, 0x00, 0x04, 0x04, 0x08};
    static const Glyph5x7 kExclamation{0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x04};
    static const Glyph5x7 kQuestion{0x0E, 0x11, 0x01, 0x02, 0x04, 0x00, 0x04};
    static const Glyph5x7 kMinus{0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00};
    static const Glyph5x7 kUnderscore{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F};
    static const Glyph5x7 kSlash{0x01, 0x02, 0x04, 0x08, 0x10, 0x00, 0x00};
    static const Glyph5x7 kPercent{0x19, 0x19, 0x02, 0x04, 0x08, 0x13, 0x13};
    static const Glyph5x7 kPlus{0x00, 0x04, 0x04, 0x1F, 0x04, 0x04, 0x00};
    static const Glyph5x7 kEqual{0x00, 0x1F, 0x00, 0x1F, 0x00, 0x00, 0x00};
    static const Glyph5x7 kApostrophe{0x04, 0x04, 0x02, 0x00, 0x00, 0x00, 0x00};
    static const Glyph5x7 kQuote{0x0A, 0x0A, 0x05, 0x00, 0x00, 0x00, 0x00};
    static const Glyph5x7 kLeftParen{0x02, 0x04, 0x08, 0x08, 0x08, 0x04, 0x02};
    static const Glyph5x7 kRightParen{0x08, 0x04, 0x02, 0x02, 0x02, 0x04, 0x08};

    switch (std::toupper(static_cast<unsigned char>(ch))) {
    case ' ':
        return kSpace;
    case '.':
        return kDot;
    case ',':
        return kComma;
    case ':':
        return kColon;
    case ';':
        return kSemicolon;
    case '!':
        return kExclamation;
    case '?':
        return kQuestion;
    case '-':
        return kMinus;
    case '_':
        return kUnderscore;
    case '/':
        return kSlash;
    case '%':
        return kPercent;
    case '+':
        return kPlus;
    case '=':
        return kEqual;
    case '\'':
        return kApostrophe;
    case '"':
        return kQuote;
    case '(':
        return kLeftParen;
    case ')':
        return kRightParen;
    case '0': {
        static const Glyph5x7 v{0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E};
        return v;
    }
    case '1': {
        static const Glyph5x7 v{0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E};
        return v;
    }
    case '2': {
        static const Glyph5x7 v{0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F};
        return v;
    }
    case '3': {
        static const Glyph5x7 v{0x1E, 0x01, 0x01, 0x0E, 0x01, 0x01, 0x1E};
        return v;
    }
    case '4': {
        static const Glyph5x7 v{0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02};
        return v;
    }
    case '5': {
        static const Glyph5x7 v{0x1F, 0x10, 0x10, 0x1E, 0x01, 0x01, 0x1E};
        return v;
    }
    case '6': {
        static const Glyph5x7 v{0x0E, 0x10, 0x10, 0x1E, 0x11, 0x11, 0x0E};
        return v;
    }
    case '7': {
        static const Glyph5x7 v{0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08};
        return v;
    }
    case '8': {
        static const Glyph5x7 v{0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E};
        return v;
    }
    case '9': {
        static const Glyph5x7 v{0x0E, 0x11, 0x11, 0x0F, 0x01, 0x01, 0x0E};
        return v;
    }
    case 'A': {
        static const Glyph5x7 v{0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11};
        return v;
    }
    case 'B': {
        static const Glyph5x7 v{0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E};
        return v;
    }
    case 'C': {
        static const Glyph5x7 v{0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E};
        return v;
    }
    case 'D': {
        static const Glyph5x7 v{0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E};
        return v;
    }
    case 'E': {
        static const Glyph5x7 v{0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F};
        return v;
    }
    case 'F': {
        static const Glyph5x7 v{0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10};
        return v;
    }
    case 'G': {
        static const Glyph5x7 v{0x0E, 0x11, 0x10, 0x10, 0x13, 0x11, 0x0E};
        return v;
    }
    case 'H': {
        static const Glyph5x7 v{0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11};
        return v;
    }
    case 'I': {
        static const Glyph5x7 v{0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E};
        return v;
    }
    case 'J': {
        static const Glyph5x7 v{0x01, 0x01, 0x01, 0x01, 0x11, 0x11, 0x0E};
        return v;
    }
    case 'K': {
        static const Glyph5x7 v{0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11};
        return v;
    }
    case 'L': {
        static const Glyph5x7 v{0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F};
        return v;
    }
    case 'M': {
        static const Glyph5x7 v{0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11};
        return v;
    }
    case 'N': {
        static const Glyph5x7 v{0x11, 0x11, 0x19, 0x15, 0x13, 0x11, 0x11};
        return v;
    }
    case 'O': {
        static const Glyph5x7 v{0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E};
        return v;
    }
    case 'P': {
        static const Glyph5x7 v{0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10};
        return v;
    }
    case 'Q': {
        static const Glyph5x7 v{0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D};
        return v;
    }
    case 'R': {
        static const Glyph5x7 v{0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11};
        return v;
    }
    case 'S': {
        static const Glyph5x7 v{0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E};
        return v;
    }
    case 'T': {
        static const Glyph5x7 v{0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04};
        return v;
    }
    case 'U': {
        static const Glyph5x7 v{0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E};
        return v;
    }
    case 'V': {
        static const Glyph5x7 v{0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04};
        return v;
    }
    case 'W': {
        static const Glyph5x7 v{0x11, 0x11, 0x11, 0x15, 0x15, 0x15, 0x0A};
        return v;
    }
    case 'X': {
        static const Glyph5x7 v{0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11};
        return v;
    }
    case 'Y': {
        static const Glyph5x7 v{0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04};
        return v;
    }
    case 'Z': {
        static const Glyph5x7 v{0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F};
        return v;
    }
    default:
        return kUnknown;
    }
}

void drawGlyph5x7(SDL_Renderer* renderer, char ch, int x, int y, int scale, SDL_Color color) {
    if (!renderer || scale <= 0) {
        return;
    }

    const Glyph5x7& glyph = glyphFor(ch);
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    for (int row = 0; row < 7; ++row) {
        for (int col = 0; col < 5; ++col) {
            if ((glyph[row] & (1 << (4 - col))) == 0) {
                continue;
            }
            SDL_Rect pixel_rect{
                x + col * scale,
                y + row * scale,
                scale,
                scale
            };
            SDL_RenderFillRect(renderer, &pixel_rect);
        }
    }
}

std::vector<std::string> splitSubtitleLines(const std::string& text, size_t max_lines) {
    std::vector<std::string> lines;
    std::stringstream stream(text);
    std::string line;

    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        std::string normalized;
        normalized.reserve(line.size());
        for (size_t i = 0; i < line.size();) {
            const unsigned char ch = static_cast<unsigned char>(line[i]);
            if (ch < 0x80) {
                normalized.push_back(static_cast<char>(ch));
                ++i;
                continue;
            }

            normalized.push_back('?');
            if ((ch & 0xE0) == 0xC0) {
                i += 2;
            } else if ((ch & 0xF0) == 0xE0) {
                i += 3;
            } else if ((ch & 0xF8) == 0xF0) {
                i += 4;
            } else {
                ++i;
            }
        }

        lines.push_back(std::move(normalized));
        if (lines.size() >= max_lines) {
            break;
        }
    }

    return lines;
}

Uint32 sdlTextureFormatForFrame(AVPixelFormat format) {
    switch (format) {
    case AV_PIX_FMT_YUV420P:
        return SDL_PIXELFORMAT_IYUV;
    case AV_PIX_FMT_NV12:
        return SDL_PIXELFORMAT_NV12;
    default:
        return SDL_PIXELFORMAT_UNKNOWN;
    }
}

bool frameHasRequiredPlanes(const AVFrame& frame, AVPixelFormat format) {
    switch (format) {
    case AV_PIX_FMT_YUV420P:
        return frame.data[0] && frame.data[1] && frame.data[2];
    case AV_PIX_FMT_NV12:
        return frame.data[0] && frame.data[1];
    default:
        return false;
    }
}

bool frameHasPositiveStrides(const AVFrame& frame, AVPixelFormat format) {
    switch (format) {
    case AV_PIX_FMT_YUV420P:
        return frame.linesize[0] > 0 && frame.linesize[1] > 0 && frame.linesize[2] > 0;
    case AV_PIX_FMT_NV12:
        return frame.linesize[0] > 0 && frame.linesize[1] > 0;
    default:
        return false;
    }
}

bool canReferenceFrameDirectly(const AVFrame& frame, AVPixelFormat format) {
    return frame.width > 0 && frame.height > 0 &&
           sdlTextureFormatForFrame(format) != SDL_PIXELFORMAT_UNKNOWN &&
           frameHasRequiredPlanes(frame, format) &&
           frameHasPositiveStrides(frame, format);
}

} // namespace
Display::Display()
    : window_(nullptr)
    , renderer_(nullptr)
    , texture_(nullptr)
    , width_(0)
    , height_(0)
    , should_quit_(false)
    , toggle_pause_requested_(false)
    , fullscreen_(false)
    , minimized_(false)
    , renderer_reset_requested_(false)
    , texture_reset_requested_(false)
    , fullscreen_toggle_requested_(false)
    , last_fullscreen_toggle_request_ms_(0)
    , event_thread_guard_reported_(false)
    , initialized_(false)
    , texture_width_(0)
    , texture_height_(0)
    , seek_requested_(false)
    , seek_ratio_(0.0)
    , seek_delta_requested_(false)
    , seek_delta_seconds_(0.0)
    , volume_change_requested_(false)
    , requested_volume_(1.0f)
    , speed_change_requested_(false)
    , speed_delta_(0.0)
    , speed_reset_requested_(false)
    , subtitle_toggle_requested_(false)
    , ab_repeat_start_requested_(false)
    , ab_repeat_end_requested_(false)
    , ab_repeat_clear_requested_(false)
    , screenshot_requested_(false)
    , step_frame_backward_requested_(false)
    , step_frame_forward_requested_(false)
    , subtitle_delay_change_requested_(false)
    , subtitle_delay_delta_seconds_(0.0)
    , audio_delay_change_requested_(false)
    , audio_delay_delta_seconds_(0.0)
    , next_chapter_requested_(false)
    , previous_chapter_requested_(false)
    , next_item_requested_(false)
    , previous_item_requested_(false)
    , last_nonzero_volume_(1.0f)
    , dragging_seek_(false)
    , dragging_volume_(false)
    , seek_preview_active_(false)
    , seek_preview_ratio_(0.0)
    , overlay_position_(0.0)
    , overlay_duration_(0.0)
    , overlay_volume_(1.0f)
    , overlay_paused_(false) {
}

Display::~Display() {
    close();
}

/// 初始化 SDL 窗口、请求状态和渲染线程。
bool Display::init(int width, int height, const std::string& title) {
    if (initialized_) {
        close();
    }
    
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        std::cerr << "Error: Could not initialize SDL: " << SDL_GetError() << std::endl;
        return false;
    }
    
    const WindowSize window_size = computeInitialWindowSize(width, height);
    width_.store(window_size.width);
    height_.store(window_size.height);
    
    window_ = SDL_CreateWindow(
        title.c_str(),
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        width_.load(),
        height_.load(),
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );
    
    if (!window_) {
        std::cerr << "Error: Could not create SDL window: " << SDL_GetError() << std::endl;
        return false;
    }

    SDL_EventState(SDL_SYSWMEVENT, SDL_ENABLE);
    SDL_SetWindowMinimumSize(window_, kMinWindowWidth, kMinWindowHeight);
    
    initialized_ = true;
    should_quit_.store(false);
    toggle_pause_requested_.store(false);
    minimized_.store(false);
    fullscreen_toggle_requested_.store(false);
    last_fullscreen_toggle_request_ms_.store(0);
    event_thread_id_ = std::this_thread::get_id();
    event_thread_guard_reported_.store(false);
    renderer_reset_requested_.store(false);
    texture_reset_requested_.store(false);
    render_initialized_.store(false);
    render_init_success_.store(false);
    pending_frame_ready_ = false;
    {
        std::lock_guard<std::mutex> lock(request_mutex_);
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
    resetFrameCopyStats();

    render_running_.store(true);
    render_thread_ = std::thread(&Display::renderLoop, this);
    for (int i = 0; i < 200 && !render_initialized_.load(); ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    if (!render_initialized_.load() || !render_init_success_.load()) {
        std::cerr << "Error: Could not initialize render thread resources" << std::endl;
        close();
        return false;
    }
    
    std::cout << "Display initialized: window " << width_.load() << "x" << height_.load()
              << " (source " << width << "x" << height << ")" << std::endl;
    
    return true;
}

/// 停止渲染线程，并释放 SDL 窗口、renderer、texture 和待渲染帧状态。
void Display::close() {
    render_running_.store(false);
    render_queue_cv_.notify_all();
    if (render_thread_.joinable()) {
        render_thread_.join();
    }

    {
        std::lock_guard<std::mutex> lock(render_queue_mutex_);
        pending_frame_ = PendingVideoFrame{};
        pending_frame_ready_ = false;
    }

    if (texture_) {
        SDL_DestroyTexture(texture_);
        texture_ = nullptr;
    }
    
    if (renderer_) {
        SDL_DestroyRenderer(renderer_);
        renderer_ = nullptr;
    }
    
    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }
    
    SDL_Quit();
    
    width_.store(0);
    height_.store(0);
    texture_width_ = 0;
    texture_height_ = 0;
    texture_format_ = SDL_PIXELFORMAT_UNKNOWN;
    minimized_.store(false);
    fullscreen_toggle_requested_.store(false);
    last_fullscreen_toggle_request_ms_.store(0);
    event_thread_id_ = std::thread::id{};
    event_thread_guard_reported_.store(false);
    renderer_reset_requested_.store(false);
    texture_reset_requested_.store(false);
    render_initialized_.store(false);
    render_init_success_.store(false);
    initialized_ = false;
    {
        std::lock_guard<std::mutex> lock(renderer_info_mutex_);
        active_renderer_driver_.clear();
    }
}

/// 根据当前偏好重建 SDL renderer，并记录实际启用的驱动。
bool Display::createRenderer() {
    if (!window_) {
        return false;
    }

    if (texture_) {
        SDL_DestroyTexture(texture_);
        texture_ = nullptr;
    }
    if (renderer_) {
        SDL_DestroyRenderer(renderer_);
        renderer_ = nullptr;
    }

    const char* previous_render_driver_hint = SDL_GetHint(SDL_HINT_RENDER_DRIVER);
    const bool had_previous_render_driver_hint =
        previous_render_driver_hint != nullptr && previous_render_driver_hint[0] != '\0';
    const std::string previous_render_driver =
        had_previous_render_driver_hint ? std::string(previous_render_driver_hint) : std::string{};

    std::string preferred_driver;
    {
        std::lock_guard<std::mutex> lock(renderer_info_mutex_);
        preferred_driver = preferred_renderer_driver_;
    }
    if (!preferred_driver.empty()) {
        SDL_SetHint(SDL_HINT_RENDER_DRIVER, preferred_driver.c_str());
    }

    renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer_) {
        renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_SOFTWARE);
    }

    if (had_previous_render_driver_hint) {
        SDL_SetHint(SDL_HINT_RENDER_DRIVER, previous_render_driver.c_str());
    } else {
        SDL_SetHint(SDL_HINT_RENDER_DRIVER, "");
    }

    if (!renderer_) {
        std::cerr << "Error: Could not create SDL renderer: " << SDL_GetError() << std::endl;
        return false;
    }

    SDL_RendererInfo renderer_info{};
    if (SDL_GetRendererInfo(renderer_, &renderer_info) == 0 && renderer_info.name) {
        std::lock_guard<std::mutex> lock(renderer_info_mutex_);
        active_renderer_driver_ = renderer_info.name;
    } else {
        std::lock_guard<std::mutex> lock(renderer_info_mutex_);
        active_renderer_driver_.clear();
    }

    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
    texture_width_ = 0;
    texture_height_ = 0;
    texture_format_ = SDL_PIXELFORMAT_UNKNOWN;
    texture_reset_requested_.store(true);
    renderer_reset_requested_.store(false);
    return true;
}

bool Display::createTexture(int width, int height, AVPixelFormat format) {
    if (texture_) {
        SDL_DestroyTexture(texture_);
        texture_ = nullptr;
    }

    const Uint32 texture_format = sdlTextureFormatForFrame(format);
    if (texture_format == SDL_PIXELFORMAT_UNKNOWN) {
        std::cerr << "Error: Unsupported SDL texture format for AVPixelFormat " << static_cast<int>(format)
                  << std::endl;
        return false;
    }

    texture_ = SDL_CreateTexture(
        renderer_,
        texture_format,
        SDL_TEXTUREACCESS_STREAMING,
        width,
        height
    );

    if (!texture_) {
        std::cerr << "Error: Could not create SDL texture: " << SDL_GetError() << std::endl;
        return false;
    }

    texture_width_ = std::max(1, width);
    texture_height_ = std::max(1, height);
    texture_format_ = texture_format;
    return true;
}

bool Display::ensureTextureForFrame(int width, int height, AVPixelFormat format) {
    const int safe_width = std::max(1, width);
    const int safe_height = std::max(1, height);
    const Uint32 expected_format = sdlTextureFormatForFrame(format);

    if (expected_format == SDL_PIXELFORMAT_UNKNOWN) {
        return false;
    }

    if (!texture_ ||
        texture_reset_requested_.exchange(false) ||
        texture_width_ != safe_width ||
        texture_height_ != safe_height ||
        texture_format_ != expected_format) {
        return createTexture(safe_width, safe_height, format);
    }
    return true;
}

/// 确保 renderer 和 texture 与当前帧尺寸、格式匹配。
bool Display::ensureRenderResources(const PendingVideoFrame& frame) {
    if (!window_) {
        return false;
    }
    if (!renderer_ || renderer_reset_requested_.exchange(false)) {
        if (!createRenderer()) {
            return false;
        }
    }
    return ensureTextureForFrame(frame.width, frame.height, frame.format);
}

/// 把待渲染帧上传到 SDL texture；支持 YUV420P 和 NV12 直接上传。
bool Display::updateTexture(const PendingVideoFrame& frame) {
    if (!texture_ || !frame.valid) {
        return false;
    }

    int ret = -1;
    switch (frame.format) {
    case AV_PIX_FMT_YUV420P: {
        const uint8_t* y_plane = nullptr;
        const uint8_t* u_plane = nullptr;
        const uint8_t* v_plane = nullptr;
        int y_pitch = frame.y_pitch;
        int u_pitch = frame.u_pitch;
        int v_pitch = frame.v_pitch;

        if (frame.direct_reference) {
            if (!frame.frame_ref || !frame.frame_ref->data[0] || !frame.frame_ref->data[1] || !frame.frame_ref->data[2]) {
                return false;
            }
            y_plane = frame.frame_ref->data[0];
            u_plane = frame.frame_ref->data[1];
            v_plane = frame.frame_ref->data[2];
            y_pitch = frame.frame_ref->linesize[0];
            u_pitch = frame.frame_ref->linesize[1];
            v_pitch = frame.frame_ref->linesize[2];
        } else {
            if (frame.y_plane.empty() || frame.u_plane.empty() || frame.v_plane.empty()) {
                return false;
            }
            y_plane = frame.y_plane.data();
            u_plane = frame.u_plane.data();
            v_plane = frame.v_plane.data();
        }

        ret = SDL_UpdateYUVTexture(texture_, nullptr, y_plane, y_pitch, u_plane, u_pitch, v_plane, v_pitch);
        break;
    }
    case AV_PIX_FMT_NV12: {
        const uint8_t* y_plane = nullptr;
        const uint8_t* uv_plane = nullptr;
        int y_pitch = frame.y_pitch;
        int uv_pitch = frame.u_pitch;

        if (frame.direct_reference) {
            if (!frame.frame_ref || !frame.frame_ref->data[0] || !frame.frame_ref->data[1]) {
                return false;
            }
            y_plane = frame.frame_ref->data[0];
            uv_plane = frame.frame_ref->data[1];
            y_pitch = frame.frame_ref->linesize[0];
            uv_pitch = frame.frame_ref->linesize[1];
        } else {
            if (frame.y_plane.empty() || frame.u_plane.empty()) {
                return false;
            }
            y_plane = frame.y_plane.data();
            uv_plane = frame.u_plane.data();
        }

        ret = SDL_UpdateNVTexture(texture_, nullptr, y_plane, y_pitch, uv_plane, uv_pitch);
        break;
    }
    default:
        return false;
    }

    if (ret < 0) {
        std::cerr << "Error: Could not update texture: " << SDL_GetError() << std::endl;
        return false;
    }

    return true;
}

/// 将提交的 AVFrame 转为待渲染状态，优先持有帧引用而不是深拷贝。
void Display::renderFrame(const uint8_t* data, int width, int height) {
    (void)width;
    (void)height;
    if (!data) {
        return;
    }

    const AVFrame* frame = reinterpret_cast<const AVFrame*>(data);
    if (!frame) {
        return;
    }

    PendingVideoFrame copied_frame;
    if (!copyFrameData(*frame, copied_frame)) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(render_queue_mutex_);
        pending_frame_ = std::move(copied_frame);
        pending_frame_ready_ = true;
    }
    render_queue_cv_.notify_one();
}

/// present 由渲染线程驱动；显式调用时不执行额外操作。
void Display::present() {
    // Present is driven by renderLoop() on the render owner thread.
}

/// 清理待渲染帧状态，避免 seek 或 stop 后继续显示旧帧。
void Display::clear() {
    {
        std::lock_guard<std::mutex> lock(render_queue_mutex_);
        pending_frame_ready_ = false;
        pending_frame_ = PendingVideoFrame{};
    }
    render_queue_cv_.notify_one();
}

Display::FrameCopyStats Display::getFrameCopyStats() const {
    FrameCopyStats stats;
    stats.frames = frame_copy_frames_.load();
    stats.bytes = frame_copy_bytes_.load();
    stats.time_us_total = frame_copy_time_us_total_.load();
    stats.time_us_max = frame_copy_time_us_max_.load();
    return stats;
}

void Display::resetFrameCopyStats() {
    frame_copy_frames_.store(0);
    frame_copy_bytes_.store(0);
    frame_copy_time_us_total_.store(0);
    frame_copy_time_us_max_.store(0);
}

/// 可行时保留 AVFrame 引用；遇到不支持格式或负 stride 时执行深拷贝。
bool Display::copyFrameData(const AVFrame& frame, PendingVideoFrame& out) {
    const AVPixelFormat format = static_cast<AVPixelFormat>(frame.format);
    if (frame.width <= 0 || frame.height <= 0 ||
        sdlTextureFormatForFrame(format) == SDL_PIXELFORMAT_UNKNOWN ||
        !frameHasRequiredPlanes(frame, format)) {
        return false;
    }

    const int width = std::max(1, frame.width);
    const int height = std::max(1, frame.height);
    const int chroma_width = std::max(1, (width + 1) / 2);
    const int chroma_height = std::max(1, (height + 1) / 2);

    out = PendingVideoFrame{};
    out.width = width;
    out.height = height;
    out.format = format;

    if (canReferenceFrameDirectly(frame, format)) {
        AVFrame* frame_ref = av_frame_alloc();
        if (!frame_ref) {
            return false;
        }
        if (av_frame_ref(frame_ref, const_cast<AVFrame*>(&frame)) < 0) {
            av_frame_free(&frame_ref);
            return false;
        }

        out.frame_ref.reset(frame_ref);
        out.y_pitch = frame_ref->linesize[0];
        out.u_pitch = frame_ref->linesize[1];
        out.v_pitch = (format == AV_PIX_FMT_YUV420P) ? frame_ref->linesize[2] : 0;
        out.direct_reference = true;
        out.valid = true;
        return true;
    }

    const auto copy_start = std::chrono::steady_clock::now();
    size_t bytes_copied = 0;

    if (format == AV_PIX_FMT_YUV420P) {
        const int src_y_stride = frame.linesize[0];
        const int src_u_stride = frame.linesize[1];
        const int src_v_stride = frame.linesize[2];
        if (src_y_stride == 0 || src_u_stride == 0 || src_v_stride == 0) {
            return false;
        }

        out.y_pitch = width;
        out.u_pitch = chroma_width;
        out.v_pitch = chroma_width;
        out.y_plane.resize(static_cast<size_t>(out.y_pitch) * static_cast<size_t>(height));
        out.u_plane.resize(static_cast<size_t>(out.u_pitch) * static_cast<size_t>(chroma_height));
        out.v_plane.resize(static_cast<size_t>(out.v_pitch) * static_cast<size_t>(chroma_height));

        const uint8_t* y_src = frame.data[0];
        const uint8_t* u_src = frame.data[1];
        const uint8_t* v_src = frame.data[2];
        if (src_y_stride < 0) {
            y_src += static_cast<ptrdiff_t>(src_y_stride) * (height - 1);
        }
        if (src_u_stride < 0) {
            u_src += static_cast<ptrdiff_t>(src_u_stride) * (chroma_height - 1);
        }
        if (src_v_stride < 0) {
            v_src += static_cast<ptrdiff_t>(src_v_stride) * (chroma_height - 1);
        }

        for (int row = 0; row < height; ++row) {
            std::memcpy(out.y_plane.data() + static_cast<size_t>(row) * static_cast<size_t>(out.y_pitch),
                        y_src + static_cast<ptrdiff_t>(row) * static_cast<ptrdiff_t>(src_y_stride),
                        static_cast<size_t>(width));
        }
        for (int row = 0; row < chroma_height; ++row) {
            std::memcpy(out.u_plane.data() + static_cast<size_t>(row) * static_cast<size_t>(out.u_pitch),
                        u_src + static_cast<ptrdiff_t>(row) * static_cast<ptrdiff_t>(src_u_stride),
                        static_cast<size_t>(chroma_width));
            std::memcpy(out.v_plane.data() + static_cast<size_t>(row) * static_cast<size_t>(out.v_pitch),
                        v_src + static_cast<ptrdiff_t>(row) * static_cast<ptrdiff_t>(src_v_stride),
                        static_cast<size_t>(chroma_width));
        }

        bytes_copied = static_cast<size_t>(width) * static_cast<size_t>(height) +
                       2U * static_cast<size_t>(chroma_width) * static_cast<size_t>(chroma_height);
    } else if (format == AV_PIX_FMT_NV12) {
        const int src_y_stride = frame.linesize[0];
        const int src_uv_stride = frame.linesize[1];
        if (src_y_stride == 0 || src_uv_stride == 0) {
            return false;
        }

        out.y_pitch = width;
        out.u_pitch = width;
        out.v_pitch = 0;
        out.y_plane.resize(static_cast<size_t>(out.y_pitch) * static_cast<size_t>(height));
        out.u_plane.resize(static_cast<size_t>(out.u_pitch) * static_cast<size_t>(chroma_height));

        const uint8_t* y_src = frame.data[0];
        const uint8_t* uv_src = frame.data[1];
        if (src_y_stride < 0) {
            y_src += static_cast<ptrdiff_t>(src_y_stride) * (height - 1);
        }
        if (src_uv_stride < 0) {
            uv_src += static_cast<ptrdiff_t>(src_uv_stride) * (chroma_height - 1);
        }

        for (int row = 0; row < height; ++row) {
            std::memcpy(out.y_plane.data() + static_cast<size_t>(row) * static_cast<size_t>(out.y_pitch),
                        y_src + static_cast<ptrdiff_t>(row) * static_cast<ptrdiff_t>(src_y_stride),
                        static_cast<size_t>(width));
        }
        for (int row = 0; row < chroma_height; ++row) {
            std::memcpy(out.u_plane.data() + static_cast<size_t>(row) * static_cast<size_t>(out.u_pitch),
                        uv_src + static_cast<ptrdiff_t>(row) * static_cast<ptrdiff_t>(src_uv_stride),
                        static_cast<size_t>(width));
        }

        bytes_copied = static_cast<size_t>(width) * static_cast<size_t>(height) +
                       static_cast<size_t>(width) * static_cast<size_t>(chroma_height);
    } else {
        return false;
    }

    out.valid = true;
    const uint64_t copy_us = static_cast<uint64_t>(
        std::max<int64_t>(
            0,
            std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::steady_clock::now() - copy_start)
                .count()));
    frame_copy_frames_.fetch_add(1);
    frame_copy_bytes_.fetch_add(static_cast<uint64_t>(bytes_copied));
    frame_copy_time_us_total_.fetch_add(copy_us);
    updateMaxAtomic(frame_copy_time_us_max_, copy_us);
    return true;
}

/// 渲染主循环：等待新帧或重建请求，并负责 SDL 呈现。
void Display::renderLoop() {
    {
        std::lock_guard<std::mutex> lock(sdl_mutex_);
        render_init_success_.store(createRenderer());
        render_initialized_.store(true);
    }
    render_queue_cv_.notify_all();
    if (!render_init_success_.load()) {
        render_running_.store(false);
        return;
    }

    PendingVideoFrame last_frame;
    bool has_last_frame = false;

    while (render_running_.load()) {
        PendingVideoFrame incoming_frame;
        bool have_new_frame = false;

        {
            std::unique_lock<std::mutex> lock(render_queue_mutex_);
            render_queue_cv_.wait_for(lock, std::chrono::milliseconds(5), [this] {
                return !render_running_.load() || pending_frame_ready_ ||
                       fullscreen_toggle_requested_.load() ||
                       renderer_reset_requested_.load() ||
                       texture_reset_requested_.load();
            });
            if (!render_running_.load()) {
                break;
            }
            if (pending_frame_ready_) {
                incoming_frame = std::move(pending_frame_);
                pending_frame_ = PendingVideoFrame{};
                pending_frame_ready_ = false;
                have_new_frame = incoming_frame.valid;
            }
        }

        std::lock_guard<std::mutex> sdl_lock(sdl_mutex_);
        if (!window_) {
            continue;
        }

        if (fullscreen_toggle_requested_.exchange(false)) {
            const bool next_fullscreen = !fullscreen_.load();
            const int ret = SDL_SetWindowFullscreen(window_,
                                                    next_fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
            if (ret == 0) {
                fullscreen_.store(next_fullscreen);
            } else {
                std::cerr << "Warning: toggle fullscreen failed: " << SDL_GetError() << std::endl;
            }
            updateWindowSizeFromSdl(window_, width_, height_);
            minimized_.store(false);
            texture_reset_requested_.store(true);
        }

        if (minimized_.load()) {
            continue;
        }

        const bool redraw_requested =
            renderer_reset_requested_.load() || texture_reset_requested_.load();

        PendingVideoFrame* frame_to_render = nullptr;
        if (have_new_frame) {
            frame_to_render = &incoming_frame;
        } else if (redraw_requested && has_last_frame) {
            frame_to_render = &last_frame;
        }

        if (frame_to_render) {
            if (!ensureRenderResources(*frame_to_render)) {
                continue;
            }
            int render_width = width_.load();
            int render_height = height_.load();
            if (SDL_GetRendererOutputSize(renderer_, &render_width, &render_height) == 0) {
                width_.store(std::max(1, render_width));
                height_.store(std::max(1, render_height));
            } else {
                render_width = std::max(1, width_.load());
                render_height = std::max(1, height_.load());
            }
            SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
            SDL_RenderClear(renderer_);
            if (!updateTexture(*frame_to_render)) {
                texture_reset_requested_.store(true);
                continue;
            }
            const SDL_Rect dst_rect = computeRenderRect(render_width, render_height,
                                                        frame_to_render->width, frame_to_render->height);
            if (SDL_RenderCopy(renderer_, texture_, nullptr, &dst_rect) < 0) {
                renderer_reset_requested_.store(true);
                continue;
            }
            drawSubtitleOverlay(render_width, render_height);
            drawControls(render_width, render_height);
            SDL_RenderPresent(renderer_);

            if (have_new_frame) {
                last_frame = std::move(incoming_frame);
                has_last_frame = last_frame.valid;
            }
        }
    }
}

/// UI 线程事件泵，把键盘和鼠标输入转换为一次性控制请求。
void Display::handleEvents() {
    if (event_thread_id_ != std::thread::id{} && event_thread_id_ != std::this_thread::get_id()) {
        if (!event_thread_guard_reported_.exchange(true)) {
            std::cerr << "Display::handleEvents called from non-event thread; ignoring event pump" << std::endl;
        }
        return;
    }

    SDL_Event event;
    
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                should_quit_.store(true);
                break;
                
            case SDL_KEYDOWN:
            {
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

                if (key_code == SDLK_f || event.key.keysym.scancode == SDL_SCANCODE_F) {
                    toggleFullscreen();
                    break;
                }

                std::optional<input::PlayerAction> action = hotkey_manager_.actionForKey(key_code);
                if (!action && key_code == SDLK_EQUALS) {
                    action = input::PlayerAction::VolumeUp;
                } else if (!action && key_code == SDLK_MINUS) {
                    action = input::PlayerAction::VolumeDown;
                }
                if (!action) {
                    break;
                }

                switch (*action) {
                case input::PlayerAction::PlayPause:
                    toggle_pause_requested_.store(true);
                    break;
                case input::PlayerAction::SeekBackward:
                case input::PlayerAction::SeekForward: {
                    const bool ctrl_pressed = (event.key.keysym.mod & KMOD_CTRL) != 0;
                    const double delta = ctrl_pressed ? kSeekStepSecondsCtrl : kSeekStepSeconds;
                    std::lock_guard<std::mutex> lock(request_mutex_);
                    seek_delta_seconds_ += (*action == input::PlayerAction::SeekForward) ? delta : -delta;
                    seek_delta_requested_ = true;
                    break;
                }
                case input::PlayerAction::VolumeUp:
                case input::PlayerAction::VolumeDown: {
                    const float direction = (*action == input::PlayerAction::VolumeUp) ? 1.0f : -1.0f;
                    const float next = clampVolume(overlay_volume_.load() + direction * kVolumeStep);
                    {
                        std::lock_guard<std::mutex> lock(request_mutex_);
                        requested_volume_ = next;
                        volume_change_requested_ = true;
                    }
                    overlay_volume_.store(next);
                    break;
                }
                case input::PlayerAction::ToggleMute: {
                    const float current = clampVolume(overlay_volume_.load());
                    float next = current;
                    if (current > 0.0001f) {
                        last_nonzero_volume_ = current;
                        next = 0.0f;
                    } else {
                        next = clampVolume(last_nonzero_volume_);
                        if (next < 0.0001f) {
                            next = 0.5f;
                        }
                    }
                    {
                        std::lock_guard<std::mutex> lock(request_mutex_);
                        requested_volume_ = next;
                        volume_change_requested_ = true;
                    }
                    overlay_volume_.store(next);
                    break;
                }
                case input::PlayerAction::SpeedDown:
                case input::PlayerAction::SpeedUp: {
                    const double delta = (*action == input::PlayerAction::SpeedUp) ? kSpeedStep : -kSpeedStep;
                    std::lock_guard<std::mutex> lock(request_mutex_);
                    speed_delta_ += delta;
                    speed_change_requested_ = true;
                    break;
                }
                case input::PlayerAction::SpeedReset: {
                    std::lock_guard<std::mutex> lock(request_mutex_);
                    speed_reset_requested_ = true;
                    break;
                }
                case input::PlayerAction::PreviousItem: {
                    std::lock_guard<std::mutex> lock(request_mutex_);
                    previous_item_requested_ = true;
                    break;
                }
                case input::PlayerAction::NextItem: {
                    std::lock_guard<std::mutex> lock(request_mutex_);
                    next_item_requested_ = true;
                    break;
                }
                case input::PlayerAction::ToggleSubtitle: {
                    std::lock_guard<std::mutex> lock(request_mutex_);
                    subtitle_toggle_requested_ = true;
                    break;
                }
                case input::PlayerAction::SetABRepeatStart: {
                    std::lock_guard<std::mutex> lock(request_mutex_);
                    ab_repeat_start_requested_ = true;
                    break;
                }
                case input::PlayerAction::SetABRepeatEnd: {
                    std::lock_guard<std::mutex> lock(request_mutex_);
                    ab_repeat_end_requested_ = true;
                    break;
                }
                case input::PlayerAction::ClearABRepeat: {
                    std::lock_guard<std::mutex> lock(request_mutex_);
                    ab_repeat_clear_requested_ = true;
                    break;
                }
                case input::PlayerAction::TakeScreenshot: {
                    std::lock_guard<std::mutex> lock(request_mutex_);
                    screenshot_requested_ = true;
                    break;
                }
                case input::PlayerAction::StepFrameBackward: {
                    std::lock_guard<std::mutex> lock(request_mutex_);
                    step_frame_backward_requested_ = true;
                    break;
                }
                case input::PlayerAction::StepFrameForward: {
                    std::lock_guard<std::mutex> lock(request_mutex_);
                    step_frame_forward_requested_ = true;
                    break;
                }
                case input::PlayerAction::SubtitleDelayDown:
                case input::PlayerAction::SubtitleDelayUp: {
                    const double delta = (*action == input::PlayerAction::SubtitleDelayUp) ? 0.1 : -0.1;
                    const bool ctrl_pressed = (event.key.keysym.mod & KMOD_CTRL) != 0;
                    std::lock_guard<std::mutex> lock(request_mutex_);
                    if (ctrl_pressed) {
                        audio_delay_delta_seconds_ += delta;
                        audio_delay_change_requested_ = true;
                    } else {
                        subtitle_delay_delta_seconds_ += delta;
                        subtitle_delay_change_requested_ = true;
                    }
                    break;
                }
                case input::PlayerAction::SeekTo10Percent:
                case input::PlayerAction::SeekTo20Percent:
                case input::PlayerAction::SeekTo30Percent:
                case input::PlayerAction::SeekTo40Percent:
                case input::PlayerAction::SeekTo50Percent:
                case input::PlayerAction::SeekTo60Percent:
                case input::PlayerAction::SeekTo70Percent:
                case input::PlayerAction::SeekTo80Percent:
                case input::PlayerAction::SeekTo90Percent: {
                    double ratio = 0.0;
                    switch (*action) {
                    case input::PlayerAction::SeekTo10Percent:
                        ratio = 0.1;
                        break;
                    case input::PlayerAction::SeekTo20Percent:
                        ratio = 0.2;
                        break;
                    case input::PlayerAction::SeekTo30Percent:
                        ratio = 0.3;
                        break;
                    case input::PlayerAction::SeekTo40Percent:
                        ratio = 0.4;
                        break;
                    case input::PlayerAction::SeekTo50Percent:
                        ratio = 0.5;
                        break;
                    case input::PlayerAction::SeekTo60Percent:
                        ratio = 0.6;
                        break;
                    case input::PlayerAction::SeekTo70Percent:
                        ratio = 0.7;
                        break;
                    case input::PlayerAction::SeekTo80Percent:
                        ratio = 0.8;
                        break;
                    case input::PlayerAction::SeekTo90Percent:
                        ratio = 0.9;
                        break;
                    default:
                        break;
                    }
                    std::lock_guard<std::mutex> lock(request_mutex_);
                    seek_ratio_ = ratio;
                    seek_requested_ = true;
                    seek_preview_active_ = false;
                    break;
                }
                case input::PlayerAction::PreviousChapter: {
                    std::lock_guard<std::mutex> lock(request_mutex_);
                    previous_chapter_requested_ = true;
                    break;
                }
                case input::PlayerAction::NextChapter: {
                    std::lock_guard<std::mutex> lock(request_mutex_);
                    next_chapter_requested_ = true;
                    break;
                }
                case input::PlayerAction::ToggleFullscreen:
                    toggleFullscreen();
                    break;
                case input::PlayerAction::Quit:
                    should_quit_.store(true);
                    break;
                }
                break;
            }
            case SDL_SYSWMEVENT:
#if defined(_WIN32)
                if (event.syswm.msg && event.syswm.msg->subsystem == SDL_SYSWM_WINDOWS) {
                    constexpr unsigned int kWmKeyDown = 0x0100;
                    constexpr unsigned int kWmSysKeyDown = 0x0104;
                    const unsigned int message = event.syswm.msg->msg.win.msg;
                    const uintptr_t key = static_cast<uintptr_t>(event.syswm.msg->msg.win.wParam);
                    if ((message == kWmKeyDown || message == kWmSysKeyDown) &&
                        (key == static_cast<uintptr_t>('F'))) {
                        toggleFullscreen();
                    }
                }
#endif
                break;
            case SDL_DROPFILE:
            {
                std::lock_guard<std::mutex> lock(request_mutex_);
                open_file_path_ = event.drop.file ? event.drop.file : "";
                open_file_requested_ = !open_file_path_.empty();
                if (event.drop.file) {
                    SDL_free(event.drop.file);
                }
                break;
            }

            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_MINIMIZED ||
                    event.window.event == SDL_WINDOWEVENT_HIDDEN) {
                    minimized_.store(true);
                    render_queue_cv_.notify_one();
                } else if (event.window.event == SDL_WINDOWEVENT_RESTORED ||
                           event.window.event == SDL_WINDOWEVENT_SHOWN ||
                           event.window.event == SDL_WINDOWEVENT_MAXIMIZED ||
                           event.window.event == SDL_WINDOWEVENT_RESIZED ||
                           event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                    minimized_.store(false);
                    texture_reset_requested_.store(true);
                    render_queue_cv_.notify_one();
                }

                if (event.window.event == SDL_WINDOWEVENT_RESIZED ||
                    event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED ||
                    event.window.event == SDL_WINDOWEVENT_MAXIMIZED ||
                    event.window.event == SDL_WINDOWEVENT_RESTORED) {
                    updateWindowSizeFromSdl(window_, width_, height_);
                }
                break;
#if defined(SDL_RENDER_TARGETS_RESET)
            case SDL_RENDER_TARGETS_RESET:
                renderer_reset_requested_.store(true);
                texture_reset_requested_.store(true);
                render_queue_cv_.notify_one();
                break;
#endif
#if defined(SDL_RENDER_DEVICE_RESET)
            case SDL_RENDER_DEVICE_RESET:
                renderer_reset_requested_.store(true);
                texture_reset_requested_.store(true);
                render_queue_cv_.notify_one();
                break;
#endif

            case SDL_MOUSEBUTTONDOWN:
                if (event.button.button != SDL_BUTTON_LEFT) {
                    break;
                }
                if (pointInRect(event.button.x, event.button.y, computeControlLayout(width_.load(), height_.load()).progress_track)) {
                    dragging_seek_ = true;
                    updateSeekFromMouse(event.button.x, false);
                } else if (pointInRect(event.button.x, event.button.y, computeControlLayout(width_.load(), height_.load()).volume_track)) {
                    dragging_volume_ = true;
                    updateVolumeFromMouse(event.button.x);
                }
                break;

            case SDL_MOUSEMOTION:
                if (dragging_seek_) {
                    updateSeekFromMouse(event.motion.x, false);
                }
                if (dragging_volume_) {
                    updateVolumeFromMouse(event.motion.x);
                }
                break;

            case SDL_MOUSEBUTTONUP:
                if (event.button.button != SDL_BUTTON_LEFT) {
                    break;
                }
                if (dragging_seek_) {
                    updateSeekFromMouse(event.button.x, true);
                    dragging_seek_ = false;
                }
                if (dragging_volume_) {
                    updateVolumeFromMouse(event.button.x);
                    dragging_volume_ = false;
                }
                break;
                
            default:
                break;
        }
    }
}

bool Display::consumeTogglePauseRequest() {
    return toggle_pause_requested_.exchange(false);
}

bool Display::consumeSeekRequest(double& normalized_position) {
    std::lock_guard<std::mutex> lock(request_mutex_);
    if (!seek_requested_) {
        return false;
    }
    normalized_position = seek_ratio_;
    seek_requested_ = false;
    return true;
}

bool Display::consumeSeekDeltaRequest(double& delta_seconds) {
    std::lock_guard<std::mutex> lock(request_mutex_);
    if (!seek_delta_requested_) {
        return false;
    }
    delta_seconds = seek_delta_seconds_;
    seek_delta_seconds_ = 0.0;
    seek_delta_requested_ = false;
    return true;
}

bool Display::consumeVolumeChangeRequest(float& volume) {
    std::lock_guard<std::mutex> lock(request_mutex_);
    if (!volume_change_requested_) {
        return false;
    }
    volume = requested_volume_;
    volume_change_requested_ = false;
    return true;
}

bool Display::consumeSpeedChangeRequest(double& speed_delta) {
    std::lock_guard<std::mutex> lock(request_mutex_);
    if (!speed_change_requested_) {
        return false;
    }
    speed_delta = speed_delta_;
    speed_delta_ = 0.0;
    speed_change_requested_ = false;
    return true;
}

bool Display::consumeResetSpeedRequest() {
    std::lock_guard<std::mutex> lock(request_mutex_);
    if (!speed_reset_requested_) {
        return false;
    }
    speed_reset_requested_ = false;
    return true;
}

bool Display::consumeToggleSubtitleRequest() {
    std::lock_guard<std::mutex> lock(request_mutex_);
    if (!subtitle_toggle_requested_) {
        return false;
    }
    subtitle_toggle_requested_ = false;
    return true;
}

bool Display::consumeSetABRepeatStartRequest() {
    std::lock_guard<std::mutex> lock(request_mutex_);
    if (!ab_repeat_start_requested_) {
        return false;
    }
    ab_repeat_start_requested_ = false;
    return true;
}

bool Display::consumeSetABRepeatEndRequest() {
    std::lock_guard<std::mutex> lock(request_mutex_);
    if (!ab_repeat_end_requested_) {
        return false;
    }
    ab_repeat_end_requested_ = false;
    return true;
}

bool Display::consumeClearABRepeatRequest() {
    std::lock_guard<std::mutex> lock(request_mutex_);
    if (!ab_repeat_clear_requested_) {
        return false;
    }
    ab_repeat_clear_requested_ = false;
    return true;
}

bool Display::consumeScreenshotRequest() {
    std::lock_guard<std::mutex> lock(request_mutex_);
    if (!screenshot_requested_) {
        return false;
    }
    screenshot_requested_ = false;
    return true;
}

bool Display::consumeStepFrameBackwardRequest() {
    std::lock_guard<std::mutex> lock(request_mutex_);
    if (!step_frame_backward_requested_) {
        return false;
    }
    step_frame_backward_requested_ = false;
    return true;
}

bool Display::consumeStepFrameForwardRequest() {
    std::lock_guard<std::mutex> lock(request_mutex_);
    if (!step_frame_forward_requested_) {
        return false;
    }
    step_frame_forward_requested_ = false;
    return true;
}

bool Display::consumeSubtitleDelayChangeRequest(double& delta_seconds) {
    std::lock_guard<std::mutex> lock(request_mutex_);
    if (!subtitle_delay_change_requested_) {
        return false;
    }
    delta_seconds = subtitle_delay_delta_seconds_;
    subtitle_delay_delta_seconds_ = 0.0;
    subtitle_delay_change_requested_ = false;
    return true;
}

bool Display::consumeAudioDelayChangeRequest(double& delta_seconds) {
    std::lock_guard<std::mutex> lock(request_mutex_);
    if (!audio_delay_change_requested_) {
        return false;
    }
    delta_seconds = audio_delay_delta_seconds_;
    audio_delay_delta_seconds_ = 0.0;
    audio_delay_change_requested_ = false;
    return true;
}

bool Display::consumeNextChapterRequest() {
    std::lock_guard<std::mutex> lock(request_mutex_);
    if (!next_chapter_requested_) {
        return false;
    }
    next_chapter_requested_ = false;
    return true;
}

bool Display::consumePreviousChapterRequest() {
    std::lock_guard<std::mutex> lock(request_mutex_);
    if (!previous_chapter_requested_) {
        return false;
    }
    previous_chapter_requested_ = false;
    return true;
}

bool Display::consumeNextItemRequest() {
    std::lock_guard<std::mutex> lock(request_mutex_);
    if (!next_item_requested_) {
        return false;
    }
    next_item_requested_ = false;
    return true;
}

bool Display::consumePreviousItemRequest() {
    std::lock_guard<std::mutex> lock(request_mutex_);
    if (!previous_item_requested_) {
        return false;
    }
    previous_item_requested_ = false;
    return true;
}

bool Display::consumeOpenFileRequest(std::string& path) {
    std::lock_guard<std::mutex> lock(request_mutex_);
    if (!open_file_requested_) {
        return false;
    }
    path = open_file_path_;
    open_file_path_.clear();
    open_file_requested_ = false;
    return !path.empty();
}

/// 更新渲染线程使用的叠加层状态，用于绘制控制条和字幕。
void Display::setOverlayState(double position, double duration, float volume, bool paused) {
    overlay_position_.store(std::max(0.0, position));
    overlay_duration_.store(std::max(0.0, duration));
    const float clamped_volume = clampVolume(volume);
    overlay_volume_.store(clamped_volume);
    if (clamped_volume > 0.0001f) {
        last_nonzero_volume_ = clamped_volume;
    }
    overlay_paused_.store(paused);
}

void Display::setSubtitleText(const std::string& text) {
    std::lock_guard<std::mutex> lock(subtitle_mutex_);
    subtitle_text_ = text;
}

void Display::setHotkeyManager(const input::HotkeyManager& hotkey_manager) {
    std::lock_guard<std::mutex> lock(request_mutex_);
    hotkey_manager_ = hotkey_manager;
}

void Display::setPreferredRendererDriver(const std::string& driver_name) {
    std::lock_guard<std::mutex> lock(renderer_info_mutex_);
    preferred_renderer_driver_ = driver_name;
}

std::string Display::currentRendererDriver() const {
    std::lock_guard<std::mutex> lock(renderer_info_mutex_);
    return active_renderer_driver_;
}

bool Display::isUsingRendererDriver(const std::string& driver_name) const {
    if (driver_name.empty()) {
        return false;
    }
    return toLowerAscii(currentRendererDriver()) == toLowerAscii(driver_name);
}

void Display::drawSubtitleOverlay(int window_width, int window_height) {
    if (!renderer_) {
        return;
    }

    std::string subtitle_text;
    {
        std::lock_guard<std::mutex> lock(subtitle_mutex_);
        subtitle_text = subtitle_text_;
    }

    if (subtitle_text.empty()) {
        return;
    }

    constexpr size_t kMaxSubtitleLines = 3;
    auto lines = splitSubtitleLines(subtitle_text, kMaxSubtitleLines);
    if (lines.empty()) {
        return;
    }

    const int scale = std::max(1, std::min(3, window_width / 520));
    const int glyph_width = 5 * scale;
    const int glyph_height = 7 * scale;
    const int glyph_spacing = scale;
    const int line_spacing = scale * 2;
    const int horizontal_margin = std::max(16, scale * 8);
    const int char_step = glyph_width + glyph_spacing;
    const size_t max_chars = std::max<size_t>(8, static_cast<size_t>(std::max(1, window_width - horizontal_margin * 2) / std::max(1, char_step)));

    int content_width = 0;
    for (std::string& line : lines) {
        if (line.size() > max_chars) {
            if (max_chars > 3) {
                line = line.substr(0, max_chars - 3) + "...";
            } else {
                line = line.substr(0, max_chars);
            }
        }
        const int line_width = static_cast<int>(line.empty() ? 0 : line.size() * char_step - glyph_spacing);
        content_width = std::max(content_width, line_width);
    }

    const int content_height =
        static_cast<int>(lines.size()) * glyph_height + static_cast<int>(std::max<size_t>(0, lines.size() - 1)) * line_spacing;

    if (content_width <= 0 || content_height <= 0) {
        return;
    }

    const ControlLayout controls = computeControlLayout(window_width, window_height);
    const int panel_padding = scale * 4;
    const int panel_width = content_width + panel_padding * 2;
    const int panel_height = content_height + panel_padding * 2;
    const int panel_x = std::max(0, (window_width - panel_width) / 2);
    const int panel_bottom = std::max(0, controls.panel.y - scale * 3);
    const int panel_y = std::max(0, panel_bottom - panel_height);

    SDL_Rect subtitle_panel{panel_x, panel_y, panel_width, panel_height};
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 150);
    SDL_RenderFillRect(renderer_, &subtitle_panel);

    SDL_Color shadow{0, 0, 0, 230};
    SDL_Color front{242, 242, 242, 245};

    for (size_t line_index = 0; line_index < lines.size(); ++line_index) {
        const std::string& line = lines[line_index];
        const int line_width = static_cast<int>(line.empty() ? 0 : line.size() * char_step - glyph_spacing);
        int x = panel_x + (panel_width - line_width) / 2;
        const int y = panel_y + panel_padding + static_cast<int>(line_index) * (glyph_height + line_spacing);
        for (char ch : line) {
            drawGlyph5x7(renderer_, ch, x + 1, y + 1, scale, shadow);
            drawGlyph5x7(renderer_, ch, x, y, scale, front);
            x += char_step;
        }
    }
}

Display::ControlLayout Display::computeControlLayout(int window_width, int window_height) const {
    const int safe_width = std::max(1, window_width);
    const int safe_height = std::max(1, window_height);

    ControlLayout layout{};
    layout.panel = {
        kControlPanelInset,
        std::max(0, safe_height - kControlPanelHeight - kControlPanelInset),
        std::max(1, safe_width - (kControlPanelInset * 2)),
        kControlPanelHeight
    };

    const int track_y = layout.panel.y + (layout.panel.h - kBarHeight) / 2;
    const int volume_width = std::min(kVolumeBarWidth, std::max(56, layout.panel.w / 5));
    int progress_width = layout.panel.w - (kControlPadding * 2) - volume_width - kControlGap;
    progress_width = std::max(kMinProgressBarWidth, progress_width);

    layout.progress_track = {
        layout.panel.x + kControlPadding,
        track_y,
        std::min(progress_width, std::max(1, layout.panel.w - (kControlPadding * 2))),
        kBarHeight
    };
    layout.volume_track = {
        layout.panel.x + layout.panel.w - kControlPadding - volume_width,
        track_y,
        std::max(1, volume_width),
        kBarHeight
    };
    return layout;
}

void Display::drawControls(int window_width, int window_height) {
    if (!renderer_) {
        return;
    }

    const ControlLayout layout = computeControlLayout(window_width, window_height);
    double progress = 0.0;
    const double duration = overlay_duration_.load();
    if (duration > 0.0) {
        progress = overlay_position_.load() / duration;
    }

    float volume = overlay_volume_.load();
    bool paused = overlay_paused_.load();

    {
        std::lock_guard<std::mutex> lock(request_mutex_);
        if (seek_preview_active_) {
            progress = seek_preview_ratio_;
        }
        if (dragging_volume_) {
            volume = requested_volume_;
        }
    }

    progress = clampRatio(progress);
    volume = clampVolume(volume);

    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 140);
    SDL_RenderFillRect(renderer_, &layout.panel);

    SDL_SetRenderDrawColor(renderer_, 85, 85, 85, 230);
    SDL_RenderFillRect(renderer_, &layout.progress_track);

    SDL_Rect progress_fill = layout.progress_track;
    progress_fill.w = std::max(1, static_cast<int>(std::lround(progress * static_cast<double>(layout.progress_track.w))));
    SDL_SetRenderDrawColor(renderer_, 245, 245, 245, 235);
    SDL_RenderFillRect(renderer_, &progress_fill);

    SDL_SetRenderDrawColor(renderer_, 85, 85, 85, 230);
    SDL_RenderFillRect(renderer_, &layout.volume_track);

    SDL_Rect volume_fill = layout.volume_track;
    volume_fill.w = std::max(1, static_cast<int>(std::lround(volume * static_cast<double>(layout.volume_track.w))));
    SDL_SetRenderDrawColor(renderer_, 130, 220, 160, 235);
    SDL_RenderFillRect(renderer_, &volume_fill);

    if (paused) {
        SDL_Rect pause_flag{
            layout.panel.x + layout.panel.w / 2 - 6,
            layout.panel.y + 8,
            12,
            6
        };
        SDL_SetRenderDrawColor(renderer_, 255, 204, 102, 235);
        SDL_RenderFillRect(renderer_, &pause_flag);
    }
}

/// 根据鼠标位置更新 seek 预览，并在提交时记录一次 seek 请求。
void Display::updateSeekFromMouse(int mouse_x, bool commit) {
    const ControlLayout layout = computeControlLayout(width_.load(), height_.load());
    if (layout.progress_track.w <= 0) {
        return;
    }
    const double ratio = clampRatio(
        static_cast<double>(mouse_x - layout.progress_track.x) / static_cast<double>(layout.progress_track.w));

    std::lock_guard<std::mutex> lock(request_mutex_);
    seek_preview_active_ = true;
    seek_preview_ratio_ = ratio;
    if (commit) {
        seek_ratio_ = ratio;
        seek_requested_ = true;
        seek_preview_active_ = false;
    }
}

/// 根据鼠标位置更新音量请求，并刷新叠加层音量显示。
void Display::updateVolumeFromMouse(int mouse_x) {
    const ControlLayout layout = computeControlLayout(width_.load(), height_.load());
    if (layout.volume_track.w <= 0) {
        return;
    }
    const float next_volume = static_cast<float>(clampRatio(
        static_cast<double>(mouse_x - layout.volume_track.x) / static_cast<double>(layout.volume_track.w)));
    {
        std::lock_guard<std::mutex> lock(request_mutex_);
        requested_volume_ = next_volume;
        volume_change_requested_ = true;
    }
    overlay_volume_.store(next_volume);
}

/// 记录全屏切换请求；实际 SDL 切换由渲染线程执行。
void Display::toggleFullscreen() {
    const int64_t now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                               std::chrono::steady_clock::now().time_since_epoch())
                               .count();
    const int64_t previous_ms = last_fullscreen_toggle_request_ms_.load();
    if (now_ms - previous_ms < 250) {
        return;
    }
    last_fullscreen_toggle_request_ms_.store(now_ms);
    fullscreen_toggle_requested_.store(true);
    render_queue_cv_.notify_one();
}

} // namespace vp
