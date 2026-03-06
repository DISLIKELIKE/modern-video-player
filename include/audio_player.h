#pragma once

#include <SDL2/SDL.h>
#include <atomic>
#include <string>
#include <vector>
#include <queue>
#include <mutex>
#include <memory>

namespace vp {

class AudioPlayer {
public:
    AudioPlayer();
    ~AudioPlayer();
    
    bool init(int sample_rate, int channels);
    void close();
    
    void play(const std::vector<uint8_t>& data, double pts = -1.0);
    void pause();
    void resume();
    void stop();
    
    void setVolume(float volume);
    float getVolume() const { return volume_; }
    
    void setMuted(bool muted);
    bool isMuted() const { return muted_; }
    double getPlaybackPts() const;

private:
    struct AudioChunk {
        std::vector<uint8_t> data;
        size_t offset{0};
        double pts{-1.0};
    };

    static void audioCallback(void* userdata, uint8_t* stream, int len);
    
    SDL_AudioDeviceID audio_device_;
    SDL_AudioSpec audio_spec_;
    
    int sample_rate_;
    int channels_;
    float volume_;
    bool muted_;
    bool paused_;
    
    std::queue<AudioChunk> audio_queue_;
    std::mutex queue_mutex_;
    std::atomic<double> playback_pts_{0.0};
    std::atomic<size_t> queued_bytes_{0};
    
    bool initialized_;
};

} // namespace vp
