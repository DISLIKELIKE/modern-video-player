#pragma once

#include <atomic>
#include <chrono>

namespace vp {

enum class SyncMode {
    AudioMaster,
    VideoMaster,
    Free
};

class SyncManager {
public:
    SyncManager();

    void setMode(SyncMode mode);
    SyncMode getMode() const { return mode_; }

    void setMasterClock(double pts);
    double getMasterClock() const { return master_clock_.load(); }

    double getVideoPts() const { return video_pts_.load(); }
    double getAudioPts() const { return audio_pts_.load(); }

    void updateVideoPts(double pts);
    void updateAudioPts(double pts);

    double calculateDelay(double current_pts);

    bool shouldSkipFrame(double frame_pts);
    bool shouldRepeatFrame(double frame_pts);

    void reset();

private:
    SyncMode mode_;
    std::atomic<double> master_clock_;
    std::atomic<double> video_pts_;
    std::atomic<double> audio_pts_;

    double sync_threshold_;
    double frame_rate_;
};

}
