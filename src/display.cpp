#include "display.h"
#include "logger.h"
#include <algorithm>
#include <cmath>
#include <iostream>

extern "C" {
#include <libavutil/frame.h>
}

namespace vp {

namespace {

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
    , initialized_(false) {
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

    SDL_SetWindowMinimumSize(window_, 320, 180);
    
    renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer_) {
        std::cerr << "Error: Could not create SDL renderer: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window_);
        window_ = nullptr;
        return false;
    }
    
    if (!createTexture(width, height)) {
        SDL_DestroyRenderer(renderer_);
        SDL_DestroyWindow(window_);
        renderer_ = nullptr;
        window_ = nullptr;
        return false;
    }
    
    initialized_ = true;
    should_quit_ = false;
    toggle_pause_requested_ = false;
    
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
    if (!texture_ || !data) {
        return false;
    }
    
    AVFrame* frame = (AVFrame*)data;
    
    int ret = SDL_UpdateYUVTexture(
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
        LOG_TRACE_EVENT("renderFrame: early return");
        return;
    }
    
    SDL_RenderClear(renderer_);
    
    AVFrame* frame = (AVFrame*)data;
    
    LOG_TRACE_EVENT("renderFrame: update YUV texture");
    
    int ret = SDL_UpdateYUVTexture(
        texture_,
        nullptr,
        frame->data[0], frame->linesize[0],
        frame->data[1], frame->linesize[1],
        frame->data[2], frame->linesize[2]
    );
    
    if (ret < 0) {
        std::cerr << "Error: Could not update YUV texture: " << SDL_GetError() << std::endl;
        return;
    }
    
    const SDL_Rect dst_rect = computeRenderRect(width_, height_, width, height);
    
    SDL_RenderCopy(renderer_, texture_, nullptr, &dst_rect);
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
    LOG_TRACE_EVENT("handleEvents: start");
    SDL_Event event;
    
    while (SDL_PollEvent(&event)) {
        LOG_TRACE_EVENT("Received event type: " << event.type);
        switch (event.type) {
            case SDL_QUIT:
                LOG_TRACE_EVENT("SDL_QUIT event received, setting should_quit_=true");
                should_quit_ = true;
                break;
                
            case SDL_KEYDOWN:
                LOG_TRACE_EVENT("SDL_KEYDOWN event, key: " << event.key.keysym.sym);
                switch (event.key.keysym.sym) {
                    case SDLK_ESCAPE:
                    case SDLK_q:
                        LOG_TRACE_EVENT("Exit key pressed, setting should_quit_=true");
                        should_quit_ = true;
                        break;
                    case SDLK_SPACE:
                        toggle_pause_requested_ = true;
                        break;
                    case SDLK_f:
                        toggleFullscreen();
                        break;
                    default:
                        break;
                }
                break;
                
            case SDL_WINDOWEVENT:
                LOG_TRACE_EVENT("SDL_WINDOWEVENT, window event: " << event.window.event);
                if (event.window.event == SDL_WINDOWEVENT_RESIZED ||
                    event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                    width_ = std::max(1, event.window.data1);
                    height_ = std::max(1, event.window.data2);
                }
                break;
                
            default:
                break;
        }
    }
    LOG_TRACE_EVENT("handleEvents: end, should_quit_=" << should_quit_);
}

bool Display::consumeTogglePauseRequest() {
    if (!toggle_pause_requested_) {
        return false;
    }
    toggle_pause_requested_ = false;
    return true;
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
}

} // namespace vp
