#include "display.h"
#include <iostream>

namespace vp {

Display::Display()
    : window_(nullptr)
    , renderer_(nullptr)
    , texture_(nullptr)
    , width_(0)
    , height_(0)
    , should_quit_(false)
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
    
    width_ = width;
    height_ = height;
    
    window_ = SDL_CreateWindow(
        title.c_str(),
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        width,
        height,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );
    
    if (!window_) {
        std::cerr << "Error: Could not create SDL window: " << SDL_GetError() << std::endl;
        return false;
    }
    
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
    
    std::cout << "Display initialized: " << width << "x" << height << std::endl;
    
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
        return;
    }
    
    SDL_RenderClear(renderer_);
    
    int ret = SDL_UpdateYUVTexture(
        texture_,
        nullptr,
        data, width,
        data + width * height, width / 2,
        data + width * height * 5 / 4, width / 2
    );
    
    if (ret < 0) {
        std::cerr << "Error: Could not update YUV texture: " << SDL_GetError() << std::endl;
        return;
    }
    
    SDL_Rect dst_rect;
    dst_rect.x = 0;
    dst_rect.y = 0;
    dst_rect.w = width_;
    dst_rect.h = height_;
    
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
    SDL_Event event;
    
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                should_quit_ = true;
                break;
                
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_ESCAPE:
                    case SDLK_q:
                        should_quit_ = true;
                        break;
                    case SDLK_SPACE:
                        break;
                    case SDLK_f:
                        toggleFullscreen();
                        break;
                    default:
                        break;
                }
                break;
                
            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                    width_ = event.window.data1;
                    height_ = event.window.data2;
                }
                break;
                
            default:
                break;
        }
    }
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
