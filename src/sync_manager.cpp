#include "sync_manager.h"
#include <cmath>

namespace vp {

SyncManager::SyncManager()
    : mode_(SyncMode::AudioMaster)
    , master_clock_(0.0)
    , video_pts_(0.0)
    , audio_pts_(0.0)
    , sync_threshold_(0.1)
    , frame_rate_(30.0) {
}

void SyncManager::setMode(SyncMode mode) {
    mode_ = mode;
}

void SyncManager::setMasterClock(double pts) {
    master_clock_.store(pts);
}

void SyncManager::updateVideoPts(double pts) {
    video_pts_.store(pts);
    
    if (mode_ == SyncMode::VideoMaster) {
        master_clock_.store(pts);
    }
}

void SyncManager::updateAudioPts(double pts) {
    audio_pts_.store(pts);
    
    if (mode_ == SyncMode::AudioMaster) {
        master_clock_.store(pts);
    }
}

double SyncManager::calculateDelay(double current_pts) {
    double diff = 0.0;
    
    switch (mode_) {
        case SyncMode::AudioMaster:
            diff = current_pts - audio_pts_.load();
            break;
        case SyncMode::VideoMaster:
            diff = current_pts - video_pts_.load();
            break;
        case SyncMode::Free:
            return 0.0;
    }
    
    if (std::abs(diff) < sync_threshold_) {
        return 0.0;
    }
    
    return diff;
}

bool SyncManager::shouldSkipFrame(double frame_pts) {
    double diff = frame_pts - master_clock_.load();
    return diff > sync_threshold_ * 2;
}

bool SyncManager::shouldRepeatFrame(double frame_pts) {
    double diff = master_clock_.load() - frame_pts;
    return diff > sync_threshold_ * 2;
}

void SyncManager::reset() {
    master_clock_.store(0.0);
    video_pts_.store(0.0);
    audio_pts_.store(0.0);
}

}
