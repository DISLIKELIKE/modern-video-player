#pragma once

#include <atomic>
#include <memory>
#include <string>

#include "core/player_core.h"

namespace vp {

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

    bool isPlaying() const;
    bool isPaused() const;
    double getDuration() const;
    double getCurrentTime() const;

    void setVolume(float volume);
    float getVolume() const;

    void setPlaybackSpeed(double speed);
    double getPlaybackSpeed() const;

private:
    std::unique_ptr<core::PlayerCore> core_player_;
    std::atomic<bool> playing_{false};
    std::atomic<bool> paused_{false};
    std::atomic<double> current_time_{0.0};
    float volume_{1.0f};
    double playback_speed_{1.0};
};

}  // namespace vp

