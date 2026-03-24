#pragma once

#include <memory>
#include <string>
#include <vector>

#include "render/video_renderer.h"

namespace vp::render {

class OpenGLVideoRenderer final : public IVideoRenderer {
public:
    OpenGLVideoRenderer();
    ~OpenGLVideoRenderer() override;

    bool init(const VideoRendererConfig& config) override;
    void close() override;
    void renderFrame(const core::VideoFrame& frame) override;
    void present() override;
    void clear() override;
    void handleEvents() override;
    bool shouldQuit() const override;
    bool consumeTogglePauseRequest() override;
    bool consumeSeekRequest(double& normalized_position) override;
    bool consumeSeekDeltaRequest(double& delta_seconds) override;
    bool consumeVolumeChangeRequest(float& volume) override;
    bool consumeSpeedChangeRequest(double& speed_delta) override;
    bool consumeResetSpeedRequest() override;
    bool consumeToggleSubtitleRequest() override;
    bool consumeSetABRepeatStartRequest() override;
    bool consumeSetABRepeatEndRequest() override;
    bool consumeClearABRepeatRequest() override;
    bool consumeScreenshotRequest() override;
    bool consumeStepFrameBackwardRequest() override;
    bool consumeStepFrameForwardRequest() override;
    bool consumeSubtitleDelayChangeRequest(double& delta_seconds) override;
    bool consumeAudioDelayChangeRequest(double& delta_seconds) override;
    bool consumeNextChapterRequest() override;
    bool consumePreviousChapterRequest() override;
    bool consumeNextItemRequest() override;
    bool consumePreviousItemRequest() override;
    void setOverlayState(double position, double duration, float volume, bool paused) override;
    void setSubtitleText(const std::string& text) override;
    void setSubtitleItems(const std::vector<subtitle::SubtitleItem>& items) override;
    void setHotkeyManager(const input::HotkeyManager& hotkey_manager) override;
    RendererDiagnostics getDiagnostics() const override;
    void resetDiagnostics() override;
    bool supportsNativeFrameFormat(AVPixelFormat format) const override;
    bool supportsDirectFrameFormat(AVPixelFormat format) const override;
    void* nativeDeviceHandle() const override;
    const char* rendererBackendName() const override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace vp::render
