#pragma once

#include <atomic>
#include <chrono>
#include <mutex>

namespace vp::core {

enum class ClockSource {
    Audio,
    Video,
    System
};

class Clock {
public:
    using TimePoint = std::chrono::steady_clock::time_point;

    Clock();

    void setSource(ClockSource source);
    ClockSource getSource() const;

    double getTime() const;
    void setTime(double time);

    void setAudioClock(double time);
    double getAudioClock() const;

    void setVideoClock(double time);
    double getVideoClock() const;

    void setSpeed(double speed);
    double getSpeed() const;

    void pause();
    void resume();
    void reset();

private:
    mutable std::mutex mutex_;
    std::atomic<double> audio_clock_{0.0};
    std::atomic<double> video_clock_{0.0};
    std::atomic<double> speed_{1.0};
    std::atomic<ClockSource> source_{ClockSource::Audio};
    std::atomic<bool> paused_{false};
    TimePoint base_tp_{};
    double base_time_{0.0};
};

}  // namespace vp::core
