#pragma once

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

#include <SDL2/SDL.h>
#include <string>
#include <memory>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>

#include "demuxer.h"
#include "decoder_worker.h"
#include "thread_safe_queue.h"
#include "clock.h"

namespace vp {

class Display;
class AudioPlayer;

class VideoPlayer {
public:
    VideoPlayer();
    ~VideoPlayer();

    bool open(const std::string& filename);
    void close();

    void play();
    void pause();
    void stop();
    void seek(double timestamp);

    bool isPlaying() const { return playing_.load(); }
    bool isPaused() const { return paused_.load(); }
    double getDuration() const;
    double getCurrentTime() const { return current_time_.load(); }

    void setVolume(float volume);
    float getVolume() const { return volume_; }

    void setPlaybackSpeed(double speed);
    double getPlaybackSpeed() const { return playback_speed_; }

    void setSyncMode(SyncMode mode);
    SyncMode getSyncMode() const { return clock_.getMode(); }

private:
    void renderLoop();
    void handleVideoFrame(AVFrame* frame);
    void handleAudioFrame(AVFrame* frame);
    bool initComponents();
    void startThreads();
    void stopThreads();

    std::unique_ptr<Demuxer> demuxer_;
    std::unique_ptr<Display> display_;
    std::unique_ptr<AudioPlayer> audio_player_;

    std::unique_ptr<DecoderWorker> video_decoder_;
    std::unique_ptr<DecoderWorker> audio_decoder_;

    using PacketQueue = ThreadSafeQueue<AVPacket*>;
    std::unique_ptr<PacketQueue> video_packet_queue_;
    std::unique_ptr<PacketQueue> audio_packet_queue_;

    std::unique_ptr<std::thread> demuxer_thread_;
    std::unique_ptr<std::thread> render_thread_;

    Clock clock_;

    std::atomic<bool> playing_{false};
    std::atomic<bool> paused_{false};
    std::atomic<bool> stopped_{true};
    std::atomic<bool> seeking_{false};

    std::atomic<double> current_time_{0.0};
    float volume_ = 1.0f;
    double playback_speed_ = 1.0;

    std::mutex mutex_;
    std::condition_variable cv_;

    AVFrame* pending_video_frame_ = nullptr;
    double pending_video_pts_ = 0.0;
};

}
