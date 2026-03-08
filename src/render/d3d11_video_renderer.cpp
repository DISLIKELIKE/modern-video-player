#include "render/d3d11_video_renderer.h"

namespace vp::render {

bool D3D11VideoRenderer::init(const VideoRendererConfig& config) {
    (void)config;
    return false;
}

void D3D11VideoRenderer::close() {}

void D3D11VideoRenderer::renderFrame(const core::VideoFrame& frame) {
    (void)frame;
}

void D3D11VideoRenderer::present() {}

void D3D11VideoRenderer::clear() {}

void D3D11VideoRenderer::handleEvents() {}

bool D3D11VideoRenderer::shouldQuit() const {
    return false;
}

bool D3D11VideoRenderer::consumeTogglePauseRequest() {
    return false;
}

bool D3D11VideoRenderer::consumeSeekRequest(double& normalized_position) {
    (void)normalized_position;
    return false;
}

bool D3D11VideoRenderer::consumeSeekDeltaRequest(double& delta_seconds) {
    (void)delta_seconds;
    return false;
}

bool D3D11VideoRenderer::consumeVolumeChangeRequest(float& volume) {
    (void)volume;
    return false;
}

bool D3D11VideoRenderer::consumeSpeedChangeRequest(double& speed_delta) {
    (void)speed_delta;
    return false;
}

bool D3D11VideoRenderer::consumeResetSpeedRequest() {
    return false;
}

bool D3D11VideoRenderer::consumeToggleSubtitleRequest() {
    return false;
}

bool D3D11VideoRenderer::consumeNextItemRequest() {
    return false;
}

bool D3D11VideoRenderer::consumePreviousItemRequest() {
    return false;
}

void D3D11VideoRenderer::setOverlayState(double position, double duration, float volume, bool paused) {
    (void)position;
    (void)duration;
    (void)volume;
    (void)paused;
}

void D3D11VideoRenderer::setSubtitleText(const std::string& text) {
    (void)text;
}

void D3D11VideoRenderer::setHotkeyManager(const input::HotkeyManager& hotkey_manager) {
    (void)hotkey_manager;
}

}  // namespace vp::render
