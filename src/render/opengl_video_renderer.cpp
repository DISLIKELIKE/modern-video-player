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

bool OpenGLVideoRenderer::consumeVolumeChangeRequest(float& volume) {
    (void)volume;
    return false;
}

void OpenGLVideoRenderer::setOverlayState(double position, double duration, float volume, bool paused) {
    (void)position;
    (void)duration;
    (void)volume;
    (void)paused;
}

}  // namespace vp::render

