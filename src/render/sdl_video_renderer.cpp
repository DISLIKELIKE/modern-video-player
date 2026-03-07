#include "render/sdl_video_renderer.h"

namespace vp::render {

SdlVideoRenderer::SdlVideoRenderer() = default;

SdlVideoRenderer::~SdlVideoRenderer() {
    close();
}

bool SdlVideoRenderer::init(const VideoRendererConfig& config) {
    close();
    display_ = std::make_unique<Display>();
    return display_->init(config.width, config.height, config.title);
}

void SdlVideoRenderer::close() {
    if (display_) {
        display_->close();
        display_.reset();
    }
}

void SdlVideoRenderer::renderFrame(const core::VideoFrame& frame) {
    if (!display_ || !frame.valid || !frame.frame) {
        return;
    }
    display_->renderFrame(reinterpret_cast<const uint8_t*>(frame.frame), frame.frame->width, frame.frame->height);
}

void SdlVideoRenderer::present() {
    if (display_) {
        display_->present();
    }
}

void SdlVideoRenderer::clear() {
    if (display_) {
        display_->clear();
    }
}

void SdlVideoRenderer::handleEvents() {
    if (display_) {
        display_->handleEvents();
    }
}

bool SdlVideoRenderer::shouldQuit() const {
    return display_ ? display_->shouldQuit() : false;
}

bool SdlVideoRenderer::consumeTogglePauseRequest() {
    return display_ ? display_->consumeTogglePauseRequest() : false;
}

bool SdlVideoRenderer::consumeSeekRequest(double& normalized_position) {
    return display_ ? display_->consumeSeekRequest(normalized_position) : false;
}

bool SdlVideoRenderer::consumeSeekDeltaRequest(double& delta_seconds) {
    return display_ ? display_->consumeSeekDeltaRequest(delta_seconds) : false;
}

bool SdlVideoRenderer::consumeVolumeChangeRequest(float& volume) {
    return display_ ? display_->consumeVolumeChangeRequest(volume) : false;
}

bool SdlVideoRenderer::consumeSpeedChangeRequest(double& speed_delta) {
    return display_ ? display_->consumeSpeedChangeRequest(speed_delta) : false;
}

bool SdlVideoRenderer::consumeResetSpeedRequest() {
    return display_ ? display_->consumeResetSpeedRequest() : false;
}

bool SdlVideoRenderer::consumeNextItemRequest() {
    return display_ ? display_->consumeNextItemRequest() : false;
}

bool SdlVideoRenderer::consumePreviousItemRequest() {
    return display_ ? display_->consumePreviousItemRequest() : false;
}

void SdlVideoRenderer::setOverlayState(double position, double duration, float volume, bool paused) {
    if (display_) {
        display_->setOverlayState(position, duration, volume, paused);
    }
}

}  // namespace vp::render
