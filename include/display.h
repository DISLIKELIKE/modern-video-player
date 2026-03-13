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

/// SDL 显示与交互封装；内部维护独立渲染线程并向播放核心暴露 UI 请求。
class Display {
public:
    /// 构造未初始化的显示模块。
    Display();
    /// 关闭窗口、渲染线程和 SDL 资源。
    ~Display();
    
    /// 初始化窗口与渲染线程。
    /// @param width 视频源宽度或期望的初始窗口宽度基准。
    /// @param height 视频源高度或期望的初始窗口高度基准。
    /// @param title 窗口标题文本。
    /// @return SDL 窗口与渲染线程都初始化成功时返回 true。
    bool init(int width, int height, const std::string& title);
    /// 停止渲染线程并释放窗口、渲染器、纹理。
    void close();
    
    /// 向渲染线程提交一帧视频数据。
    /// @param data 当前实现约定为 `AVFrame*` 的字节指针视图。
    /// @param width 调用方传入的帧宽度；当前主要用于接口兼容。
    /// @param height 调用方传入的帧高度；当前主要用于接口兼容。
    /// @note 调用线程只负责排队和复制，不直接执行 SDL 呈现。
    void renderFrame(const uint8_t* data, int width, int height);
    /// 请求立即呈现；当前实现由渲染线程统一驱动，通常为 no-op。
    void present();
    /// 清空待显示帧和最近一次缓存帧。
    void clear();
    
    /// 轮询 SDL 事件并转换为播放器控制请求。
    void handleEvents();
    /// 返回窗口是否已收到退出信号。
    bool shouldQuit() const { return should_quit_.load(); }
    /// 消费一次暂停/继续请求。
    bool consumeTogglePauseRequest();
    /// 消费一次按比例 seek 请求，范围为 [0, 1]。
    /// @param normalized_position 成功时写出标准化目标位置。
    /// @return 有新的 seek 请求时返回 true。
    bool consumeSeekRequest(double& normalized_position);
    /// 消费一次按秒增量 seek 请求。
    bool consumeSeekDeltaRequest(double& delta_seconds);
    /// 消费一次音量调整请求。
    bool consumeVolumeChangeRequest(float& volume);
    /// 消费一次倍速增量调整请求。
    bool consumeSpeedChangeRequest(double& speed_delta);
    /// 消费一次“恢复默认倍速”请求。
    bool consumeResetSpeedRequest();
    /// 消费一次字幕开关请求。
    bool consumeToggleSubtitleRequest();
    /// 消费一次 A-B 起点设置请求。
    bool consumeSetABRepeatStartRequest();
    /// 消费一次 A-B 终点设置请求。
    bool consumeSetABRepeatEndRequest();
    /// 消费一次清除 A-B 循环请求。
    bool consumeClearABRepeatRequest();
    /// 消费一次截图请求。
    bool consumeScreenshotRequest();
    /// 消费一次向后逐帧请求。
    bool consumeStepFrameBackwardRequest();
    /// 消费一次向前逐帧请求。
    bool consumeStepFrameForwardRequest();
    /// 消费一次字幕延迟调整请求。
    bool consumeSubtitleDelayChangeRequest(double& delta_seconds);
    /// 消费一次音频延迟调整请求。
    bool consumeAudioDelayChangeRequest(double& delta_seconds);
    /// 消费一次下一章节请求。
    bool consumeNextChapterRequest();
    /// 消费一次上一章节请求。
    bool consumePreviousChapterRequest();
    /// 消费一次下一播放项请求。
    bool consumeNextItemRequest();
    /// 消费一次上一播放项请求。
    bool consumePreviousItemRequest();
    /// 更新底部叠加层状态，用于绘制进度、音量和暂停标记。
    /// @param position 当前播放位置，单位秒。
    /// @param duration 当前媒体总时长，单位秒。
    /// @param volume 当前音量，范围通常为 [0, 1]。
    /// @param paused 当前是否处于暂停态。
    void setOverlayState(double position, double duration, float volume, bool paused);
    /// 更新当前应显示的字幕文本。
    void setSubtitleText(const std::string& text);
    /// 替换热键绑定配置。
    void setHotkeyManager(const input::HotkeyManager& hotkey_manager);
    /// 指定期望的 SDL 渲染驱动名称。
    void setPreferredRendererDriver(const std::string& driver_name);
    /// 返回当前实际使用的 SDL 渲染驱动名称。
    std::string currentRendererDriver() const;
    /// 判断当前是否使用指定 SDL 渲染驱动。
    bool isUsingRendererDriver(const std::string& driver_name) const;
    
    /// 异步切换全屏状态，真正切换发生在渲染线程。
    void toggleFullscreen();
    /// 返回当前窗口宽度。
    int getWidth() const { return width_.load(); }
    /// 返回当前窗口高度。
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
    bool screenshot_requested_;
    bool step_frame_backward_requested_;
    bool step_frame_forward_requested_;
    bool subtitle_delay_change_requested_;
    double subtitle_delay_delta_seconds_;
    bool audio_delay_change_requested_;
    double audio_delay_delta_seconds_;
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
