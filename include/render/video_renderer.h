#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "core/frame.h"
#include "input/hotkey_manager.h"
#include "subtitle/subtitle_parser.h"

namespace vp::render {

enum class VideoRendererType {
    Auto,
    SoftwareSDL,
    D3D11,
    OpenGL
};

struct VideoRendererConfig {
    int width{0};
    int height{0};
    std::string title{"Video Player"};
};

struct RendererDiagnostics {
    uint64_t display_copy_frames{0};
    uint64_t display_copy_bytes{0};
    uint64_t display_copy_time_us_total{0};
    uint64_t display_copy_time_us_max{0};
};

class IVideoRenderer {
public:
    virtual ~IVideoRenderer() = default;

    virtual bool init(const VideoRendererConfig& config) = 0;
    virtual void close() = 0;

    virtual void renderFrame(const core::VideoFrame& frame) = 0;
    virtual void present() = 0;
    virtual void clear() = 0;

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
    virtual bool consumeSubtitleDelayChangeRequest(double& delta_seconds) = 0;
    virtual bool consumeAudioDelayChangeRequest(double& delta_seconds) = 0;
    virtual bool consumeNextChapterRequest() = 0;
    virtual bool consumePreviousChapterRequest() = 0;
    virtual bool consumeNextItemRequest() = 0;
    virtual bool consumePreviousItemRequest() = 0;

    virtual void setOverlayState(double position, double duration, float volume, bool paused) = 0;
    virtual void setSubtitleText(const std::string& text) = 0;
    virtual void setSubtitleItems(const std::vector<subtitle::SubtitleItem>& items) {
        setSubtitleText(subtitle::flattenSubtitleText(items));
    }
    virtual void setHotkeyManager(const input::HotkeyManager& hotkey_manager) = 0;

    virtual bool supportsNativeFrameFormat(AVPixelFormat format) const {
        (void)format;
        return false;
    }

    virtual bool supportsDirectFrameFormat(AVPixelFormat format) const {
        (void)format;
        return false;
    }

    virtual void* nativeDeviceHandle() const {
        return nullptr;
    }

    virtual RendererDiagnostics getDiagnostics() const {
        return {};
    }

    virtual void resetDiagnostics() {}

    virtual const char* rendererBackendName() const = 0;
};

using VideoRendererPtr = std::unique_ptr<IVideoRenderer>;

}  // namespace vp::render
