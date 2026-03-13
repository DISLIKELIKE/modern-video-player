#include "video_player.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <system_error>

#include "logger.h"
#include "decoder/decoder_factory.h"
#include "subtitle/srt_parser.h"

namespace vp {

/// 构造时创建 `PlayerCore`，并把底层状态同步到门面缓存。
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

/// 打开媒体并把当前用户态配置重新下发到底层播放核心。
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

/// 关闭底层链路并重置门面层缓存状态。
void VideoPlayer::close() {
    if (core_player_) {
        core_player_->close();
    }
    clearExternalSubtitle();
    playing_.store(false);
    paused_.store(false);
    current_time_.store(0.0);
}

/// 转发播放请求；从暂停态恢复或从停止态启动底层播放链路。
void VideoPlayer::play() {
    if (core_player_) {
        core_player_->play();
    }
}

/// 转发暂停请求；底层会冻结调度、时钟和音频设备。
void VideoPlayer::pause() {
    if (core_player_) {
        core_player_->pause();
    }
}

/// 转发停止请求；底层会清空管线并回到停止态。
void VideoPlayer::stop() {
    if (core_player_) {
        core_player_->stop();
    }
}

/// 转发按秒 seek 请求；实际 flush 与时钟重同步由 `PlayerCore` 完成。
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

/// 在暂停态向后逐帧，并同步门面缓存的位置。
bool VideoPlayer::stepFrameBackward() {
    const bool ok = core_player_ ? core_player_->stepFrameBackward() : false;
    if (ok && core_player_) {
        current_time_.store(core_player_->getInfo().position);
    }
    return ok;
}

/// 在暂停态向前逐帧，并同步门面缓存的位置。
bool VideoPlayer::stepFrameForward() {
    const bool ok = core_player_ ? core_player_->stepFrameForward() : false;
    if (ok && core_player_) {
        current_time_.store(core_player_->getInfo().position);
    }
    return ok;
}

/// 驱动显示层事件泵，把热键和窗口控制请求下发到底层。
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

core::PlaybackInfo VideoPlayer::getInfo() const {
    return core_player_ ? core_player_->getInfo() : core::PlaybackInfo{};
}

core::DiagnosticsSnapshot VideoPlayer::getDiagnosticsSnapshot() const {
    return core_player_ ? core_player_->getDiagnosticsSnapshot() : core::DiagnosticsSnapshot{};
}

/// 缓存并下发音量；门面层先把范围裁剪到 `[0, 1]`。
void VideoPlayer::setVolume(float volume) {
    volume_ = std::max(0.0f, std::min(1.0f, volume));
    if (core_player_) {
        core_player_->setVolume(volume_);
    }
}

float VideoPlayer::getVolume() const {
    return volume_;
}

/// 缓存并下发倍速；门面层先把范围裁剪到 `[0.5, 2.0]`。
void VideoPlayer::setPlaybackSpeed(double speed) {
    playback_speed_ = std::max(0.5, std::min(2.0, speed));
    if (core_player_) {
        core_player_->setPlaybackSpeed(playback_speed_);
    }
}

double VideoPlayer::getPlaybackSpeed() const {
    return playback_speed_;
}

/// 更新音频延迟配置，并立即同步到底层播放核心。
void VideoPlayer::setAudioDelay(double delay_seconds) {
    if (core_player_) {
        core_player_->setAudioDelay(delay_seconds);
    }
}

double VideoPlayer::getAudioDelay() const {
    return core_player_ ? core_player_->getAudioDelay() : 0.0;
}

/// 更新字幕延迟配置，并立即同步到底层字幕时间线。
void VideoPlayer::setSubtitleDelay(double delay_seconds) {
    if (core_player_) {
        core_player_->setSubtitleDelay(delay_seconds);
    }
}

double VideoPlayer::getSubtitleDelay() const {
    return core_player_ ? core_player_->getSubtitleDelay() : 0.0;
}

/// 记录硬解偏好，并影响后续打开媒体时的解码后端选择。
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

/// 校验并加载外挂字幕；成功后立即同步到 `PlayerCore`。
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

/// 清除外挂字幕缓存，并通知底层移除当前字幕时间线。
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

/// 更新字幕总开关，并立即同步到底层渲染链路。
void VideoPlayer::setSubtitleEnabled(bool enabled) {
    subtitle_enabled_ = enabled;
    if (core_player_) {
        core_player_->setSubtitleEnabled(enabled);
    }
}

bool VideoPlayer::isSubtitleEnabled() const {
    return subtitle_enabled_;
}

/// 翻转字幕开关并返回新的启用状态。
bool VideoPlayer::toggleSubtitleEnabled() {
    setSubtitleEnabled(!subtitle_enabled_);
    return subtitle_enabled_;
}

}  // namespace vp

