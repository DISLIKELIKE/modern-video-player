#include "clock.h"
#include <cmath>

namespace vp {

Clock::Clock() {
    start_time_ = std::chrono::steady_clock::now();
}

void Clock::setMasterClock(double pts) {
    master_clock_.store(pts);
    start_time_ = std::chrono::steady_clock::now();
}

double Clock::getMasterClock() const {
    return master_clock_.load() + getCurrentTime() * playback_speed_.load();
}

void Clock::updateVideoPts(double pts) {
    video_pts_.store(pts);
}

void Clock::updateAudioPts(double pts) {
    audio_pts_.store(pts);
}

double Clock::calculateDelay(double frame_pts) const {
    double master = getMasterClock();
    double diff = frame_pts - master;

    if (std::abs(diff) < sync_threshold_) {
        return 0.0;
    }

    return diff / playback_speed_.load();
}

bool Clock::shouldSkipFrame(double frame_pts) const {
    double master = getMasterClock();
    double diff = frame_pts - master;
    return diff < -sync_threshold_ * 2;
}

bool Clock::shouldRepeatFrame(double frame_pts) const {
    double master = getMasterClock();
    double diff = master - frame_pts;
    return diff > sync_threshold_ * 2;
}

void Clock::reset() {
    master_clock_.store(0.0);
    video_pts_.store(0.0);
    audio_pts_.store(0.0);
    start_time_ = std::chrono::steady_clock::now();
}

void Clock::setPlaybackSpeed(double speed) {
    playback_speed_.store(speed);
}

double Clock::getCurrentTime() const {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration<double>(now - start_time_).count();
    return elapsed;
}

}
