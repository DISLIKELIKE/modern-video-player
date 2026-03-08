#pragma once

#include <memory>

#include "display.h"
#include "render/video_renderer.h"

namespace vp::render {

class SdlVideoRenderer final : public IVideoRenderer {
public:
    SdlVideoRenderer();
    ~SdlVideoRenderer() override;

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
