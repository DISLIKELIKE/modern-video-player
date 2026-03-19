#include "core/clock.h"

#include <algorithm>

namespace vp::core {

Clock::Clock() {
    reset();
}

/// 切换主时钟来源；后续 `getTime()` 会按该来源解析播放时间。
void Clock::setSource(ClockSource source) {
    source_.store(source);
}

ClockSource Clock::getSource() const {
    return source_.load();
}

/// 根据当前主时钟来源返回解析后的播放时间。
double Clock::getTime() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (paused_.load()) {
        return base_time_;
    }

    const ClockSource src = source_.load();
    if (src == ClockSource::Audio) {
        return audio_clock_.load();
    }
    if (src == ClockSource::Video) {
        return video_clock_.load();
    }

    const auto now = std::chrono::steady_clock::now();
    const double elapsed = std::chrono::duration<double>(now - base_tp_).count();
    return base_time_ + elapsed * speed_.load();
}

/// 重置系统时钟基准到指定时间；常用于 seek 或单帧步进后同步。
void Clock::setTime(double time) {
    std::lock_guard<std::mutex> lock(mutex_);
    base_time_ = std::max(0.0, time);
    base_tp_ = std::chrono::steady_clock::now();
}

/// 更新音频时钟值；通常由音频消费线程按真实播放进度推进。
void Clock::setAudioClock(double time) {
    audio_clock_.store(std::max(0.0, time));
}

double Clock::getAudioClock() const {
    return audio_clock_.load();
}

/// 更新视频时钟值；通常由渲染线程在提交视频帧后推进。
void Clock::setVideoClock(double time) {
    video_clock_.store(std::max(0.0, time));
}

double Clock::getVideoClock() const {
    return video_clock_.load();
}

/// 更新系统时钟倍速；仅在以系统时钟为主时直接影响 `getTime()`。
void Clock::setSpeed(double speed) {
    const double clamped = std::max(0.1, speed);
    std::lock_guard<std::mutex> lock(mutex_);
    if (!paused_.load() && source_.load() == ClockSource::System) {
        const auto now = std::chrono::steady_clock::now();
        const double elapsed = std::chrono::duration<double>(now - base_tp_).count();
        base_time_ += elapsed * speed_.load();
        base_tp_ = now;
    }
    speed_.store(clamped);
}

double Clock::getSpeed() const {
    return speed_.load();
}

/// 将当前解析时间冻结到 `base_time_`，供暂停态持续读取。
void Clock::pause() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (paused_.load()) {
        return;
    }

    const auto now = std::chrono::steady_clock::now();
    const ClockSource src = source_.load();
    if (src == ClockSource::Audio) {
        base_time_ = std::max(0.0, audio_clock_.load());
    } else if (src == ClockSource::Video) {
        base_time_ = std::max(0.0, video_clock_.load());
    } else {
        const double elapsed = std::chrono::duration<double>(now - base_tp_).count();
        base_time_ += elapsed * speed_.load();
    }
    base_tp_ = now;
    paused_.store(true);
}

/// 从冻结时间继续推进系统时钟基准。
void Clock::resume() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!paused_.load()) {
        return;
    }
    base_tp_ = std::chrono::steady_clock::now();
    paused_.store(false);
}

/// 清空音视频时钟与系统基准，恢复默认播放速度。
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

