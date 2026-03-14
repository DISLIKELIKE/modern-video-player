#include "render/sdl_video_renderer.h"

namespace vp::render {

SdlVideoRenderer::SdlVideoRenderer() = default;

SdlVideoRenderer::~SdlVideoRenderer() {
    close();
}

/// 创建 `Display` 并使用 SDL 通用渲染路径初始化窗口。
bool SdlVideoRenderer::init(const VideoRendererConfig& config) {
    close();
    display_ = std::make_unique<Display>();
    return display_->init(config.width, config.height, config.title);
}

/// 关闭底层 `Display` 并释放 SDL 渲染路径持有的窗口资源。
void SdlVideoRenderer::close() {
    if (display_) {
        display_->close();
        display_.reset();
    }
}

/// 把 `VideoFrame` 中的 `AVFrame` 交给 `Display` 做异步显示。
void SdlVideoRenderer::renderFrame(const core::VideoFrame& frame) {
    if (!display_ || !frame.valid || !frame.frame) {
        return;
    }
    display_->renderFrame(reinterpret_cast<const uint8_t*>(frame.frame), frame.frame->width, frame.frame->height);
}

/// 将呈现请求转发给 `Display`；真正显示仍由其内部渲染线程驱动。
void SdlVideoRenderer::present() {
    if (display_) {
        display_->present();
    }
}

/// 清空显示层缓存帧，避免停止或 seek 后残留旧画面。
void SdlVideoRenderer::clear() {
    if (display_) {
        display_->clear();
    }
}

/// 转发 SDL 事件轮询，让 `Display` 生成一次性播放控制请求。
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

/// 转发 OSD 状态到显示层，用于绘制进度、音量和暂停标记。
void SdlVideoRenderer::setOverlayState(double position, double duration, float volume, bool paused) {
    if (display_) {
        display_->setOverlayState(position, duration, volume, paused);
    }
}

/// 转发当前字幕文本到显示层，供渲染线程叠加。
void SdlVideoRenderer::setSubtitleText(const std::string& text) {
    if (display_) {
        display_->setSubtitleText(text);
    }
}

/// 覆盖显示层热键配置，使窗口事件解释与播放器设置保持一致。
void SdlVideoRenderer::setHotkeyManager(const input::HotkeyManager& hotkey_manager) {
    if (display_) {
        display_->setHotkeyManager(hotkey_manager);
    }
}

const char* SdlVideoRenderer::rendererBackendName() const {
    return "SoftwareSDL";
}

}  // namespace vp::render
