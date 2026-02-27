#pragma once

#include <atomic>
#include <chrono>

namespace vp {

enum class SyncMode {
    AudioMaster,
    VideoMaster,
    Free
};

class Clock {
public:
    using TimePoint = std::chrono::steady_clock::time_point;

    Clock();

    void setMode(SyncMode mode) { mode_ = mode; }
    SyncMode getMode() const { return mode_; }

    void setMasterClock(double pts);
    double getMasterClock() const;

    void updateVideoPts(double pts);
    void updateAudioPts(double pts);

    double calculateDelay(double frame_pts) const;
    bool shouldSkipFrame(double frame_pts) const;
    bool shouldRepeatFrame(double frame_pts) const;

    void reset();
    void setPlaybackSpeed(double speed);

private:
    double getCurrentTime() const;

    SyncMode mode_ = SyncMode::AudioMaster;
    std::atomic<double> master_clock_{0.0};
    std::atomic<double> video_pts_{0.0};
    std::atomic<double> audio_pts_{0.0};
    std::atomic<double> playback_speed_{1.0};

    TimePoint start_time_;
    double sync_threshold_ = 0.01;
};

}
