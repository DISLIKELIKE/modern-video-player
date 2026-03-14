#include "render/opengl_video_renderer.h"

namespace vp::render {

/// OpenGL 渲染路径当前未实现，初始化固定返回 false。
bool OpenGLVideoRenderer::init(const VideoRendererConfig& config) {
    (void)config;
    return false;
}

/// 当前 OpenGL 路径未落地，关闭操作为 no-op。
void OpenGLVideoRenderer::close() {}

/// 当前 OpenGL 路径未落地，收到视频帧后直接忽略。
void OpenGLVideoRenderer::renderFrame(const core::VideoFrame& frame) {
    (void)frame;
}

void OpenGLVideoRenderer::present() {}

void OpenGLVideoRenderer::clear() {}

/// 当前 OpenGL 路径未落地，不维护独立事件处理逻辑。
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

bool OpenGLVideoRenderer::consumeSetABRepeatStartRequest() {
    return false;
}

bool OpenGLVideoRenderer::consumeSetABRepeatEndRequest() {
    return false;
}

bool OpenGLVideoRenderer::consumeClearABRepeatRequest() {
    return false;
}

bool OpenGLVideoRenderer::consumeScreenshotRequest() {
    return false;
}

bool OpenGLVideoRenderer::consumeStepFrameBackwardRequest() {
    return false;
}

bool OpenGLVideoRenderer::consumeStepFrameForwardRequest() {
    return false;
}

bool OpenGLVideoRenderer::consumeSubtitleDelayChangeRequest(double& delta_seconds) {
    (void)delta_seconds;
    return false;
}

bool OpenGLVideoRenderer::consumeAudioDelayChangeRequest(double& delta_seconds) {
    (void)delta_seconds;
    return false;
}

bool OpenGLVideoRenderer::consumeNextChapterRequest() {
    return false;
}

bool OpenGLVideoRenderer::consumePreviousChapterRequest() {
    return false;
}

bool OpenGLVideoRenderer::consumeNextItemRequest() {
    return false;
}

bool OpenGLVideoRenderer::consumePreviousItemRequest() {
    return false;
}

/// 当前 OpenGL 路径未落地，OSD 状态仅作接口占位。
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

const char* OpenGLVideoRenderer::rendererBackendName() const {
    return "OpenGL";
}

}  // namespace vp::render
