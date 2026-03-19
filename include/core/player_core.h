#pragma once

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/channel_layout.h>
}

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <limits>
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
}
struct SwrContext;
struct SwsContext;

namespace vp::core {

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

struct PlaybackInfo {
    double duration{0.0};
    double position{0.0};
    int video_width{0};
    int video_height{0};
    int audio_sample_rate{0};
    int audio_channels{0};
};

struct DiagnosticsSnapshot {
    bool audio_output_initialized{false};
    bool video_only_fallback{false};
    ClockSource clock_source{ClockSource::Audio};
    uint64_t demux_video_packets{0};
    uint64_t demux_audio_packets{0};
    uint64_t demux_push_retries{0};
    uint64_t demux_dropped_packets{0};
    uint64_t demux_ignored_packets{0};
    uint64_t demux_queue_drop_packets{0};
    uint64_t decode_video_ok{0};
    uint64_t decode_audio_ok{0};
    uint64_t video_packet_dequeue_count{0};
    uint64_t video_send_packet_ok{0};
    int video_send_packet_last_ret{std::numeric_limits<int>::min()};
    uint64_t decode_video_send_eagain{0};
    uint64_t decode_audio_send_eagain{0};
    uint64_t video_decoder_drain_signals{0};
    uint64_t audio_decoder_drain_signals{0};
    uint64_t video_native_output_frames{0};
    uint64_t video_copy_back_frames{0};
    uint64_t video_swscale_frames{0};
    uint64_t video_filter_blocked_native_frames{0};
    uint64_t audio_submitted_frames{0};
    uint64_t render_frames{0};
    uint64_t scheduler_video_decoded_frames{0};
    uint64_t scheduler_audio_decoded_frames{0};
    uint64_t scheduler_late_drops{0};
    uint64_t scheduler_wait_events{0};
    uint64_t scheduler_video_backpressure_events{0};
    uint64_t scheduler_audio_backpressure_events{0};
    uint64_t scheduler_video_backpressure_wait_ms{0};
    uint64_t scheduler_audio_backpressure_wait_ms{0};
    uint64_t scheduler_video_restart_attempts{0};
    uint64_t scheduler_audio_restart_attempts{0};
    uint64_t scheduler_render_restart_attempts{0};
    uint64_t scheduler_video_restart_limit_hits{0};
    uint64_t scheduler_audio_restart_limit_hits{0};
    uint64_t scheduler_render_restart_limit_hits{0};
    uint64_t video_copy_back_time_us_total{0};
    uint64_t video_swscale_time_us_total{0};
    uint64_t video_copy_back_time_us_max{0};
    uint64_t video_swscale_time_us_max{0};
    uint64_t display_copy_frames{0};
    uint64_t display_copy_bytes{0};
    uint64_t display_copy_time_us_total{0};
    uint64_t display_copy_time_us_max{0};
    size_t video_packet_queue_size{0};
    size_t audio_packet_queue_size{0};
    size_t video_frame_queue_size{0};
    size_t audio_frame_queue_size{0};
    size_t video_frame_queue_capacity{0};
    size_t audio_frame_queue_capacity{0};
    size_t video_frame_queue_peak_size{0};
    size_t audio_frame_queue_peak_size{0};
    uint64_t video_frame_queue_push_timeouts{0};
    uint64_t audio_frame_queue_push_timeouts{0};
};

class PlayerCore {
public:
    PlayerCore();
    ~PlayerCore();

    bool open(const std::string& filename);
    void close();

    void play();
    void pause();
    void stop();
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
    bool consumeLastScreenshotPath(std::string& path);
    bool stepFrameBackward();
    bool stepFrameForward();
    void pumpEvents();
    bool consumeQuitRequest();
    bool consumeNextItemRequest();
    bool consumePreviousItemRequest();

    PlaybackState getState() const;
    PlaybackInfo getInfo() const;
    DiagnosticsSnapshot getDiagnosticsSnapshot() const;

    void setVolume(float volume);
    float getVolume() const;

    void setPlaybackSpeed(double speed);
    double getPlaybackSpeed() const;
    void setAudioDelay(double delay_seconds);
    double getAudioDelay() const;
    void setSubtitleDelay(double delay_seconds);
    double getSubtitleDelay() const;
    void setPreferHardwareDecode(bool prefer_hardware_decode);
    bool preferHardwareDecode() const;
    decoder::DecoderBackend videoDecoderBackend() const;
    std::string videoRendererBackendName() const;
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

    void onStateChanged(StateCallback callback);
    void onPositionChanged(PositionCallback callback);
    void onError(ErrorCallback callback);
    void onFrameRendered(FrameCallback callback);

private:
    struct AvPacketDeleter {
        void operator()(AVPacket* packet) const noexcept {
            if (packet) {
                av_packet_free(&packet);
            }
        }
    };

    using PacketPtr = std::unique_ptr<AVPacket, AvPacketDeleter>;
    using PacketQueue = ThreadSafeQueue<PacketPtr>;

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
    void configureFrameQueues(const MediaInfo& info);
    void startDemuxThread();
    void stopDemuxThread();
    void startAudioConsumer();
    void stopAudioConsumer();
    void flushPipelines();
    void requestDeferredStop();
    void serviceDeferredStop();
    void reapCompletedWorkers();

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
    std::atomic<bool> deferred_stop_pending_{false};

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
    std::chrono::steady_clock::time_point last_position_emit_tp_{};
    std::atomic<uint64_t> demux_video_packets_{0};
    std::atomic<uint64_t> demux_audio_packets_{0};
    std::atomic<uint64_t> demux_push_retries_{0};
    std::atomic<uint64_t> demux_dropped_packets_{0};
    std::atomic<uint64_t> demux_ignored_packets_{0};
    std::atomic<uint64_t> demux_queue_drop_packets_{0};
    std::atomic<uint64_t> decode_video_ok_{0};
    std::atomic<uint64_t> decode_audio_ok_{0};
    std::atomic<uint64_t> video_packet_dequeue_count_{0};
    std::atomic<uint64_t> video_send_packet_ok_{0};
    std::atomic<int> video_send_packet_last_ret_{std::numeric_limits<int>::min()};
    std::atomic<uint64_t> decode_video_send_eagain_{0};
    std::atomic<uint64_t> decode_audio_send_eagain_{0};
    std::atomic<uint64_t> video_decoder_drain_signals_{0};
    std::atomic<uint64_t> audio_decoder_drain_signals_{0};
    std::atomic<uint64_t> video_native_output_frames_{0};
    std::atomic<uint64_t> video_copy_back_frames_{0};
    std::atomic<uint64_t> video_swscale_frames_{0};
    std::atomic<uint64_t> video_filter_blocked_native_frames_{0};
    std::atomic<uint64_t> audio_submitted_frames_{0};
    std::atomic<uint64_t> render_frames_{0};
    std::atomic<uint64_t> video_copy_back_time_us_total_{0};
    std::atomic<uint64_t> video_swscale_time_us_total_{0};
    std::atomic<uint64_t> video_copy_back_time_us_max_{0};
    std::atomic<uint64_t> video_swscale_time_us_max_{0};
    std::atomic<int64_t> last_diag_log_ms_{0};
    std::atomic<int64_t> last_video_decode_issue_log_ms_{0};
    std::atomic<bool> first_video_send_started_logged_{false};
    std::atomic<bool> first_video_send_completed_logged_{false};
    std::mutex video_codec_mutex_;
    std::mutex audio_codec_mutex_;
    bool video_decoder_draining_{false};
    bool audio_decoder_draining_{false};

    mutable std::mutex subtitle_mutex_;
    std::vector<subtitle::SubtitleItem> subtitle_items_;
    std::string subtitle_source_path_;
    std::vector<int> subtitle_active_indices_;
    bool subtitle_enabled_{true};
    input::HotkeyManager hotkey_manager_{};
    mutable std::mutex screenshot_mutex_;
    mutable std::mutex rendered_frame_mutex_;
    std::string last_screenshot_path_;
    bool screenshot_path_pending_{false};
    AVFrame* last_rendered_frame_{nullptr};
};

}  // namespace vp::core


