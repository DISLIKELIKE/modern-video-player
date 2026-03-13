#pragma once

#include <atomic>
#include <memory>
#include <string>
#include <vector>

#include "core/player_core.h"
#include "subtitle/subtitle_parser.h"

namespace vp {

/// 面向 UI/应用层的播放器门面；负责转发控制命令并缓存用户态播放配置。
class VideoPlayer {
public:
    /// 创建播放器并注册底层状态、位置回调。
    VideoPlayer();
    /// 关闭底层播放链路并释放门面持有的状态。
    ~VideoPlayer();

    /// 打开媒体文件并恢复音量、倍速、硬解偏好、外挂字幕等配置。
    /// @param filename 媒体文件路径或当前进程可访问的输入 URI。
    /// @return 底层 `PlayerCore` 成功完成打开与初始化时返回 true。
    bool open(const std::string& filename);
    /// 关闭当前媒体并清理外挂字幕与状态缓存。
    void close();

    /// 切换到底层播放态。
    void play();
    /// 切换到底层暂停态。
    void pause();
    /// 停止播放并回到初始停止态。
    void stop();
    /// 按秒 seek 到目标时间点。
    /// @param timestamp 目标播放位置，单位秒；越界值由底层链路自行裁剪或拒绝。
    void seek(double timestamp);
    /// 跳到下一章节起点；不存在章节时返回 false。
    bool seekToNextChapter();
    /// 跳到上一章节起点；不存在章节时返回 false。
    bool seekToPreviousChapter();
    /// 返回已解析出的章节数。
    size_t chapterCount() const;
    /// 记录 A-B 循环起点；成功时返回 true。
    bool setABRepeatStart();
    /// 记录 A-B 循环终点；成功时返回 true。
    bool setABRepeatEnd();
    /// 清除当前 A-B 循环区间。
    void clearABRepeat();
    /// 返回是否已启用 A-B 循环。
    bool isABRepeatEnabled() const;
    /// 返回 A-B 循环起点秒数；未设置时返回负值。
    double abRepeatStart() const;
    /// 返回 A-B 循环终点秒数；未设置时返回负值。
    double abRepeatEnd() const;
    /// 请求对当前渲染帧截图。
    bool requestScreenshot();
    /// 取走最近一次截图路径；没有新截图时返回 false。
    /// @param path 成功时写出截图文件路径；失败时保持调用方可自行决定是否复用旧值。
    /// @return 取到新截图路径时返回 true。
    bool consumeLastScreenshotPath(std::string& path);
    /// 向后逐帧步进一帧；通常在暂停态使用。
    bool stepFrameBackward();
    /// 向前逐帧步进一帧；通常在暂停态使用。
    bool stepFrameForward();
    /// 将窗口事件和热键请求泵入底层显示模块。
    void pumpEvents();
    /// 消费一次退出请求。
    bool consumeQuitRequest();
    /// 消费一次“下一项播放内容”请求。
    bool consumeNextItemRequest();
    /// 消费一次“上一项播放内容”请求。
    bool consumePreviousItemRequest();

    /// 返回当前是否处于播放态缓存，适合 UI 线程轮询。
    bool isPlaying() const;
    /// 返回当前是否处于暂停态缓存。
    bool isPaused() const;
    /// 返回媒体总时长（秒）。
    double getDuration() const;
    /// 返回最近一次同步到门面的播放位置（秒）。
    double getCurrentTime() const;
    /// 返回播放信息快照。
    core::PlaybackInfo getInfo() const;
    /// 返回链路诊断计数快照，用于观察读包/解码/渲染健康度。
    core::DiagnosticsSnapshot getDiagnosticsSnapshot() const;

    /// 设置播放音量，范围为 [0, 1]。
    void setVolume(float volume);
    /// 返回当前音量配置。
    float getVolume() const;

    /// 设置播放速度，范围为 [0.5, 2.0]。
    void setPlaybackSpeed(double speed);
    /// 返回当前播放速度。
    double getPlaybackSpeed() const;
    /// 设置音频延迟，单位秒；正值表示更晚播放。
    void setAudioDelay(double delay_seconds);
    /// 返回当前音频延迟。
    double getAudioDelay() const;
    /// 设置字幕延迟，单位秒；正值表示更晚显示。
    void setSubtitleDelay(double delay_seconds);
    /// 返回当前字幕延迟。
    double getSubtitleDelay() const;
    /// 设置是否优先选择硬件解码后端。
    void setPreferHardwareDecode(bool prefer_hardware_decode);
    /// 返回当前硬解偏好配置。
    bool preferHardwareDecode() const;
    /// 返回当前视频渲染后端名称。
    std::string videoRendererBackendName() const;
    /// 返回当前视频解码后端名称。
    const char* videoDecoderBackendName() const;
    /// 覆盖当前热键绑定配置。
    void setHotkeyManager(const input::HotkeyManager& hotkey_manager);
    /// 返回当前生效的热键配置。
    const input::HotkeyManager& hotkeyManager() const;

    /// 加载外挂字幕文件并立刻同步到底层。
    /// @param subtitle_file 外挂字幕路径；当前实现仅对受支持格式返回成功。
    /// @return 文件存在、解析成功且已同步到底层时返回 true。
    bool loadExternalSubtitle(const std::string& subtitle_file);
    /// 清除当前外挂字幕及其来源路径。
    void clearExternalSubtitle();
    /// 返回是否已加载外挂字幕。
    bool hasExternalSubtitle() const;
    /// 返回外挂字幕源文件路径。
    const std::string& externalSubtitlePath() const;
    /// 返回外挂字幕条目数。
    size_t externalSubtitleCount() const;
    /// 显式开启或关闭字幕显示。
    void setSubtitleEnabled(bool enabled);
    /// 返回字幕显示总开关状态。
    bool isSubtitleEnabled() const;
    /// 翻转字幕显示状态并返回翻转后的结果。
    bool toggleSubtitleEnabled();

private:
    std::unique_ptr<core::PlayerCore> core_player_;
    std::atomic<bool> playing_{false};
    std::atomic<bool> paused_{false};
    std::atomic<double> current_time_{0.0};
    float volume_{1.0f};
    double playback_speed_{1.0};
    bool prefer_hardware_decode_{true};
    std::string subtitle_path_;
    std::vector<subtitle::SubtitleItem> subtitle_items_;
    bool subtitle_enabled_{true};
};

}  // namespace vp

