#include "render/d3d11_video_renderer.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>

#include "logger.h"

namespace vp::render {

namespace {
/// 读取环境变量，用于覆盖 SDL 的 D3D11 驱动提示。
std::string readEnvVar(const char* key) {
    if (!key || key[0] == '\0') {
        return {};
    }
#if defined(_WIN32)
    char* value = nullptr;
    size_t len = 0;
    if (_dupenv_s(&value, &len, key) != 0 || !value) {
        return {};
    }
    std::string result(value);
    std::free(value);
    return result;
#else
    const char* value = std::getenv(key);
    return value ? std::string(value) : std::string{};
#endif
}
}

D3D11VideoRenderer::D3D11VideoRenderer() = default;

D3D11VideoRenderer::~D3D11VideoRenderer() {
    close();
}

/// 初始化 D3D11 偏好的 SDL 渲染驱动，并校验实际后端是否匹配。
bool D3D11VideoRenderer::init(const VideoRendererConfig& config) {
    close();
    display_ = std::make_unique<Display>();

    std::string preferred_driver = "direct3d11";
    const std::string forced_driver = readEnvVar("MVP_D3D11_DRIVER_HINT");
    if (!forced_driver.empty()) {
        preferred_driver = forced_driver;
    }
    display_->setPreferredRendererDriver(preferred_driver);
    if (!display_->init(config.width, config.height, config.title)) {
        return false;
    }

    std::string driver_name = display_->currentRendererDriver();
    std::transform(driver_name.begin(), driver_name.end(), driver_name.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });

    const bool is_d3d11 = driver_name.find("direct3d11") != std::string::npos ||
                          driver_name.find("d3d11") != std::string::npos;
    if (!is_d3d11) {
        LOG_WARNING("D3D11 renderer init requested, but SDL backend is '" << driver_name
                    << "', fallback to SoftwareSDL renderer");
        close();
        return false;
    }

    LOG_INFO("D3D11 renderer initialized with SDL backend: " << driver_name);
    return true;
}

/// 关闭底层 `Display` 并释放 D3D11 偏好的 SDL 渲染路径资源。
void D3D11VideoRenderer::close() {
    if (display_) {
        display_->close();
        display_.reset();
    }
}

/// 把 `VideoFrame` 中的 `AVFrame` 交给 D3D11 偏好的显示封装。
void D3D11VideoRenderer::renderFrame(const core::VideoFrame& frame) {
    if (!display_ || !frame.valid || !frame.frame) {
        return;
    }
    display_->renderFrame(reinterpret_cast<const uint8_t*>(frame.frame), frame.frame->width, frame.frame->height);
}

/// 将呈现请求转发给 `Display`；实际提交仍在其渲染线程完成。
void D3D11VideoRenderer::present() {
    if (display_) {
        display_->present();
    }
}

/// 清空显示层缓存帧，避免后续继续显示过期视频内容。
void D3D11VideoRenderer::clear() {
    if (display_) {
        display_->clear();
    }
}

/// 转发窗口事件处理，让显示层生成 seek、暂停等控制请求。
void D3D11VideoRenderer::handleEvents() {
    if (display_) {
        display_->handleEvents();
    }
}

bool D3D11VideoRenderer::shouldQuit() const {
    return display_ ? display_->shouldQuit() : false;
}

bool D3D11VideoRenderer::consumeTogglePauseRequest() {
    return display_ ? display_->consumeTogglePauseRequest() : false;
}

bool D3D11VideoRenderer::consumeSeekRequest(double& normalized_position) {
    return display_ ? display_->consumeSeekRequest(normalized_position) : false;
}

bool D3D11VideoRenderer::consumeSeekDeltaRequest(double& delta_seconds) {
    return display_ ? display_->consumeSeekDeltaRequest(delta_seconds) : false;
}

bool D3D11VideoRenderer::consumeVolumeChangeRequest(float& volume) {
    return display_ ? display_->consumeVolumeChangeRequest(volume) : false;
}

bool D3D11VideoRenderer::consumeSpeedChangeRequest(double& speed_delta) {
    return display_ ? display_->consumeSpeedChangeRequest(speed_delta) : false;
}

bool D3D11VideoRenderer::consumeResetSpeedRequest() {
    return display_ ? display_->consumeResetSpeedRequest() : false;
}

bool D3D11VideoRenderer::consumeToggleSubtitleRequest() {
    return display_ ? display_->consumeToggleSubtitleRequest() : false;
}

bool D3D11VideoRenderer::consumeSetABRepeatStartRequest() {
    return display_ ? display_->consumeSetABRepeatStartRequest() : false;
}

bool D3D11VideoRenderer::consumeSetABRepeatEndRequest() {
    return display_ ? display_->consumeSetABRepeatEndRequest() : false;
}

bool D3D11VideoRenderer::consumeClearABRepeatRequest() {
    return display_ ? display_->consumeClearABRepeatRequest() : false;
}

bool D3D11VideoRenderer::consumeScreenshotRequest() {
    return display_ ? display_->consumeScreenshotRequest() : false;
}

bool D3D11VideoRenderer::consumeStepFrameBackwardRequest() {
    return display_ ? display_->consumeStepFrameBackwardRequest() : false;
}

bool D3D11VideoRenderer::consumeStepFrameForwardRequest() {
    return display_ ? display_->consumeStepFrameForwardRequest() : false;
}

bool D3D11VideoRenderer::consumeSubtitleDelayChangeRequest(double& delta_seconds) {
    return display_ ? display_->consumeSubtitleDelayChangeRequest(delta_seconds) : false;
}

bool D3D11VideoRenderer::consumeAudioDelayChangeRequest(double& delta_seconds) {
    return display_ ? display_->consumeAudioDelayChangeRequest(delta_seconds) : false;
}

bool D3D11VideoRenderer::consumeNextChapterRequest() {
    return display_ ? display_->consumeNextChapterRequest() : false;
}

bool D3D11VideoRenderer::consumePreviousChapterRequest() {
    return display_ ? display_->consumePreviousChapterRequest() : false;
}

bool D3D11VideoRenderer::consumeNextItemRequest() {
    return display_ ? display_->consumeNextItemRequest() : false;
}

bool D3D11VideoRenderer::consumePreviousItemRequest() {
    return display_ ? display_->consumePreviousItemRequest() : false;
}

/// 转发 OSD 状态到显示层，用于绘制进度条和暂停标记。
void D3D11VideoRenderer::setOverlayState(double position, double duration, float volume, bool paused) {
    if (display_) {
        display_->setOverlayState(position, duration, volume, paused);
    }
}

/// 转发字幕文本到显示层，供渲染线程叠加显示。
void D3D11VideoRenderer::setSubtitleText(const std::string& text) {
    if (display_) {
        display_->setSubtitleText(text);
    }
}

/// 覆盖显示层热键配置，保证 D3D11 路径和 SDL 路径行为一致。
void D3D11VideoRenderer::setHotkeyManager(const input::HotkeyManager& hotkey_manager) {
    if (display_) {
        display_->setHotkeyManager(hotkey_manager);
    }
}

const char* D3D11VideoRenderer::rendererBackendName() const {
    return "D3D11";
}

}  // namespace vp::render
