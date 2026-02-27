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

#include "frame_queue.h"
#include "packet_reader.h"
#include "video_decode_thread.h"
#include "audio_decode_thread.h"
#include "sync_manager.h"

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

    void setSyncMode(SyncMode mode);
    SyncMode getSyncMode() const { return sync_manager_.getMode(); }

private:
    void renderLoop();
    void updateClock();
    double getMasterClock();
    
    bool initDecodeThreads(AVFormatContext* fmt_ctx, int video_stream_idx, int audio_stream_idx);
    void stopDecodeThreads();

    std::unique_ptr<VideoDecoder> video_decoder_;
    std::unique_ptr<AudioDecoder> audio_decoder_;
    std::unique_ptr<Display> display_;
    std::unique_ptr<AudioPlayer> audio_player_;
    
    std::unique_ptr<PacketReaderThread> packet_reader_;
    std::unique_ptr<PacketQueue<PacketRef>> video_packet_queue_;
    std::unique_ptr<PacketQueue<PacketRef>> audio_packet_queue_;
    
    std::unique_ptr<VideoDecodeThread> video_decode_thread_;
    std::unique_ptr<AudioDecodeThread> audio_decode_thread_;
    
    std::unique_ptr<FrameQueue<VideoFrame>> video_frame_queue_;
    std::unique_ptr<FrameQueue<AudioFrame>> audio_frame_queue_;
    
    SyncManager sync_manager_;
    
    std::atomic<bool> playing_;
    std::atomic<bool> paused_;
    std::atomic<bool> stopped_;
    std::atomic<bool> seeking_;
    
    double duration_;
    std::atomic<double> current_time_;
    float volume_;
    double playback_speed_;
    
    std::thread render_thread_;
    std::mutex mutex_;
    std::condition_variable cv_;
    
    AVFormatContext* format_ctx_;
    int video_stream_idx_;
    int audio_stream_idx_;
};

}
