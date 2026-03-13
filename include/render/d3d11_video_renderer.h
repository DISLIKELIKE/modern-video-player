#pragma once

#include <memory>

#include "display.h"
#include "render/video_renderer.h"

namespace vp::render {

/// D3D11 渲染包装器；底层仍复用 `Display`，并校验 SDL 实际驱动是否为 D3D11。
class D3D11VideoRenderer final : public IVideoRenderer {
public:
    D3D11VideoRenderer();
    ~D3D11VideoRenderer() override;

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

private:
    std::unique_ptr<Display> display_;
};

}  // namespace vp::render