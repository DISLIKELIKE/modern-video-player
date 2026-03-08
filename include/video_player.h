#pragma once

#include <atomic>
#include <memory>
#include <string>
#include <vector>

#include "core/player_core.h"
#include "subtitle/subtitle_parser.h"

namespace vp {

class VideoPlayer {
public:
    VideoPlayer();
    ~VideoPlayer();

    bool open(const std::string& filename);
    void close();

    void play();
    void pause();
    void stop();
    void seek(double timestamp);
    bool seekToNextChapter();
    bool seekToPreviousChapter();
    size_t chapterCount() const;
    void pumpEvents();
    bool consumeQuitRequest();
    bool consumeNextItemRequest();
    bool consumePreviousItemRequest();

    bool isPlaying() const;
    bool isPaused() const;
    double getDuration() const;
    double getCurrentTime() const;

    void setVolume(float volume);
    float getVolume() const;

    void setPlaybackSpeed(double speed);
    double getPlaybackSpeed() const;
    void setPreferHardwareDecode(bool prefer_hardware_decode);
    bool preferHardwareDecode() const;
    std::string videoRendererBackendName() const;
    const char* videoDecoderBackendName() const;
    void setHotkeyManager(const input::HotkeyManager& hotkey_manager);
    const input::HotkeyManager& hotkeyManager() const;

    bool loadExternalSubtitle(const std::string& subtitle_file);
    void clearExternalSubtitle();
    bool hasExternalSubtitle() const;
    const std::string& externalSubtitlePath() const;
    size_t externalSubtitleCount() const;
    void setSubtitleEnabled(bool enabled);
    bool isSubtitleEnabled() const;
    bool toggleSubtitleEnabled();

private:
    std::unique_ptr<core::PlayerCore> core_player_;
    std::atomic<bool> playing_{false};
    std::atomic<bool> paused_{false};
    std::atomic<double> current_time_{0.0};
    float volume_{1.0f};
    double playback_speed_{1.0};
    bool prefer_hardware_decode_{true};
    std::string subtitle_path_;
    std::vector<subtitle::SubtitleItem> subtitle_items_;
    bool subtitle_enabled_{true};
};

}  // namespace vp

