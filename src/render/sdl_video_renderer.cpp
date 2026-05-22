#include "render/sdl_video_renderer.h"

namespace vp::render {

SdlVideoRenderer::SdlVideoRenderer() = default;

SdlVideoRenderer::~SdlVideoRenderer() {
    close();
}

bool SdlVideoRenderer::init(const VideoRendererConfig& config) {
    close();
    display_ = std::make_unique<Display>();
    if (!display_->init(config.width, config.height, config.title)) {
        display_.reset();
        return false;
    }
    display_->resetFrameCopyStats();
    return true;
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

bool SdlVideoRenderer::consumeScreenshotRequest() {
    return display_ ? display_->consumeScreenshotRequest() : false;
}

bool SdlVideoRenderer::consumeStepFrameBackwardRequest() {
    return display_ ? display_->consumeStepFrameBackwardRequest() : false;
}

bool SdlVideoRenderer::consumeStepFrameForwardRequest() {
    return display_ ? display_->consumeStepFrameForwardRequest() : false;
}

bool SdlVideoRenderer::consumePreviousSubtitleTrackRequest() {
    return false;
}

bool SdlVideoRenderer::consumeNextSubtitleTrackRequest() {
    return false;
}

bool SdlVideoRenderer::consumeSubtitleDelayChangeRequest(double& delta_seconds) {
    return display_ ? display_->consumeSubtitleDelayChangeRequest(delta_seconds) : false;
}

bool SdlVideoRenderer::consumeAudioDelayChangeRequest(double& delta_seconds) {
    return display_ ? display_->consumeAudioDelayChangeRequest(delta_seconds) : false;
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

bool SdlVideoRenderer::consumeOpenFileRequest(std::string& path) {
    return display_ ? display_->consumeOpenFileRequest(path) : false;
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

RendererDiagnostics SdlVideoRenderer::getDiagnostics() const {
    if (!display_) {
        return {};
    }

    const Display::FrameCopyStats stats = display_->getFrameCopyStats();
    RendererDiagnostics diagnostics;
    diagnostics.display_copy_frames = stats.frames;
    diagnostics.display_copy_bytes = stats.bytes;
    diagnostics.display_copy_time_us_total = stats.time_us_total;
    diagnostics.display_copy_time_us_max = stats.time_us_max;
    return diagnostics;
}

void SdlVideoRenderer::resetDiagnostics() {
    if (display_) {
        display_->resetFrameCopyStats();
    }
}

bool SdlVideoRenderer::supportsDirectFrameFormat(AVPixelFormat format) const {
    return format == AV_PIX_FMT_YUV420P || format == AV_PIX_FMT_NV12;
}

const char* SdlVideoRenderer::rendererBackendName() const {
    return "SoftwareSDL";
}

}  // namespace vp::render
