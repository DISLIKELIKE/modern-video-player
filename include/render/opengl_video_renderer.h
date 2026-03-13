#pragma once

#include "render/video_renderer.h"

namespace vp::render {

/// OpenGL 渲染占位实现；当前接口已预留，但后端尚未真正落地。
class OpenGLVideoRenderer final : public IVideoRenderer {
public:
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
    void setHotkeyManager(const input::HotkeyManager& hotkey_manager) override;
    const char* rendererBackendName() const override;
};

}  // namespace vp::render