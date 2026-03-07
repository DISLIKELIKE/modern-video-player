#pragma once

#include "render/video_renderer.h"

namespace vp::render {

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
    bool consumeVolumeChangeRequest(float& volume) override;
    void setOverlayState(double position, double duration, float volume, bool paused) override;
};

}  // namespace vp::render

