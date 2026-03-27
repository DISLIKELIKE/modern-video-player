#pragma once

#include <memory>
#include <string>

#include "input/playback_input_source.h"
#include "render/render_overlay_sink.h"
#include "render/video_renderer.h"

namespace vp::render {

class VulkanVideoRenderer final : public IVideoRenderer,
                                  public input::IPlaybackInputSource,
                                  public IRenderOverlaySink {
public:
    VulkanVideoRenderer();
    ~VulkanVideoRenderer() override;

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
    bool consumePreviousSubtitleTrackRequest() override;
    bool consumeNextSubtitleTrackRequest() override;
    bool consumeSubtitleDelayChangeRequest(double& delta_seconds) override;
    bool consumeAudioDelayChangeRequest(double& delta_seconds) override;
    bool consumeNextChapterRequest() override;
    bool consumePreviousChapterRequest() override;
    bool consumeNextItemRequest() override;
    bool consumePreviousItemRequest() override;
    bool consumeOpenFileRequest(std::string& path) override;

    void setOverlayState(double position, double duration, float volume, bool paused) override;
    void setSubtitleText(const std::string& text) override;
    void setHotkeyManager(const input::HotkeyManager& hotkey_manager) override;
    bool supportsDirectFrameFormat(AVPixelFormat format) const override;

    const char* rendererBackendName() const override;

private:
    struct VulkanState;
    std::unique_ptr<VulkanState> state_;
};

}  // namespace vp::render
