#pragma once

extern "C" {
#include <libavutil/frame.h>
}

#include <SDL2/SDL.h>
#include <string>
#include <memory>

namespace vp {

class Display {
public:
    Display();
    ~Display();
    
    bool init(int width, int height, const std::string& title);
    void close();
    
    void renderFrame(const uint8_t* data, int width, int height);
    void present();
    void clear();
    
    void handleEvents();
    bool shouldQuit() const { return should_quit_; }
    bool consumeTogglePauseRequest();
    
    void toggleFullscreen();
    int getWidth() const { return width_; }
    int getHeight() const { return height_; }

private:
    bool createTexture(int width, int height);
    bool updateTexture(const uint8_t* data, int width, int height);
    
    SDL_Window* window_;
    SDL_Renderer* renderer_;
    SDL_Texture* texture_;
    
    int width_;
    int height_;
    bool should_quit_;
    bool toggle_pause_requested_;
    bool fullscreen_;
    bool initialized_;
};

} // namespace vp
