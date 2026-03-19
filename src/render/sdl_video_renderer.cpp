#include "render/sdl_video_renderer.h"

namespace vp::render {

SdlVideoRenderer::SdlVideoRenderer() = default;

SdlVideoRenderer::~SdlVideoRenderer() {
    close();
}

/// 鍒涘缓 `Display` 骞朵娇鐢?SDL 閫氱敤娓叉煋璺緞鍒濆鍖栫獥鍙ｃ€?
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

/// 鍏抽棴搴曞眰 `Display` 骞堕噴鏀?SDL 娓叉煋璺緞鎸佹湁鐨勭獥鍙ｈ祫婧愩€?
void SdlVideoRenderer::close() {
    if (display_) {
        display_->close();
        display_.reset();
    }
}

/// 鎶?`VideoFrame` 涓殑 `AVFrame` 浜ょ粰 `Display` 鍋氬紓姝ユ樉绀恒€?
void SdlVideoRenderer::renderFrame(const core::VideoFrame& frame) {
    if (!display_ || !frame.valid || !frame.frame) {
        return;
    }
    display_->renderFrame(reinterpret_cast<const uint8_t*>(frame.frame), frame.frame->width, frame.frame->height);
}

/// 灏嗗憟鐜拌姹傝浆鍙戠粰 `Display`锛涚湡姝ｆ樉绀轰粛鐢卞叾鍐呴儴娓叉煋绾跨▼椹卞姩銆?
void SdlVideoRenderer::present() {
    if (display_) {
        display_->present();
    }
}

/// 娓呯┖鏄剧ず灞傜紦瀛樺抚锛岄伩鍏嶅仠姝㈡垨 seek 鍚庢畫鐣欐棫鐢婚潰銆?
void SdlVideoRenderer::clear() {
    if (display_) {
        display_->clear();
    }
}

/// 杞彂 SDL 浜嬩欢杞锛岃 `Display` 鐢熸垚涓€娆℃€ф挱鏀炬帶鍒惰姹傘€?
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

/// 杞彂 OSD 鐘舵€佸埌鏄剧ず灞傦紝鐢ㄤ簬缁樺埗杩涘害銆侀煶閲忓拰鏆傚仠鏍囪銆?
void SdlVideoRenderer::setOverlayState(double position, double duration, float volume, bool paused) {
    if (display_) {
        display_->setOverlayState(position, duration, volume, paused);
    }
}

/// 杞彂褰撳墠瀛楀箷鏂囨湰鍒版樉绀哄眰锛屼緵娓叉煋绾跨▼鍙犲姞銆?
void SdlVideoRenderer::setSubtitleText(const std::string& text) {
    if (display_) {
        display_->setSubtitleText(text);
    }
}

/// 瑕嗙洊鏄剧ず灞傜儹閿厤缃紝浣跨獥鍙ｄ簨浠惰В閲婁笌鎾斁鍣ㄨ缃繚鎸佷竴鑷淬€?
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

const char* SdlVideoRenderer::rendererBackendName() const {
    return "SoftwareSDL";
}

}  // namespace vp::render