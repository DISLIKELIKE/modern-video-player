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
#include "core/playback_strategy.h"
#include "core/frame.h"
#include "core/frame_queue.h"
#include "core/scheduler.h"
#include "core/worker_thread.h"

#include "decoder/decoder_capability.h"

#include "demuxer.h"

#include "filters/filter_pipeline.h"

#include "input/hotkey_manager.h"
#include "input/playback_input_source.h"

#include "render/render_overlay_sink.h"
#include "render/video_renderer.h"

#include "subtitle/embedded_subtitle_loader.h"

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



enum class EndedReason {

    None = 0,

    Eof

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

    bool audio_device_open_attempted{false};

    std::uint64_t audio_init_latency_ms{0};

    std::string audio_init_strategy{"not-run"};

    std::string audio_init_detail;

    std::string startup_platform{"Unknown"};

    std::string startup_renderer_capabilities{"none"};

    std::string startup_decoder_capabilities{"none"};
    std::string startup_renderer_compiled_set{"none"};
    std::string startup_renderer_runtime_set{"none"};
    std::string startup_decoder_compiled_set{"none"};
    std::string startup_decoder_runtime_set{"none"};

    std::string startup_renderer_candidates{"none"};

    std::string startup_decoder_candidates{"none"};

    std::string startup_renderer_plan_reason{"none"};

    std::string startup_decoder_plan_reason{"none"};

    std::string startup_selected_renderer{"None"};

    std::string startup_selected_decoder{"Unknown"};

    std::string startup_renderer_fallback_reason{"none"};

    std::string startup_decoder_fallback_reason{"none"};

    SchedulerClockPolicy scheduler_clock_policy{SchedulerClockPolicy::UseClockSource};

    SchedulerAudioMasterPolicy scheduler_audio_master_policy{SchedulerAudioMasterPolicy::Disabled};

    SchedulerEndedPolicy scheduler_ended_policy{SchedulerEndedPolicy::StopOutput};

    double scheduler_audio_buffered_seconds{0.0};

    TimelineSerial timeline_serial{kInvalidTimelineSerial};

    TimelineSerial pending_seek_serial{kInvalidTimelineSerial};

    EndedReason ended_reason{EndedReason::None};

    uint64_t video_packet_queue_generation{0};

    uint64_t audio_packet_queue_generation{0};

    uint64_t video_frame_queue_generation{0};

    uint64_t audio_frame_queue_generation{0};

    uint64_t demux_video_packets{0};

    uint64_t demux_audio_packets{0};

    uint64_t demux_push_retries{0};

    uint64_t demux_dropped_packets{0};

    uint64_t demux_ignored_packets{0};

    uint64_t demux_queue_drop_packets{0};

    uint64_t decode_video_ok{0};

    uint64_t decode_audio_ok{0};

    uint64_t stale_video_packets_dropped{0};

    uint64_t stale_audio_packets_dropped{0};

    uint64_t stale_video_frames_dropped{0};

    uint64_t stale_audio_frames_dropped{0};

    uint64_t stale_audio_submit_frames_dropped{0};

    uint64_t stale_render_frames_dropped{0};

    uint64_t stale_paused_render_frames_dropped{0};

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

    uint64_t scheduler_render_wait_ms{0};

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

    uint64_t runtime_failure_stop_requests{0};

    uint64_t runtime_failure_fail_sessions{0};

    uint64_t illegal_session_transitions{0};

    uint64_t illegal_run_transitions{0};

    uint64_t illegal_pipeline_transitions{0};

    uint64_t runtime_drop_total{0};

    std::string runtime_drop_summary{"none"};

    uint64_t video_copy_back_time_us_total{0};

    uint64_t video_swscale_time_us_total{0};

    uint64_t video_copy_back_time_us_max{0};

    uint64_t video_swscale_time_us_max{0};

    uint64_t display_copy_frames{0};

    uint64_t display_copy_bytes{0};

    uint64_t display_copy_time_us_total{0};

    uint64_t display_copy_time_us_max{0};

    bool renderer_opengl_native_interop_active{false};

    bool renderer_opengl_native_interop_startup_disabled{false};

    std::string renderer_opengl_native_interop_disable_rule{"none"};

    uint64_t renderer_opengl_native_interop_frames{0};

    uint64_t renderer_opengl_native_interop_disable_events{0};

    uint64_t renderer_opengl_present_wait_timeouts{0};

    std::string renderer_opengl_present_mode_requested{"auto"};

    std::string renderer_opengl_present_mode_active{"unknown"};

    bool renderer_opengl_hdr_bridge_requested{false};

    bool renderer_opengl_hdr_bridge_active{false};

    std::string renderer_opengl_hdr_bridge_mode{"auto"};

    std::string renderer_opengl_hdr_bridge_decision{"not-evaluated"};

    bool renderer_opengl_output_lut_configured{false};

    bool renderer_opengl_output_lut_active{false};

    int renderer_opengl_output_lut_size{0};

    std::string renderer_opengl_output_lut_path;

    std::string renderer_opengl_output_lut_error;

    std::string renderer_opengl_output_lut_source{"none"};

    uint64_t renderer_opengl_output_lut_reload_count{0};

    int renderer_opengl_output_display_index{-1};

    std::string renderer_opengl_output_display_name{"unknown"};

    std::string renderer_opengl_output_display_device_name{"unknown"};

    bool renderer_opengl_output_icc_profile_available{false};

    std::string renderer_opengl_output_icc_profile_source{"none"};

    std::string renderer_opengl_output_icc_profile_path;

    std::string renderer_opengl_output_icc_profile_description{"unknown"};

    std::string renderer_opengl_output_binding_error;

    bool renderer_d3d11_hdr_present_requested{false};

    bool renderer_d3d11_hdr_present_active{false};

    bool renderer_d3d11_hdr_content_detected{false};

    std::string renderer_d3d11_hdr_present_mode{"auto"};

    std::string renderer_d3d11_hdr_present_decision{"not-evaluated"};

    std::string renderer_d3d11_hdr_swapchain_format{"unknown"};

    std::string renderer_d3d11_hdr_output_color_space{"unknown"};

    int renderer_d3d11_hdr_output_display_index{-1};

    std::string renderer_d3d11_hdr_output_display_name{"unknown"};

    std::string renderer_d3d11_hdr_output_device_name{"unknown"};

    bool renderer_d3d11_hdr_output_advanced_color_active{false};

    bool renderer_d3d11_hdr_output_hdr_active{false};

    bool renderer_d3d11_hdr_metadata_available{false};

    bool renderer_d3d11_hdr_metadata_applied{false};

    std::string renderer_d3d11_hdr_metadata_source{"none"};

    uint64_t renderer_d3d11_hdr_state_reload_count{0};

    uint64_t renderer_d3d11_present_count{0};

    uint64_t renderer_d3d11_present_failures{0};

    uint64_t renderer_d3d11_present_time_us_total{0};

    uint64_t renderer_d3d11_present_time_us_max{0};

    std::string renderer_d3d11_hdr_error;

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

    bool consumeOpenFileRequest(std::string& path);



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

    void setPreferredRenderer(render::VideoRendererType renderer_type);

    render::VideoRendererType preferredRenderer() const;

    decoder::DecoderBackend videoDecoderBackend() const;

    std::string videoRendererBackendName() const;

    void setExternalSubtitles(std::vector<subtitle::SubtitleItem> subtitles, const std::string& source_path);

    void clearExternalSubtitles();

    bool hasExternalSubtitles() const;

    std::vector<subtitle::EmbeddedSubtitleTrackInfo> embeddedSubtitleTracks() const;

    int selectedEmbeddedSubtitleStreamIndex() const;

    bool selectEmbeddedSubtitleStream(int stream_index);

    bool cycleEmbeddedSubtitleTrack(int direction);

    void setPreferredEmbeddedSubtitleStreamIndex(int stream_index);

    int preferredEmbeddedSubtitleStreamIndex() const;

    void setEmbeddedSubtitleSelectionPolicy(const subtitle::EmbeddedSubtitleSelectionPolicy& policy);

    subtitle::EmbeddedSubtitleSelectionPolicy embeddedSubtitleSelectionPolicy() const;

    std::string activeSubtitleSourcePath() const;

    size_t activeSubtitleItemCount() const;

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

    enum class FailureRecoveryPolicy {

        EmitOnly,

        StopPlayback,

        FailSession

    };

    enum class SessionState {

        Closed,

        Opening,

        Ready,

        Closing,

        Failed

    };



    enum class RunState {

        Stopped,

        Starting,

        Running,

        Pausing,

        Paused,

        Stopping,

        Ended

    };



    enum class PipelinePhase {

        Idle,

        Normal,

        Seeking,

        Draining,

        Flushing

    };



    struct CoreStateSnapshot {

        SessionState session_state{SessionState::Closed};

        RunState run_state{RunState::Stopped};

        PipelinePhase pipeline_phase{PipelinePhase::Idle};

        bool eof_reached{false};

        bool pending_seek{false};

        bool deferred_stop_pending{false};

        TimelineSerial timeline_serial{kInvalidTimelineSerial};

        TimelineSerial pending_seek_serial{kInvalidTimelineSerial};

        EndedReason ended_reason{EndedReason::None};

    };

    struct DeferredFailSessionState {

        bool pending{false};

        ErrorCode code{ErrorCode::None};

        std::string message;

        std::string reason{"unspecified"};

    };



    struct AvPacketDeleter {

        void operator()(AVPacket* packet) const noexcept {

            if (packet) {

                av_packet_free(&packet);

            }

        }

    };



    using PacketPtr = std::unique_ptr<AVPacket, AvPacketDeleter>;

    struct DemuxPacket {

        PacketPtr packet;

        TimelineSerial serial{kInvalidTimelineSerial};

    };

    using PacketQueue = ThreadSafeQueue<DemuxPacket>;



    bool initDecoders(const PlaybackOpenPlan& plan);

    void releaseDecoders();

    bool tryConfigureHardwareDecode(const AVCodec* codec,
                                    AVCodecContext* codec_ctx,
                                    decoder::DecoderBackend backend);

    bool configureHardwareFramesContext(AVCodecContext* codec_ctx);

    bool configureD3D11HardwareFramesContext(AVCodecContext* codec_ctx);

    static AVPixelFormat selectVideoPixelFormat(AVCodecContext* ctx, const AVPixelFormat* pix_fmts);

    static AVPixelFormat selectSoftwarePixelFormat(const AVPixelFormat* pix_fmts);

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

    void applyStartPlaybackSideEffects(const char* reason);

    void applyResumePlaybackSideEffects(const char* reason);

    void applyPausePlaybackSideEffects(const char* reason);

    void applyStopRequestSideEffects(const char* reason);

    void applyStopCompletionSideEffects(const char* reason, bool rewind_demux, bool reset_position);

    bool applySeekSideEffects(double timestamp,
                              bool was_playing,
                              bool demux_was_running,
                              TimelineSerial pending_seek_serial,
                              const char* reason);

    void applySessionReleaseSideEffects(const char* reason);

    bool handleRuntimeFailure(ErrorCode code,
                              const char* message,
                              const char* reason,
                              FailureRecoveryPolicy policy);

    void requestDeferredStop();

    void serviceDeferredStop();

    void reapCompletedWorkers();



    bool decodeVideoFrame(VideoFrame& out);

    bool decodeAudioFrame(AudioFrame& out);

    bool renderFrame(VideoFrame&& frame);

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
    void setEmbeddedSubtitles(std::vector<subtitle::SubtitleItem> subtitles, const std::string& source_path);
    void clearEmbeddedSubtitles();
    void refreshActiveSubtitleTrackLocked();
    bool applyEmbeddedSubtitleTrackSelectionLocked(int stream_index, bool force_reload);
    int selectedEmbeddedSubtitleTrackOrdinalLocked() const;
    void updateSubtitleTrackOverlayState();

    bool transitionSessionState(SessionState next, const char* reason);

    bool transitionRunState(RunState next, const char* reason);

    bool transitionPipelinePhase(PipelinePhase next, const char* reason);

    TimelineSerial allocateNextTimelineSerial(const char* reason);

    void activateTimelineSerial(TimelineSerial serial, const char* reason);

    void setPendingSeekSerial(TimelineSerial serial, const char* reason);

    void clearPendingSeekSerial(const char* reason);

    TimelineSerial currentTimelineSerial() const;

    TimelineSerial acceptedTimelineSerial() const;

    bool isCurrentTimelineSerial(TimelineSerial serial) const;

    bool isAcceptedTimelineSerial(TimelineSerial serial) const;

    void setEofReached(bool eof_reached, const char* reason);

    void setDeferredStopPending(bool deferred_stop_pending, const char* reason);

    void setEndedReason(EndedReason ended_reason, const char* reason);

    bool consumeDeferredStopPending(const char* reason);

    void armDeferredFailSession(ErrorCode code, const char* message, const char* reason);

    bool consumeDeferredFailSession(ErrorCode& code, std::string& message, std::string& reason);

    void clearDeferredFailSession(const char* reason);

    CoreStateSnapshot getCoreStateSnapshot() const;

    SchedulerControlSnapshot makeSchedulerControlSnapshot() const;

    void publishPlaybackStateFromInternalState(const char* reason);



    void emitStateChanged(PlaybackState state);

    void emitPositionChanged(double position);

    void emitFrameRendered();

    void emitError(ErrorCode code, const std::string& message);

    void resetDiagnostics();

    void maybeLogDiagnostics(const char* source_tag);



    std::unique_ptr<Demuxer> demuxer_;

    render::VideoRendererPtr video_renderer_;
    input::IPlaybackInputSource* playback_input_source_{nullptr};
    render::IRenderOverlaySink* render_overlay_sink_{nullptr};

    std::unique_ptr<AudioPlayer> audio_player_;

    bool audio_device_open_attempted_{false};

    std::uint64_t audio_init_latency_ms_{0};

    std::string audio_init_strategy_{"not-run"};

    std::string audio_init_detail_;

    std::string startup_platform_{"Unknown"};

    std::string startup_renderer_capabilities_{"none"};

    std::string startup_decoder_capabilities_{"none"};
    std::string startup_renderer_compiled_set_{"none"};
    std::string startup_renderer_runtime_set_{"none"};
    std::string startup_decoder_compiled_set_{"none"};
    std::string startup_decoder_runtime_set_{"none"};

    std::string startup_renderer_candidates_{"none"};

    std::string startup_decoder_candidates_{"none"};

    std::string startup_renderer_plan_reason_{"none"};

    std::string startup_decoder_plan_reason_{"none"};

    std::string startup_selected_renderer_{"None"};

    std::string startup_selected_decoder_{"Unknown"};

    std::string startup_renderer_fallback_reason_{"none"};

    std::string startup_decoder_fallback_reason_{"none"};



    Scheduler scheduler_;

    Clock clock_;

    VideoFrameQueue video_queue_{16};

    AudioFrameQueue audio_queue_{32};

    std::unique_ptr<PacketQueue> video_packet_queue_;

    std::unique_ptr<PacketQueue> audio_packet_queue_;



    WorkerThread demux_worker_;

    WorkerThread audio_consumer_worker_;



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

    uint64_t swr_in_channel_layout_{0};

    uint64_t swr_out_channel_layout_{0};

    int swr_in_channels_{0};

    int swr_out_channels_{0};

    AVSampleFormat swr_in_sample_fmt_{AV_SAMPLE_FMT_NONE};

    AVSampleFormat swr_out_sample_fmt_{AV_SAMPLE_FMT_NONE};

    int swr_in_sample_rate_{0};

    int swr_out_sample_rate_{0};

    AVRational video_time_base_{0, 1};

    AVRational audio_time_base_{0, 1};



    std::atomic<PlaybackState> state_{PlaybackState::Stopped};

    std::atomic<TimelineSerial> next_timeline_serial_{1};

    std::atomic<TimelineSerial> audio_output_serial_{kInvalidTimelineSerial};

    mutable std::mutex core_state_mutex_;

    CoreStateSnapshot core_state_{};

    mutable std::mutex deferred_fail_session_mutex_;

    DeferredFailSessionState deferred_fail_session_{};

    std::atomic<double> position_{0.0};

    std::atomic<float> volume_{1.0f};

    std::atomic<double> speed_{1.0};

    std::atomic<double> audio_delay_seconds_{0.0};

    std::atomic<double> subtitle_delay_seconds_{0.0};

    std::atomic<bool> prefer_hardware_decode_{true};

    std::atomic<render::VideoRendererType> preferred_renderer_{render::VideoRendererType::Auto};

    std::atomic<bool> opened_{false};

    std::atomic<bool> quit_requested_{false};

    std::atomic<bool> next_item_requested_{false};

    std::atomic<bool> previous_item_requested_{false};
    std::thread::id event_thread_id_{};
    std::atomic<bool> event_thread_violation_logged_{false};

    std::mutex open_file_request_mutex_;

    bool open_file_requested_{false};

    std::string open_file_path_;

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

    std::atomic<uint64_t> stale_video_packets_dropped_{0};

    std::atomic<uint64_t> stale_audio_packets_dropped_{0};

    std::atomic<uint64_t> stale_video_frames_dropped_{0};

    std::atomic<uint64_t> stale_audio_frames_dropped_{0};

    std::atomic<uint64_t> stale_audio_submit_frames_dropped_{0};

    std::atomic<uint64_t> stale_render_frames_dropped_{0};

    std::atomic<uint64_t> stale_paused_render_frames_dropped_{0};

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

    std::atomic<uint64_t> runtime_failure_stop_requests_{0};

    std::atomic<uint64_t> runtime_failure_fail_sessions_{0};

    std::atomic<uint64_t> illegal_session_transitions_{0};

    std::atomic<uint64_t> illegal_run_transitions_{0};

    std::atomic<uint64_t> illegal_pipeline_transitions_{0};

    std::atomic<int64_t> last_diag_log_ms_{0};

    std::atomic<int64_t> last_video_decode_issue_log_ms_{0};

    std::atomic<bool> first_video_send_started_logged_{false};

    std::atomic<bool> first_video_send_completed_logged_{false};

    std::recursive_mutex video_codec_mutex_;

    std::recursive_mutex audio_codec_mutex_;

    TimelineSerial video_decoder_packet_serial_{kInvalidTimelineSerial};

    TimelineSerial audio_decoder_packet_serial_{kInvalidTimelineSerial};

    bool video_decoder_draining_{false};

    bool audio_decoder_draining_{false};



    mutable std::mutex subtitle_mutex_;

    std::vector<subtitle::SubtitleItem> subtitle_items_;
    std::vector<subtitle::SubtitleItem> external_subtitle_items_;
    std::vector<subtitle::SubtitleItem> embedded_subtitle_items_;
    std::vector<subtitle::EmbeddedSubtitleTrackInfo> embedded_subtitle_tracks_;

    std::string current_media_source_path_;

    std::string subtitle_source_path_;
    std::string external_subtitle_source_path_;
    std::string embedded_subtitle_source_path_;
    int embedded_subtitle_selected_stream_index_{-1};
    int preferred_embedded_subtitle_stream_index_{-1};
    subtitle::EmbeddedSubtitleSelectionPolicy embedded_subtitle_selection_policy_{};

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
