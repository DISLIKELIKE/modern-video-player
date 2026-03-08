#include "render/opengl_video_renderer.h"

namespace vp::render {

bool OpenGLVideoRenderer::init(const VideoRendererConfig& config) {
    (void)config;
    return false;
}

void OpenGLVideoRenderer::close() {}

void OpenGLVideoRenderer::renderFrame(const core::VideoFrame& frame) {
    (void)frame;
}

void OpenGLVideoRenderer::present() {}

void OpenGLVideoRenderer::clear() {}

void OpenGLVideoRenderer::handleEvents() {}

bool OpenGLVideoRenderer::shouldQuit() const {
    return false;
}

bool OpenGLVideoRenderer::consumeTogglePauseRequest() {
    return false;
}

bool OpenGLVideoRenderer::consumeSeekRequest(double& normalized_position) {
    (void)normalized_position;
    return false;
}

bool OpenGLVideoRenderer::consumeSeekDeltaRequest(double& delta_seconds) {
    (void)delta_seconds;
    return false;
}

bool OpenGLVideoRenderer::consumeVolumeChangeRequest(float& volume) {
    (void)volume;
    return false;
}

bool OpenGLVideoRenderer::consumeSpeedChangeRequest(double& speed_delta) {
    (void)speed_delta;
    return false;
}

bool OpenGLVideoRenderer::consumeResetSpeedRequest() {
    return false;
}

bool OpenGLVideoRenderer::consumeToggleSubtitleRequest() {
    return false;
}

bool OpenGLVideoRenderer::consumeNextItemRequest() {
    return false;
}

bool OpenGLVideoRenderer::consumePreviousItemRequest() {
    return false;
}

void OpenGLVideoRenderer::setOverlayState(double position, double duration, float volume, bool paused) {
    (void)position;
    (void)duration;
    (void)volume;
    (void)paused;
}

void OpenGLVideoRenderer::setSubtitleText(const std::string& text) {
    (void)text;
}

void OpenGLVideoRenderer::setHotkeyManager(const input::HotkeyManager& hotkey_manager) {
    (void)hotkey_manager;
}

}  // namespace vp::render
