#pragma once

#include <string>

#include "input/hotkey_manager.h"

namespace vp::input {

class IPlaybackInputSource {
public:
    virtual ~IPlaybackInputSource() = default;

    virtual void handleEvents() = 0;
    virtual bool shouldQuit() const = 0;

    virtual bool consumeTogglePauseRequest() = 0;
    virtual bool consumeSeekRequest(double& normalized_position) = 0;
    virtual bool consumeSeekDeltaRequest(double& delta_seconds) = 0;
    virtual bool consumeVolumeChangeRequest(float& volume) = 0;
    virtual bool consumeSpeedChangeRequest(double& speed_delta) = 0;
    virtual bool consumeResetSpeedRequest() = 0;
    virtual bool consumeToggleSubtitleRequest() = 0;
    virtual bool consumeSetABRepeatStartRequest() = 0;
    virtual bool consumeSetABRepeatEndRequest() = 0;
    virtual bool consumeClearABRepeatRequest() = 0;
    virtual bool consumeScreenshotRequest() = 0;
    virtual bool consumeStepFrameBackwardRequest() = 0;
    virtual bool consumeStepFrameForwardRequest() = 0;
    virtual bool consumePreviousSubtitleTrackRequest() = 0;
    virtual bool consumeNextSubtitleTrackRequest() = 0;
    virtual bool consumeSubtitleDelayChangeRequest(double& delta_seconds) = 0;
    virtual bool consumeAudioDelayChangeRequest(double& delta_seconds) = 0;
    virtual bool consumeNextChapterRequest() = 0;
    virtual bool consumePreviousChapterRequest() = 0;
    virtual bool consumeNextItemRequest() = 0;
    virtual bool consumePreviousItemRequest() = 0;
    virtual bool consumeOpenFileRequest(std::string& path) = 0;

    virtual void setHotkeyManager(const HotkeyManager& hotkey_manager) = 0;
};

}  // namespace vp::input
