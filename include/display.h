#pragma once

extern "C" {
#include <libavutil/frame.h>
#include <libavutil/pixfmt.h>
}

#if __has_include(<SDL2/SDL.h>)
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#elif __has_include(<SDL.h>)
#include <SDL.h>
#include <SDL_syswm.h>
#else
#error "SDL2 headers not found"
#endif

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "input/hotkey_manager.h"

namespace vp {

class Display {
public:
    struct FrameCopyStats {
        uint64_t frames{0};
        uint64_t bytes{0};
        uint64_t time_us_total{0};
        uint64_t time_us_max{0};
    };

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
    bool consumeScreenshotRequest();
    bool consumeStepFrameBackwardRequest();
    bool consumeStepFrameForwardRequest();
    bool consumeSubtitleDelayChangeRequest(double& delta_seconds);
    bool consumeAudioDelayChangeRequest(double& delta_seconds);
    bool consumeNextChapterRequest();
    bool consumePreviousChapterRequest();
    bool consumeNextItemRequest();
    bool consumePreviousItemRequest();
    bool consumeOpenFileRequest(std::string& path);
    void setOverlayState(double position, double duration, float volume, bool paused);
    void setSubtitleText(const std::string& text);
    void setHotkeyManager(const input::HotkeyManager& hotkey_manager);
    void setPreferredRendererDriver(const std::string& driver_name);
    std::string currentRendererDriver() const;
    bool isUsingRendererDriver(const std::string& driver_name) const;
    FrameCopyStats getFrameCopyStats() const;
    void resetFrameCopyStats();

    void toggleFullscreen();
    int getWidth() const { return width_.load(); }
    int getHeight() const { return height_.load(); }

private:
    struct AvFrameRefDeleter {
        void operator()(AVFrame* frame) const {
            if (frame) {
                av_frame_free(&frame);
            }
        }
    };

    struct PendingVideoFrame {
        using FrameRefPtr = std::unique_ptr<AVFrame, AvFrameRefDeleter>;

        int width{0};
        int height{0};
        AVPixelFormat format{AV_PIX_FMT_NONE};
        int y_pitch{0};
        int u_pitch{0};
        int v_pitch{0};
        std::vector<uint8_t> y_plane;
        std::vector<uint8_t> u_plane;
        std::vector<uint8_t> v_plane;
        FrameRefPtr frame_ref{};
        bool direct_reference{false};
        bool valid{false};

        PendingVideoFrame() = default;
        PendingVideoFrame(const PendingVideoFrame&) = delete;
        PendingVideoFrame& operator=(const PendingVideoFrame&) = delete;
        PendingVideoFrame(PendingVideoFrame&&) noexcept = default;
        PendingVideoFrame& operator=(PendingVideoFrame&&) noexcept = default;
    };

    struct ControlLayout {
        SDL_Rect panel;
        SDL_Rect progress_track;
        SDL_Rect volume_track;
    };

    bool createRenderer();
    bool createTexture(int width, int height, AVPixelFormat format);
    bool ensureTextureForFrame(int width, int height, AVPixelFormat format);
    bool ensureRenderResources(const PendingVideoFrame& frame);
    bool updateTexture(const PendingVideoFrame& frame);
    bool copyFrameData(const AVFrame& frame, PendingVideoFrame& out);
    void renderLoop();
    void drawControls(int window_width, int window_height);
    void drawSubtitleOverlay(int window_width, int window_height);
    ControlLayout computeControlLayout(int window_width, int window_height) const;
    void updateSeekFromMouse(int mouse_x, bool commit);
    void updateVolumeFromMouse(int mouse_x);

    SDL_Window* window_{nullptr};
    SDL_Renderer* renderer_{nullptr};
    SDL_Texture* texture_{nullptr};

    std::atomic<int> width_{0};
    std::atomic<int> height_{0};
    std::atomic<bool> should_quit_{false};
    std::atomic<bool> toggle_pause_requested_{false};
    std::atomic<bool> fullscreen_{false};
    std::atomic<bool> minimized_{false};
    std::atomic<bool> renderer_reset_requested_{false};
    std::atomic<bool> texture_reset_requested_{false};
    std::atomic<bool> fullscreen_toggle_requested_{false};
    std::atomic<int64_t> last_fullscreen_toggle_request_ms_{0};
    std::thread::id event_thread_id_{};
    std::atomic<bool> event_thread_guard_reported_{false};
    bool initialized_{false};
    std::mutex sdl_mutex_;
    int texture_width_{0};
    int texture_height_{0};
    Uint32 texture_format_{SDL_PIXELFORMAT_UNKNOWN};
    std::thread render_thread_;
    std::atomic<bool> render_running_{false};
    std::atomic<bool> render_initialized_{false};
    std::atomic<bool> render_init_success_{false};
    std::mutex render_queue_mutex_;
    std::condition_variable render_queue_cv_;
    PendingVideoFrame pending_frame_;
    bool pending_frame_ready_{false};

    std::mutex request_mutex_;
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
    bool open_file_requested_{false};
    std::string open_file_path_;
    float last_nonzero_volume_{1.0f};
    bool dragging_seek_{false};
    bool dragging_volume_{false};
    bool seek_preview_active_{false};
    double seek_preview_ratio_{0.0};

    std::atomic<double> overlay_position_{0.0};
    std::atomic<double> overlay_duration_{0.0};
    std::atomic<float> overlay_volume_{1.0f};
    std::atomic<bool> overlay_paused_{false};
    std::mutex subtitle_mutex_;
    std::string subtitle_text_;
    input::HotkeyManager hotkey_manager_{};
    std::string preferred_renderer_driver_;
    mutable std::mutex renderer_info_mutex_;
    std::string active_renderer_driver_;
    std::atomic<uint64_t> frame_copy_frames_{0};
    std::atomic<uint64_t> frame_copy_bytes_{0};
    std::atomic<uint64_t> frame_copy_time_us_total_{0};
    std::atomic<uint64_t> frame_copy_time_us_max_{0};
};

}  // namespace vp
