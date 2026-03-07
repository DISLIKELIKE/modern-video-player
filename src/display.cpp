#include "display.h"

#include <algorithm>
#include <cmath>
#include <iostream>

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
    , initialized_(false)
    , seek_requested_(false)
    , seek_ratio_(0.0)
    , seek_delta_requested_(false)
    , seek_delta_seconds_(0.0)
    , volume_change_requested_(false)
    , requested_volume_(1.0f)
    , speed_change_requested_(false)
    , speed_delta_(0.0)
    , speed_reset_requested_(false)
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

bool Display::init(int width, int height, const std::string& title) {
    if (initialized_) {
        close();
    }
    
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        std::cerr << "Error: Could not initialize SDL: " << SDL_GetError() << std::endl;
        return false;
    }
    
    const WindowSize window_size = computeInitialWindowSize(width, height);
    width_ = window_size.width;
    height_ = window_size.height;
    
    window_ = SDL_CreateWindow(
        title.c_str(),
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        width_,
        height_,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );
    
    if (!window_) {
        std::cerr << "Error: Could not create SDL window: " << SDL_GetError() << std::endl;
        return false;
    }

    SDL_SetWindowMinimumSize(window_, kMinWindowWidth, kMinWindowHeight);
    
    renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer_) {
        renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_SOFTWARE);
    }
    if (!renderer_) {
        std::cerr << "Error: Could not create SDL renderer: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window_);
        window_ = nullptr;
        return false;
    }
    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
    
    if (!createTexture(width, height)) {
        SDL_DestroyRenderer(renderer_);
        SDL_DestroyWindow(window_);
        renderer_ = nullptr;
        window_ = nullptr;
        return false;
    }
    
    initialized_ = true;
    should_quit_.store(false);
    toggle_pause_requested_.store(false);
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
        next_item_requested_ = false;
        previous_item_requested_ = false;
        last_nonzero_volume_ = 1.0f;
        dragging_seek_ = false;
        dragging_volume_ = false;
        seek_preview_active_ = false;
        seek_preview_ratio_ = 0.0;
    }
    
    std::cout << "Display initialized: window " << width_ << "x" << height_
              << " (source " << width << "x" << height << ")" << std::endl;
    
    return true;
}

void Display::close() {
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
    
    width_ = 0;
    height_ = 0;
    initialized_ = false;
}

bool Display::createTexture(int width, int height) {
    if (texture_) {
        SDL_DestroyTexture(texture_);
        texture_ = nullptr;
    }
    
    texture_ = SDL_CreateTexture(
        renderer_,
        SDL_PIXELFORMAT_IYUV,
        SDL_TEXTUREACCESS_STREAMING,
        width,
        height
    );
    
    if (!texture_) {
        std::cerr << "Error: Could not create SDL texture: " << SDL_GetError() << std::endl;
        return false;
    }
    
    return true;
}

bool Display::updateTexture(const uint8_t* data, int width, int height) {
    (void)width;
    (void)height;
    if (!texture_ || !data) {
        return false;
    }
    
    const AVFrame* frame = reinterpret_cast<const AVFrame*>(data);
    
    const int ret = SDL_UpdateYUVTexture(
        texture_,
        nullptr,
        frame->data[0], frame->linesize[0],
        frame->data[1], frame->linesize[1],
        frame->data[2], frame->linesize[2]
    );
    
    if (ret < 0) {
        std::cerr << "Error: Could not update texture: " << SDL_GetError() << std::endl;
        return false;
    }
    
    return true;
}

void Display::renderFrame(const uint8_t* data, int width, int height) {
    if (!renderer_ || !texture_ || !data) {
        return;
    }
    
    handleEvents();
    SDL_RenderClear(renderer_);
    if (!updateTexture(data, width, height)) {
        return;
    }
    
    const SDL_Rect dst_rect = computeRenderRect(width_, height_, width, height);
    SDL_RenderCopy(renderer_, texture_, nullptr, &dst_rect);
    drawControls(width_, height_);
}

void Display::present() {
    if (renderer_) {
        SDL_RenderPresent(renderer_);
    }
}

void Display::clear() {
    if (renderer_) {
        SDL_RenderClear(renderer_);
    }
}

void Display::handleEvents() {
    SDL_Event event;
    
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                should_quit_.store(true);
                break;
                
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_ESCAPE:
                        if (fullscreen_) {
                            toggleFullscreen();
                        } else {
                            should_quit_.store(true);
                        }
                        break;
                    case SDLK_q:
                        should_quit_.store(true);
                        break;
                    case SDLK_SPACE:
                        toggle_pause_requested_.store(true);
                        break;
                    case SDLK_RETURN:
                    case SDLK_KP_ENTER:
                    case SDLK_f:
                        toggleFullscreen();
                        break;
                    case SDLK_LEFT:
                    case SDLK_RIGHT: {
                        const bool ctrl_pressed = (event.key.keysym.mod & KMOD_CTRL) != 0;
                        const double delta = ctrl_pressed ? kSeekStepSecondsCtrl : kSeekStepSeconds;
                        std::lock_guard<std::mutex> lock(request_mutex_);
                        seek_delta_seconds_ += (event.key.keysym.sym == SDLK_RIGHT) ? delta : -delta;
                        seek_delta_requested_ = true;
                        break;
                    }
                    case SDLK_UP:
                    case SDLK_EQUALS: {
                        const float next = clampVolume(overlay_volume_.load() + kVolumeStep);
                        {
                            std::lock_guard<std::mutex> lock(request_mutex_);
                            requested_volume_ = next;
                            volume_change_requested_ = true;
                        }
                        overlay_volume_.store(next);
                        break;
                    }
                    case SDLK_DOWN:
                    case SDLK_MINUS: {
                        const float next = clampVolume(overlay_volume_.load() - kVolumeStep);
                        {
                            std::lock_guard<std::mutex> lock(request_mutex_);
                            requested_volume_ = next;
                            volume_change_requested_ = true;
                        }
                        overlay_volume_.store(next);
                        break;
                    }
                    case SDLK_m: {
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
                    case SDLK_LEFTBRACKET:
                    case SDLK_RIGHTBRACKET: {
                        std::lock_guard<std::mutex> lock(request_mutex_);
                        speed_delta_ += (event.key.keysym.sym == SDLK_RIGHTBRACKET) ? kSpeedStep : -kSpeedStep;
                        speed_change_requested_ = true;
                        break;
                    }
                    case SDLK_r: {
                        std::lock_guard<std::mutex> lock(request_mutex_);
                        speed_reset_requested_ = true;
                        break;
                    }
                    case SDLK_PAGEUP: {
                        std::lock_guard<std::mutex> lock(request_mutex_);
                        previous_item_requested_ = true;
                        break;
                    }
                    case SDLK_PAGEDOWN: {
                        std::lock_guard<std::mutex> lock(request_mutex_);
                        next_item_requested_ = true;
                        break;
                    }
                    default:
                        break;
                }
                break;
                
            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_RESIZED ||
                    event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED ||
                    event.window.event == SDL_WINDOWEVENT_MAXIMIZED ||
                    event.window.event == SDL_WINDOWEVENT_RESTORED) {
                    int new_width = event.window.data1;
                    int new_height = event.window.data2;
                    if (window_ && (new_width <= 0 || new_height <= 0)) {
                        SDL_GetWindowSize(window_, &new_width, &new_height);
                    }
                    width_ = std::max(1, new_width);
                    height_ = std::max(1, new_height);
                }
                break;

            case SDL_MOUSEBUTTONDOWN:
                if (event.button.button != SDL_BUTTON_LEFT) {
                    break;
                }
                if (pointInRect(event.button.x, event.button.y, computeControlLayout(width_, height_).progress_track)) {
                    dragging_seek_ = true;
                    updateSeekFromMouse(event.button.x, false);
                } else if (pointInRect(event.button.x, event.button.y, computeControlLayout(width_, height_).volume_track)) {
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

void Display::updateSeekFromMouse(int mouse_x, bool commit) {
    const ControlLayout layout = computeControlLayout(width_, height_);
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

void Display::updateVolumeFromMouse(int mouse_x) {
    const ControlLayout layout = computeControlLayout(width_, height_);
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

void Display::toggleFullscreen() {
    if (!window_) {
        return;
    }
    
    fullscreen_ = !fullscreen_;

    if (fullscreen_) {
        SDL_SetWindowFullscreen(window_, SDL_WINDOW_FULLSCREEN_DESKTOP);
    } else {
        SDL_SetWindowFullscreen(window_, 0);
    }

    int updated_width = width_;
    int updated_height = height_;
    SDL_GetWindowSize(window_, &updated_width, &updated_height);
    width_ = std::max(1, updated_width);
    height_ = std::max(1, updated_height);
}

} // namespace vp
