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
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "input/hotkey_manager.h"

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
    bool consumeToggleSubtitleRequest();
    bool consumeSetABRepeatStartRequest();
    bool consumeSetABRepeatEndRequest();
    bool consumeClearABRepeatRequest();
    bool consumeNextChapterRequest();
    bool consumePreviousChapterRequest();
    bool consumeNextItemRequest();
    bool consumePreviousItemRequest();
    void setOverlayState(double position, double duration, float volume, bool paused);
    void setSubtitleText(const std::string& text);
    void setHotkeyManager(const input::HotkeyManager& hotkey_manager);
    void setPreferredRendererDriver(const std::string& driver_name);
    std::string currentRendererDriver() const;
    bool isUsingRendererDriver(const std::string& driver_name) const;
    
    void toggleFullscreen();
    int getWidth() const { return width_.load(); }
    int getHeight() const { return height_.load(); }

private:
    struct PendingVideoFrame {
        int width{0};
        int height{0};
        int y_pitch{0};
        int u_pitch{0};
        int v_pitch{0};
        std::vector<uint8_t> y_plane;
        std::vector<uint8_t> u_plane;
        std::vector<uint8_t> v_plane;
        bool valid{false};
    };

    struct ControlLayout {
        SDL_Rect panel;
        SDL_Rect progress_track;
        SDL_Rect volume_track;
    };

    bool createRenderer();
    bool createTexture(int width, int height);
    bool ensureTextureForFrame(int width, int height);
    bool ensureRenderResources(int frame_width, int frame_height);
    bool updateTexture(const PendingVideoFrame& frame);
    bool copyFrameData(const AVFrame& frame, PendingVideoFrame& out);
    void renderLoop();
    void drawControls(int window_width, int window_height);
    void drawSubtitleOverlay(int window_width, int window_height);
    ControlLayout computeControlLayout(int window_width, int window_height) const;
    void updateSeekFromMouse(int mouse_x, bool commit);
    void updateVolumeFromMouse(int mouse_x);
    
    SDL_Window* window_;
    SDL_Renderer* renderer_;
    SDL_Texture* texture_;
    
    std::atomic<int> width_;
    std::atomic<int> height_;
    std::atomic<bool> should_quit_;
    std::atomic<bool> toggle_pause_requested_;
    std::atomic<bool> fullscreen_;
    std::atomic<bool> minimized_;
    std::atomic<bool> renderer_reset_requested_;
    std::atomic<bool> texture_reset_requested_;
    std::atomic<bool> fullscreen_toggle_requested_;
    bool initialized_;
    std::mutex sdl_mutex_;
    int texture_width_;
    int texture_height_;
    std::thread render_thread_;
    std::atomic<bool> render_running_{false};
    std::atomic<bool> render_initialized_{false};
    std::atomic<bool> render_init_success_{false};
    std::mutex render_queue_mutex_;
    std::condition_variable render_queue_cv_;
    PendingVideoFrame pending_frame_;
    bool pending_frame_ready_{false};

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
    bool subtitle_toggle_requested_;
    bool ab_repeat_start_requested_;
    bool ab_repeat_end_requested_;
    bool ab_repeat_clear_requested_;
    bool next_chapter_requested_;
    bool previous_chapter_requested_;
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
    std::mutex subtitle_mutex_;
    std::string subtitle_text_;
    input::HotkeyManager hotkey_manager_;
    std::string preferred_renderer_driver_;
    mutable std::mutex renderer_info_mutex_;
    std::string active_renderer_driver_;
};

} // namespace vp
