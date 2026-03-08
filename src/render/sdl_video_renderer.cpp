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

bool SdlVideoRenderer::consumeToggleSubtitleRequest() {
    return display_ ? display_->consumeToggleSubtitleRequest() : false;
}

bool SdlVideoRenderer::consumeSetABRepeatStartRequest() {
    return display_ ? display_->consumeSetABRepeatStartRequest() : false;
}

bool SdlVideoRenderer::consumeSetABRepeatEndRequest() {
    return display_ ? display_->consumeSetABRepeatEndRequest() : false;
}

bool SdlVideoRenderer::consumeClearABRepeatRequest() {
    return display_ ? display_->consumeClearABRepeatRequest() : false;
}

bool SdlVideoRenderer::consumeNextChapterRequest() {
    return display_ ? display_->consumeNextChapterRequest() : false;
}

bool SdlVideoRenderer::consumePreviousChapterRequest() {
    return display_ ? display_->consumePreviousChapterRequest() : false;
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

void SdlVideoRenderer::setSubtitleText(const std::string& text) {
    if (display_) {
        display_->setSubtitleText(text);
    }
}

void SdlVideoRenderer::setHotkeyManager(const input::HotkeyManager& hotkey_manager) {
    if (display_) {
        display_->setHotkeyManager(hotkey_manager);
    }
}

const char* SdlVideoRenderer::rendererBackendName() const {
    return "SoftwareSDL";
}

}  // namespace vp::render
