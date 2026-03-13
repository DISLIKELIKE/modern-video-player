#pragma once

#include <atomic>
#include <chrono>
#include <mutex>

namespace vp::core {

/// 播放主时钟来源。
enum class ClockSource {
    /// 以音频设备实际播放位置为准。
    Audio,
    /// 以最近渲染视频帧时间为准。
    Video,
    /// 以本地单调时钟推算为准。
    System
};

/// 播放时钟封装；可在音频、视频和系统时钟之间切换主参考源。
class Clock {
public:
    using TimePoint = std::chrono::steady_clock::time_point;

    /// 创建并重置到初始状态。
    Clock();

    /// 切换当前主时钟来源。
    void setSource(ClockSource source);
    /// 返回当前主时钟来源。
    ClockSource getSource() const;

    /// 返回当前解析后的播放时间（秒）。
    double getTime() const;
    /// 重置系统时钟基准时间；当主时钟为 `System` 或暂停时生效。
    void setTime(double time);

    /// 更新音频时钟。
    void setAudioClock(double time);
    /// 返回最近写入的音频时钟。
    double getAudioClock() const;

    /// 更新视频时钟。
    void setVideoClock(double time);
    /// 返回最近写入的视频时钟。
    double getVideoClock() const;

    /// 设置系统时钟倍速，下限为 0.1。
    void setSpeed(double speed);
    /// 返回当前系统时钟倍速。
    double getSpeed() const;

    /// 冻结当前解析时间，供暂停态读取。
    void pause();
    /// 从冻结时间继续推进系统时钟。
    void resume();
    /// 清空所有时钟状态并恢复默认倍速。
    void reset();

private:
    mutable std::mutex mutex_;
    std::atomic<double> audio_clock_{0.0};
    std::atomic<double> video_clock_{0.0};
    std::atomic<double> speed_{1.0};
    std::atomic<ClockSource> source_{ClockSource::Audio};
    std::atomic<bool> paused_{false};
    TimePoint base_tp_;
    double base_time_{0.0};
};

}  // namespace vp::core

