#pragma once

#include <memory>
#include <string>

#include "core/frame.h"
#include "input/hotkey_manager.h"

namespace vp::render {

/// 视频渲染后端类型。
enum class VideoRendererType {
    /// 自动选择平台默认后端。
    Auto,
    /// 使用 SDL 软件/通用渲染路径。
    SoftwareSDL,
    /// 使用 D3D11 渲染路径。
    D3D11,
    /// 使用 OpenGL 渲染路径。
    OpenGL
};

/// 渲染器初始化参数。
struct VideoRendererConfig {
    int width{0};
    int height{0};
    std::string title{"Video Player"};
};

/// 视频渲染抽象接口；既负责显示视频，也负责承载窗口事件与 UI 请求。
class IVideoRenderer {
public:
    virtual ~IVideoRenderer() = default;

    /// 初始化窗口和渲染后端。
    virtual bool init(const VideoRendererConfig& config) = 0;
    /// 释放渲染后端资源。
    virtual void close() = 0;

    /// 提交一帧待渲染的视频帧。
    virtual void renderFrame(const core::VideoFrame& frame) = 0;
    /// 立即呈现已提交内容。
    virtual void present() = 0;
    /// 清空当前显示内容。
    virtual void clear() = 0;

    /// 处理窗口事件、键盘和鼠标输入。
    virtual void handleEvents() = 0;
    /// 返回窗口是否请求退出。
    virtual bool shouldQuit() const = 0;

    /// 消费一次暂停/继续请求。
    virtual bool consumeTogglePauseRequest() = 0;
    /// 消费一次按比例 seek 请求。
    virtual bool consumeSeekRequest(double& normalized_position) = 0;
    /// 消费一次按秒增量 seek 请求。
    virtual bool consumeSeekDeltaRequest(double& delta_seconds) = 0;
    /// 消费一次音量调整请求。
    virtual bool consumeVolumeChangeRequest(float& volume) = 0;
    /// 消费一次倍速增量调整请求。
    virtual bool consumeSpeedChangeRequest(double& speed_delta) = 0;
    /// 消费一次倍速重置请求。
    virtual bool consumeResetSpeedRequest() = 0;
    /// 消费一次字幕显示切换请求。
    virtual bool consumeToggleSubtitleRequest() = 0;
    /// 消费一次 A-B 起点设置请求。
    virtual bool consumeSetABRepeatStartRequest() = 0;
    /// 消费一次 A-B 终点设置请求。
    virtual bool consumeSetABRepeatEndRequest() = 0;
    /// 消费一次清除 A-B 循环请求。
    virtual bool consumeClearABRepeatRequest() = 0;
    /// 消费一次截图请求。
    virtual bool consumeScreenshotRequest() = 0;
    /// 消费一次向后逐帧请求。
    virtual bool consumeStepFrameBackwardRequest() = 0;
    /// 消费一次向前逐帧请求。
    virtual bool consumeStepFrameForwardRequest() = 0;
    /// 消费一次字幕延迟调整请求。
    virtual bool consumeSubtitleDelayChangeRequest(double& delta_seconds) = 0;
    /// 消费一次音频延迟调整请求。
    virtual bool consumeAudioDelayChangeRequest(double& delta_seconds) = 0;
    /// 消费一次下一章节请求。
    virtual bool consumeNextChapterRequest() = 0;
    /// 消费一次上一章节请求。
    virtual bool consumePreviousChapterRequest() = 0;
    /// 消费一次下一播放项请求。
    virtual bool consumeNextItemRequest() = 0;
    /// 消费一次上一播放项请求。
    virtual bool consumePreviousItemRequest() = 0;
    /// 更新底部叠加层状态。
    virtual void setOverlayState(double position, double duration, float volume, bool paused) = 0;
    /// 更新当前应显示的字幕文本。
    virtual void setSubtitleText(const std::string& text) = 0;
    /// 更新热键绑定配置。
    virtual void setHotkeyManager(const input::HotkeyManager& hotkey_manager) = 0;
    /// 返回实际渲染后端名称。
    virtual const char* rendererBackendName() const = 0;
};

using VideoRendererPtr = std::unique_ptr<IVideoRenderer>;

}  // namespace vp::render