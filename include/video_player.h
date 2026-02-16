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
#include <queue>
#include <functional>

namespace vp {

using Clock = std::chrono::steady_clock;
using TimePoint = std::chrono::time_point<Clock>;

class VideoDecoder;
class AudioDecoder;
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
    double getDuration() const { return duration_; }
    double getCurrentTime() const { return current_time_.load(); }
    
    void setVolume(float volume);
    float getVolume() const { return volume_; }
    
    void setPlaybackSpeed(double speed);
    double getPlaybackSpeed() const { return playback_speed_; }

private:
    void playLoop();
    void updateClock();
    double getMasterClock();
    
    std::unique_ptr<VideoDecoder> video_decoder_;
    std::unique_ptr<AudioDecoder> audio_decoder_;
    std::unique_ptr<Display> display_;
    std::unique_ptr<AudioPlayer> audio_player_;
    
    std::atomic<bool> playing_;
    std::atomic<bool> paused_;
    std::atomic<bool> stopped_;
    std::atomic<bool> seeking_;
    
    double duration_;
    std::atomic<double> current_time_;
    float volume_;
    double playback_speed_;
    
    std::thread play_thread_;
    std::mutex mutex_;
    std::condition_variable cv_;
};

} // namespace vp
