#pragma once

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/channel_layout.h>
}

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "core/clock.h"
#include "core/frame.h"
#include "core/frame_queue.h"
#include "core/scheduler.h"
#include "decoder/decoder_capability.h"
#include "demuxer.h"
#include "filters/filter_pipeline.h"
#include "input/hotkey_manager.h"
#include "render/video_renderer.h"
#include "subtitle/subtitle_parser.h"
#include "thread_safe_queue.h"

namespace vp {
class AudioPlayer;
}  // namespace vp
struct SwrContext;
struct SwsContext;

namespace vp::core {

/// 播放状态枚举。
enum class PlaybackState {
    Stopped,
    Playing,
    Paused
};

enum class ErrorCode {
    None = 0,
    FileNotFound,
    UnsupportedFormat,
    DecoderInitFailed,
    DisplayInitFailed,
    AudioInitFailed,
    SeekFailed,
    FilterError
};

/// 播放信息快照。
struct PlaybackInfo {
    // 媒体总时长（秒），未知时为 0。
    double duration{0.0};
    // 当前播放位置（秒）。
    double position{0.0};
    // 当前视频分辨率，纯音频时为 0。
    int video_width{0};
    int video_height{0};
    // 当前音频参数，纯视频时为 0。
    int audio_sample_rate{0};
    int audio_channels{0};
};

/// 播放链路诊断统计快照。
struct DiagnosticsSnapshot {
    uint64_t demux_video_packets{0};
    uint64_t demux_audio_packets{0};
    uint64_t demux_push_retries{0};
    uint64_t demux_dropped_packets{0};
    uint64_t decode_video_ok{0};
    uint64_t decode_audio_ok{0};
    uint64_t audio_submitted_frames{0};
    uint64_t render_frames{0};
    uint64_t scheduler_video_decoded_frames{0};
    uint64_t scheduler_audio_decoded_frames{0};
    uint64_t scheduler_late_drops{0};
    size_t video_packet_queue_size{0};
    size_t audio_packet_queue_size{0};
    size_t video_frame_queue_size{0};
    size_t audio_frame_queue_size{0};
};

/// 播放核心；负责组织解复用、解码、调度、音频与渲染全链路。
class PlayerCore {
public:
    PlayerCore();
    ~PlayerCore();

    /// 打开媒体并初始化渲染/解码/队列资源。
    /// @param filename 媒体文件路径或输入 URI。
    /// @return 全链路初始化成功时返回 true；失败时可结合错误回调定位原因。
    bool open(const std::string& filename);
    // 停止播放并释放全部运行时资源。
    void close();

    // 播放控制：play/pause/stop 支持在 UI 线程直接调用。
    void play();
    void pause();
    void stop();
    /// 按秒 seek 到目标位置。
    /// @param timestamp 目标时间点，单位秒。
    /// @note 内部会执行队列清空、解码器 flush 和时钟重同步。
    void seek(double timestamp);
    bool seekToNextChapter();
    bool seekToPreviousChapter();
    size_t chapterCount() const;
    bool setABRepeatStart();
    bool setABRepeatEnd();
    void clearABRepeat();
    bool isABRepeatEnabled() const;
    double abRepeatStart() const;
    double abRepeatEnd() const;
    bool requestScreenshot();
    /// 取走最近一次截图路径。
    /// @param path 成功时写出截图文件路径。
    /// @return 有新的截图结果可消费时返回 true。
    bool consumeLastScreenshotPath(std::string& path);
    bool stepFrameBackward();
    bool stepFrameForward();
    // 处理渲染器事件（热键、拖动进度、窗口关闭等）。
    void pumpEvents();
    bool consumeQuitRequest();
    bool consumeNextItemRequest();
    bool consumePreviousItemRequest();

    PlaybackState getState() const;
    PlaybackInfo getInfo() const;
    // 读取关键诊断计数，便于定位“读包/解码/渲染”瓶颈。
    DiagnosticsSnapshot getDiagnosticsSnapshot() const;

    // 音量范围 [0,1]。
    void setVolume(float volume);
    float getVolume() const;

    // 倍速范围 [0.5,2.0]，具体能力受解码与音频链路约束。
    void setPlaybackSpeed(double speed);
    double getPlaybackSpeed() const;
    // 音频/字幕延迟单位均为秒，正值表示“更晚播放/显示”。
    void setAudioDelay(double delay_seconds);
    double getAudioDelay() const;
    void setSubtitleDelay(double delay_seconds);
    double getSubtitleDelay() const;
    void setPreferHardwareDecode(bool prefer_hardware_decode);
    bool preferHardwareDecode() const;
    decoder::DecoderBackend videoDecoderBackend() const;
    std::string videoRendererBackendName() const;
    /// 设置外挂字幕集合并记录来源路径。
    /// @param subtitles 已解析的字幕条目；会移动到内部存储。
    /// @param source_path 字幕来源路径，用于状态展示和诊断日志。
    void setExternalSubtitles(std::vector<subtitle::SubtitleItem> subtitles, const std::string& source_path);
    void clearExternalSubtitles();
    bool hasExternalSubtitles() const;
    void setSubtitleEnabled(bool enabled);
    bool isSubtitleEnabled() const;
    bool toggleSubtitleEnabled();
    void setHotkeyManager(const input::HotkeyManager& hotkey_manager);
    const input::HotkeyManager& hotkeyManager() const;

    using StateCallback = std::function<void(PlaybackState)>;
    using PositionCallback = std::function<void(double)>;
    using ErrorCallback = std::function<void(ErrorCode, const std::string&)>;
    using FrameCallback = std::function<void()>;

    /// 注册状态变更回调。
    /// @param callback 回调在内部线程触发，实现方需避免长时间阻塞。
    void onStateChanged(StateCallback callback);
    /// 注册位置变更回调。
    /// @param callback 回调在内部线程触发，适合做轻量 UI 状态同步。
    void onPositionChanged(PositionCallback callback);
    /// 注册错误回调。
    /// @param callback 回调会带出错误码与说明文本。
    void onError(ErrorCallback callback);
    /// 注册帧渲染完成回调。
    /// @param callback 回调在渲染链路内触发，不能执行耗时操作。
    void onFrameRendered(FrameCallback callback);

private:
    using PacketQueue = ThreadSafeQueue<AVPacket*>;

    bool initDecoders();
    void releaseDecoders();
    bool tryConfigureD3D11HardwareDecode(const AVCodec* codec, AVCodecContext* codec_ctx);
    static AVPixelFormat selectVideoPixelFormat(AVCodecContext* ctx, const AVPixelFormat* pix_fmts);
    bool prepareVideoOutputFrame(AVFrame* decoded_frame, VideoFrame& out);
    bool ensureVideoScaler(const AVFrame* src_frame);
    bool convertVideoFrameToYuv420(const AVFrame* src_frame, AVFrame* dst_frame);
    void releaseVideoScaler();
    bool ensureAudioResampler(const AVFrame* frame);
    void releaseAudioResampler();
    void startDemuxThread();
    void stopDemuxThread();
    void startAudioConsumer();
    void stopAudioConsumer();
    void flushPipelines();

    bool decodeVideoFrame(VideoFrame& out);
    bool decodeAudioFrame(AudioFrame& out);
    void renderFrame(VideoFrame&& frame);
    void onRenderIdle();
    void rebuildChapterPoints();
    void handleABRepeatLoop();
    bool captureScreenshot(const VideoFrame& frame);
    bool captureScreenshotFrame(const AVFrame* frame);
    bool captureScreenshotFromCachedFrame();
    bool stepFrame(int direction);
    bool renderPausedFrameAtOrAfter(double target_seconds);
    double estimateFrameStepSeconds() const;
    void updateLastRenderedFrame(const VideoFrame& frame);
    void clearLastRenderedFrame();
    void updateSubtitleOverlay(double position_seconds);

    void emitStateChanged(PlaybackState state);
    void emitPositionChanged(double position);
    void emitFrameRendered();
    void emitError(ErrorCode code, const std::string& message);
    void resetDiagnostics();
    void maybeLogDiagnostics(const char* source_tag);

    std::unique_ptr<Demuxer> demuxer_;
    render::VideoRendererPtr video_renderer_;
    std::unique_ptr<AudioPlayer> audio_player_;

    Scheduler scheduler_;
    Clock clock_;
    VideoFrameQueue video_queue_{16};
    AudioFrameQueue audio_queue_{32};
    std::unique_ptr<PacketQueue> video_packet_queue_;
    std::unique_ptr<PacketQueue> audio_packet_queue_;

    std::thread demux_thread_;
    std::thread audio_consumer_thread_;
    std::atomic<bool> demux_running_{false};
    std::atomic<bool> audio_consumer_running_{false};

    AVCodecContext* video_codec_ctx_{nullptr};
    AVCodecContext* audio_codec_ctx_{nullptr};
    AVBufferRef* video_hw_device_ctx_{nullptr};
    AVPixelFormat video_hw_pixel_fmt_{AV_PIX_FMT_NONE};
    decoder::DecoderBackend video_decoder_backend_{decoder::DecoderBackend::Software};
    SwsContext* video_sws_ctx_{nullptr};
    int video_sws_src_width_{0};
    int video_sws_src_height_{0};
    AVPixelFormat video_sws_src_fmt_{AV_PIX_FMT_NONE};
    SwrContext* audio_swr_ctx_{nullptr};
    AVChannelLayout swr_in_layout_{};
    AVChannelLayout swr_out_layout_{};
    AVSampleFormat swr_in_sample_fmt_{AV_SAMPLE_FMT_NONE};
    AVSampleFormat swr_out_sample_fmt_{AV_SAMPLE_FMT_NONE};
    int swr_in_sample_rate_{0};
    int swr_out_sample_rate_{0};
    AVRational video_time_base_{0, 1};
    AVRational audio_time_base_{0, 1};

    std::atomic<PlaybackState> state_{PlaybackState::Stopped};
    std::atomic<double> position_{0.0};
    std::atomic<float> volume_{1.0f};
    std::atomic<double> speed_{1.0};
    std::atomic<double> audio_delay_seconds_{0.0};
    std::atomic<double> subtitle_delay_seconds_{0.0};
    std::atomic<bool> prefer_hardware_decode_{true};
    std::atomic<bool> opened_{false};
    std::atomic<bool> quit_requested_{false};
    std::atomic<bool> next_item_requested_{false};
    std::atomic<bool> previous_item_requested_{false};
    std::vector<double> chapter_points_;
    std::atomic<bool> ab_repeat_enabled_{false};
    std::atomic<double> ab_repeat_start_{-1.0};
    std::atomic<double> ab_repeat_end_{-1.0};
    std::atomic<int64_t> ab_repeat_last_loop_ms_{0};
    std::atomic<bool> screenshot_requested_{false};
    std::atomic<double> last_video_frame_duration_{0.0};
    filters::FilterPipeline filter_pipeline_;

    mutable std::mutex callback_mutex_;
    std::vector<StateCallback> state_callbacks_;
    std::vector<PositionCallback> position_callbacks_;
    std::vector<ErrorCallback> error_callbacks_;
    std::vector<FrameCallback> frame_callbacks_;
    std::chrono::steady_clock::time_point last_position_emit_tp_;
    std::atomic<uint64_t> demux_video_packets_{0};
    std::atomic<uint64_t> demux_audio_packets_{0};
    std::atomic<uint64_t> demux_push_retries_{0};
    std::atomic<uint64_t> demux_dropped_packets_{0};
    std::atomic<uint64_t> decode_video_ok_{0};
    std::atomic<uint64_t> decode_audio_ok_{0};
    std::atomic<uint64_t> audio_submitted_frames_{0};
    std::atomic<uint64_t> render_frames_{0};
    std::atomic<int64_t> last_diag_log_ms_{0};
    std::mutex video_codec_mutex_;
    std::mutex audio_codec_mutex_;

    mutable std::mutex subtitle_mutex_;
    std::vector<subtitle::SubtitleItem> subtitle_items_;
    std::string subtitle_source_path_;
    std::string subtitle_active_text_;
    int subtitle_active_index_{-1};
    bool subtitle_enabled_{true};
    input::HotkeyManager hotkey_manager_{};
    mutable std::mutex screenshot_mutex_;
    mutable std::mutex rendered_frame_mutex_;
    std::string last_screenshot_path_;
    bool screenshot_path_pending_{false};
    AVFrame* last_rendered_frame_{nullptr};
};

}  // namespace vp::core
