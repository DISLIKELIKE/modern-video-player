#include "video_player.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <system_error>

#include "logger.h"
#include "decoder/decoder_factory.h"
#include "subtitle/srt_parser.h"

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
    core_player_->clearExternalSubtitles();
    const bool ok = core_player_->open(filename);
    if (!ok) {
        LOG_ERROR("Failed to open file with PlayerCore");
        return false;
    }
    core_player_->setVolume(volume_);
    core_player_->setPlaybackSpeed(playback_speed_);
    core_player_->setPreferHardwareDecode(prefer_hardware_decode_);
    core_player_->setSubtitleEnabled(subtitle_enabled_);
    if (!subtitle_items_.empty()) {
        core_player_->setExternalSubtitles(subtitle_items_, subtitle_path_);
    }
    return true;
}

void VideoPlayer::close() {
    if (core_player_) {
        core_player_->close();
    }
    clearExternalSubtitle();
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

bool VideoPlayer::seekToNextChapter() {
    return core_player_ ? core_player_->seekToNextChapter() : false;
}

bool VideoPlayer::seekToPreviousChapter() {
    return core_player_ ? core_player_->seekToPreviousChapter() : false;
}

size_t VideoPlayer::chapterCount() const {
    return core_player_ ? core_player_->chapterCount() : 0U;
}

bool VideoPlayer::setABRepeatStart() {
    return core_player_ ? core_player_->setABRepeatStart() : false;
}

bool VideoPlayer::setABRepeatEnd() {
    return core_player_ ? core_player_->setABRepeatEnd() : false;
}

void VideoPlayer::clearABRepeat() {
    if (core_player_) {
        core_player_->clearABRepeat();
    }
}

bool VideoPlayer::isABRepeatEnabled() const {
    return core_player_ ? core_player_->isABRepeatEnabled() : false;
}

double VideoPlayer::abRepeatStart() const {
    return core_player_ ? core_player_->abRepeatStart() : -1.0;
}

double VideoPlayer::abRepeatEnd() const {
    return core_player_ ? core_player_->abRepeatEnd() : -1.0;
}

bool VideoPlayer::requestScreenshot() {
    return core_player_ ? core_player_->requestScreenshot() : false;
}

bool VideoPlayer::consumeLastScreenshotPath(std::string& path) {
    return core_player_ ? core_player_->consumeLastScreenshotPath(path) : false;
}

bool VideoPlayer::stepFrameBackward() {
    const bool ok = core_player_ ? core_player_->stepFrameBackward() : false;
    if (ok && core_player_) {
        current_time_.store(core_player_->getInfo().position);
    }
    return ok;
}

bool VideoPlayer::stepFrameForward() {
    const bool ok = core_player_ ? core_player_->stepFrameForward() : false;
    if (ok && core_player_) {
        current_time_.store(core_player_->getInfo().position);
    }
    return ok;
}

void VideoPlayer::pumpEvents() {
    if (core_player_) {
        core_player_->pumpEvents();
    }
}

bool VideoPlayer::consumeQuitRequest() {
    return core_player_ ? core_player_->consumeQuitRequest() : false;
}

bool VideoPlayer::consumeNextItemRequest() {
    return core_player_ ? core_player_->consumeNextItemRequest() : false;
}

bool VideoPlayer::consumePreviousItemRequest() {
    return core_player_ ? core_player_->consumePreviousItemRequest() : false;
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

void VideoPlayer::setPreferHardwareDecode(bool prefer_hardware_decode) {
    prefer_hardware_decode_ = prefer_hardware_decode;
    if (core_player_) {
        core_player_->setPreferHardwareDecode(prefer_hardware_decode_);
    }
}

bool VideoPlayer::preferHardwareDecode() const {
    return prefer_hardware_decode_;
}

std::string VideoPlayer::videoRendererBackendName() const {
    return core_player_ ? core_player_->videoRendererBackendName() : "None";
}

const char* VideoPlayer::videoDecoderBackendName() const {
    if (!core_player_) {
        return "Unknown";
    }
    return decoder::DecoderFactory::backendName(core_player_->videoDecoderBackend());
}

void VideoPlayer::setHotkeyManager(const input::HotkeyManager& hotkey_manager) {
    if (core_player_) {
        core_player_->setHotkeyManager(hotkey_manager);
    }
}

const input::HotkeyManager& VideoPlayer::hotkeyManager() const {
    static const input::HotkeyManager kFallbackHotkeys{};
    return core_player_ ? core_player_->hotkeyManager() : kFallbackHotkeys;
}

bool VideoPlayer::loadExternalSubtitle(const std::string& subtitle_file) {
    clearExternalSubtitle();
    if (subtitle_file.empty()) {
        return false;
    }

    std::error_code ec;
    std::filesystem::path path(subtitle_file);
    std::string extension = path.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });

    if (extension != ".srt") {
        LOG_WARNING("Unsupported subtitle extension: " << subtitle_file);
        return false;
    }
    if (!std::filesystem::exists(path, ec) || ec || !std::filesystem::is_regular_file(path, ec) || ec) {
        LOG_WARNING("Subtitle file not found: " << subtitle_file);
        return false;
    }

    subtitle::SrtParser parser;
    bool parsed = false;
    try {
        parsed = parser.parseFile(path.string());
    } catch (const std::exception& ex) {
        LOG_WARNING("Subtitle parser raised exception: " << ex.what());
        return false;
    } catch (...) {
        LOG_WARNING("Subtitle parser raised unknown exception");
        return false;
    }
    if (!parsed) {
        LOG_WARNING("Failed to parse subtitle file: " << subtitle_file);
        return false;
    }

    subtitle_path_ = path.string();
    subtitle_items_ = parser.items();
    if (core_player_) {
        core_player_->setExternalSubtitles(subtitle_items_, subtitle_path_);
    }
    LOG_INFO("Loaded external subtitle: " << subtitle_path_ << " entries=" << subtitle_items_.size());
    return true;
}

void VideoPlayer::clearExternalSubtitle() {
    if (core_player_) {
        core_player_->clearExternalSubtitles();
    }
    subtitle_path_.clear();
    subtitle_items_.clear();
}

bool VideoPlayer::hasExternalSubtitle() const {
    return !subtitle_items_.empty();
}

const std::string& VideoPlayer::externalSubtitlePath() const {
    return subtitle_path_;
}

size_t VideoPlayer::externalSubtitleCount() const {
    return subtitle_items_.size();
}

void VideoPlayer::setSubtitleEnabled(bool enabled) {
    subtitle_enabled_ = enabled;
    if (core_player_) {
        core_player_->setSubtitleEnabled(enabled);
    }
}

bool VideoPlayer::isSubtitleEnabled() const {
    return subtitle_enabled_;
}

bool VideoPlayer::toggleSubtitleEnabled() {
    setSubtitleEnabled(!subtitle_enabled_);
    return subtitle_enabled_;
}

}  // namespace vp

