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
#include "render/video_renderer.h"
#include "thread_safe_queue.h"

namespace vp {
class AudioPlayer;
}  // namespace vp
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
    void pumpEvents();
    bool consumeQuitRequest();
    bool consumeNextItemRequest();
    bool consumePreviousItemRequest();

    PlaybackState getState() const;
    PlaybackInfo getInfo() const;

    void setVolume(float volume);
    float getVolume() const;

    void setPlaybackSpeed(double speed);
    double getPlaybackSpeed() const;

    using StateCallback = std::function<void(PlaybackState)>;
    using PositionCallback = std::function<void(double)>;
    using ErrorCallback = std::function<void(ErrorCode, const std::string&)>;
    using FrameCallback = std::function<void()>;

    void onStateChanged(StateCallback callback);
    void onPositionChanged(PositionCallback callback);
    void onError(ErrorCallback callback);
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
    std::atomic<bool> opened_{false};
    std::atomic<bool> quit_requested_{false};
    std::atomic<bool> next_item_requested_{false};
    std::atomic<bool> previous_item_requested_{false};
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
};

}  // namespace vp::core
