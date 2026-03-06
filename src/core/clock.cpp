#include "core/clock.h"

#include <algorithm>

namespace vp::core {

Clock::Clock() {
    reset();
}

void Clock::setSource(ClockSource source) {
    source_.store(source);
}

ClockSource Clock::getSource() const {
    return source_.load();
}

double Clock::getTime() const {
    if (paused_.load()) {
        std::lock_guard<std::mutex> lock(mutex_);
        return base_time_;
    }

    const ClockSource src = source_.load();
    if (src == ClockSource::Audio) {
        return audio_clock_.load();
    }
    if (src == ClockSource::Video) {
        return video_clock_.load();
    }

    std::lock_guard<std::mutex> lock(mutex_);
    const auto now = std::chrono::steady_clock::now();
    const double elapsed = std::chrono::duration<double>(now - base_tp_).count();
    return base_time_ + elapsed * speed_.load();
}

void Clock::setTime(double time) {
    std::lock_guard<std::mutex> lock(mutex_);
    base_time_ = std::max(0.0, time);
    base_tp_ = std::chrono::steady_clock::now();
}

void Clock::setAudioClock(double time) {
    audio_clock_.store(std::max(time, audio_clock_.load()));
}

double Clock::getAudioClock() const {
    return audio_clock_.load();
}

void Clock::setVideoClock(double time) {
    video_clock_.store(std::max(time, video_clock_.load()));
}

double Clock::getVideoClock() const {
    return video_clock_.load();
}

void Clock::setSpeed(double speed) {
    speed_.store(std::max(0.1, speed));
}

double Clock::getSpeed() const {
    return speed_.load();
}

void Clock::pause() {
    if (paused_.exchange(true)) {
        return;
    }
    setTime(getTime());
}

void Clock::resume() {
    if (!paused_.exchange(false)) {
        return;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    base_tp_ = std::chrono::steady_clock::now();
}

void Clock::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    audio_clock_.store(0.0);
    video_clock_.store(0.0);
    speed_.store(1.0);
    paused_.store(false);
    base_time_ = 0.0;
    base_tp_ = std::chrono::steady_clock::now();
}

}  // namespace vp::core

