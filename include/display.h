#pragma once

extern "C" {
#include <libavutil/frame.h>
}

#if __has_include(<SDL2/SDL.h>)
#include <SDL2/SDL.h>
#elif __has_include(<SDL.h>)
#include <SDL.h>
#else
#error "SDL2 headers not found"
#endif
#include <atomic>
#include <mutex>
#include <string>

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
    bool shouldQuit() const { return should_quit_.load(); }
    bool consumeTogglePauseRequest();
    bool consumeSeekRequest(double& normalized_position);
    bool consumeSeekDeltaRequest(double& delta_seconds);
    bool consumeVolumeChangeRequest(float& volume);
    bool consumeSpeedChangeRequest(double& speed_delta);
    bool consumeResetSpeedRequest();
    bool consumeNextItemRequest();
    bool consumePreviousItemRequest();
    void setOverlayState(double position, double duration, float volume, bool paused);
    
    void toggleFullscreen();
    int getWidth() const { return width_; }
    int getHeight() const { return height_; }

private:
    struct ControlLayout {
        SDL_Rect panel;
        SDL_Rect progress_track;
        SDL_Rect volume_track;
    };

    bool createTexture(int width, int height);
    bool updateTexture(const uint8_t* data, int width, int height);
    void drawControls(int window_width, int window_height);
    ControlLayout computeControlLayout(int window_width, int window_height) const;
    void updateSeekFromMouse(int mouse_x, bool commit);
    void updateVolumeFromMouse(int mouse_x);
    
    SDL_Window* window_;
    SDL_Renderer* renderer_;
    SDL_Texture* texture_;
    
    int width_;
    int height_;
    std::atomic<bool> should_quit_;
    std::atomic<bool> toggle_pause_requested_;
    bool fullscreen_;
    bool initialized_;

    std::mutex request_mutex_;
    bool seek_requested_;
    double seek_ratio_;
    bool seek_delta_requested_;
    double seek_delta_seconds_;
    bool volume_change_requested_;
    float requested_volume_;
    bool speed_change_requested_;
    double speed_delta_;
    bool speed_reset_requested_;
    bool next_item_requested_;
    bool previous_item_requested_;
    float last_nonzero_volume_;
    bool dragging_seek_;
    bool dragging_volume_;
    bool seek_preview_active_;
    double seek_preview_ratio_;

    std::atomic<double> overlay_position_;
    std::atomic<double> overlay_duration_;
    std::atomic<float> overlay_volume_;
    std::atomic<bool> overlay_paused_;
};

} // namespace vp
