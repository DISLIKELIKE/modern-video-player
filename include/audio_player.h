#pragma once

#if __has_include(<SDL2/SDL.h>)
#include <SDL2/SDL.h>
#elif __has_include(<SDL.h>)
#include <SDL.h>
#else
#error "SDL2 headers not found"
#endif

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <vector>

namespace vp {

struct AudioInitReport {
    bool initialized{false};
    bool device_open_attempted{false};
    std::uint64_t elapsed_ms{0};
    std::string strategy{"not-run"};
    std::string detail;
};

class AudioPlayer {
public:
    AudioPlayer();
    ~AudioPlayer();

    AudioInitReport init(int sample_rate, int channels);
    void close();

    void play(const std::vector<uint8_t>& data, double pts = -1.0);
    void pause();
    void resume();
    void stop();

    void setVolume(float volume);
    float getVolume() const { return volume_.load(); }

    void setMuted(bool muted);
    bool isMuted() const { return muted_.load(); }
    double getPlaybackPts() const;
    size_t getQueuedBytes() const;
    double getBufferedSeconds() const;
    int outputSampleRate() const { return audio_spec_.freq; }
    int outputChannels() const { return audio_spec_.channels; }
    SDL_AudioFormat outputFormat() const { return audio_spec_.format; }
    int outputBytesPerSample() const;
    bool isInitialized() const { return initialized_; }
    const AudioInitReport& lastInitReport() const { return last_init_report_; }

private:
    struct AudioChunk {
        std::vector<uint8_t> data;
        size_t offset{0};
        double pts{-1.0};
    };

    static void audioCallback(void* userdata, uint8_t* stream, int len);

    SDL_AudioDeviceID audio_device_{0};
    SDL_AudioSpec audio_spec_{};

    int sample_rate_{0};
    int channels_{0};
    std::atomic<float> volume_{1.0f};
    std::atomic<bool> muted_{false};
    std::atomic<bool> paused_{false};

    std::queue<AudioChunk> audio_queue_;
    std::mutex queue_mutex_;
    std::atomic<double> playback_pts_{0.0};
    std::atomic<size_t> queued_bytes_{0};

    bool initialized_{false};
    AudioInitReport last_init_report_{};
};

}  // namespace vp
