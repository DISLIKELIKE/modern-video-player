#pragma once

extern "C" {
#include <libavcodec/avcodec.h>
}

#include <atomic>
#include <chrono>
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
#include "demuxer.h"
#include "filters/filter_pipeline.h"
#include "thread_safe_queue.h"

namespace vp {
class AudioPlayer;
class Display;
}  // namespace vp

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
    void startDemuxThread();
    void stopDemuxThread();
    void startAudioConsumer();
    void stopAudioConsumer();
    void flushPipelines();

    bool decodeVideoFrame(VideoFrame& out);
    bool decodeAudioFrame(AudioFrame& out);
    void renderFrame(VideoFrame&& frame);

    void emitStateChanged(PlaybackState state);
    void emitPositionChanged(double position);
    void emitFrameRendered();
    void emitError(ErrorCode code, const std::string& message);

    std::unique_ptr<Demuxer> demuxer_;
    std::unique_ptr<Display> display_;
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
    AVRational video_time_base_{0, 1};
    AVRational audio_time_base_{0, 1};

    std::atomic<PlaybackState> state_{PlaybackState::Stopped};
    std::atomic<double> position_{0.0};
    std::atomic<float> volume_{1.0f};
    std::atomic<double> speed_{1.0};
    std::atomic<bool> opened_{false};
    filters::FilterPipeline filter_pipeline_;

    mutable std::mutex callback_mutex_;
    std::vector<StateCallback> state_callbacks_;
    std::vector<PositionCallback> position_callbacks_;
    std::vector<ErrorCallback> error_callbacks_;
    std::vector<FrameCallback> frame_callbacks_;
    std::chrono::steady_clock::time_point last_position_emit_tp_;
};

}  // namespace vp::core
