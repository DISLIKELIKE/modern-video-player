#include "video_player.h"

#include <algorithm>

#include "logger.h"

namespace vp {

VideoPlayer::VideoPlayer() : core_player_(std::make_unique<core::PlayerCore>()) {
    core_player_->onStateChanged([this](core::PlaybackState state) {
        playing_.store(state == core::PlaybackState::Playing);
        paused_.store(state == core::PlaybackState::Paused);
    });
    core_player_->onPositionChanged([this](double pos) {
        current_time_.store(pos);
    });
}

VideoPlayer::~VideoPlayer() {
    close();
}

bool VideoPlayer::open(const std::string& filename) {
    if (!core_player_) {
        core_player_ = std::make_unique<core::PlayerCore>();
    }
    const bool ok = core_player_->open(filename);
    if (!ok) {
        LOG_ERROR("Failed to open file with PlayerCore");
        return false;
    }
    core_player_->setVolume(volume_);
    core_player_->setPlaybackSpeed(playback_speed_);
    return true;
}

void VideoPlayer::close() {
    if (core_player_) {
        core_player_->close();
    }
    playing_.store(false);
    paused_.store(false);
    current_time_.store(0.0);
}

void VideoPlayer::play() {
    if (core_player_) {
        core_player_->play();
    }
}

void VideoPlayer::pause() {
    if (core_player_) {
        core_player_->pause();
    }
}

void VideoPlayer::stop() {
    if (core_player_) {
        core_player_->stop();
    }
}

void VideoPlayer::seek(double timestamp) {
    if (core_player_) {
        core_player_->seek(timestamp);
    }
}

bool VideoPlayer::isPlaying() const {
    return playing_.load();
}

bool VideoPlayer::isPaused() const {
    return paused_.load();
}

double VideoPlayer::getDuration() const {
    return core_player_ ? core_player_->getInfo().duration : 0.0;
}

double VideoPlayer::getCurrentTime() const {
    return current_time_.load();
}

void VideoPlayer::setVolume(float volume) {
    volume_ = std::max(0.0f, std::min(1.0f, volume));
    if (core_player_) {
        core_player_->setVolume(volume_);
    }
}

float VideoPlayer::getVolume() const {
    return volume_;
}

void VideoPlayer::setPlaybackSpeed(double speed) {
    playback_speed_ = std::max(0.5, std::min(2.0, speed));
    if (core_player_) {
        core_player_->setPlaybackSpeed(playback_speed_);
    }
}

double VideoPlayer::getPlaybackSpeed() const {
    return playback_speed_;
}

}  // namespace vp

