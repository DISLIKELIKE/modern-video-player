#pragma once

#include <SDL2/SDL.h>
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
    
    void play(const std::vector<uint8_t>& data);
    void pause();
    void resume();
    void stop();
    
    void setVolume(float volume);
    float getVolume() const { return volume_; }
    
    void setMuted(bool muted);
    bool isMuted() const { return muted_; }

private:
    static void audioCallback(void* userdata, uint8_t* stream, int len);
    
    SDL_AudioDeviceID audio_device_;
    SDL_AudioSpec audio_spec_;
    
    int sample_rate_;
    int channels_;
    float volume_;
    bool muted_;
    bool paused_;
    
    std::queue<std::vector<uint8_t>> audio_queue_;
    std::mutex queue_mutex_;
    
    bool initialized_;
};

} // namespace vp
