#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace vp::input {

/// 播放器支持的标准化用户动作。
enum class PlayerAction {
    PlayPause,
    SeekBackward,
    SeekForward,
    VolumeUp,
    VolumeDown,
    ToggleMute,
    SpeedDown,
    SpeedUp,
    SpeedReset,
    SetABRepeatStart,
    SetABRepeatEnd,
    ClearABRepeat,
    TakeScreenshot,
    StepFrameBackward,
    StepFrameForward,
    SubtitleDelayDown,
    SubtitleDelayUp,
    SeekTo10Percent,
    SeekTo20Percent,
    SeekTo30Percent,
    SeekTo40Percent,
    SeekTo50Percent,
    SeekTo60Percent,
    SeekTo70Percent,
    SeekTo80Percent,
    SeekTo90Percent,
    PreviousChapter,
    NextChapter,
    PreviousItem,
    NextItem,
    ToggleSubtitle,
    ToggleFullscreen,
    Quit
};

/// 热键绑定管理器；负责动作与按键的双向查询、序列化与冲突检查。
class HotkeyManager {
public:
    /// 创建热键管理器并加载默认绑定。
    HotkeyManager();

    /// 绑定动作到指定按键码；若动作已有绑定则覆盖。
    void bind(PlayerAction action, int key_code);
    /// 解除指定动作的按键绑定。
    void unbind(PlayerAction action);
    /// 恢复内置默认热键集合。
    void resetToDefaults();

    /// 由按键码反查动作；未命中时返回空。
    std::optional<PlayerAction> actionForKey(int key_code) const;
    /// 由动作反查按键码；未绑定时返回空。
    std::optional<int> keyForAction(PlayerAction action) const;
    /// 返回当前动作到按键码的绑定表视图。
    const std::unordered_map<PlayerAction, int>& bindings() const;
    /// 找出多个动作绑定到同一按键的冲突对。
    std::vector<std::pair<PlayerAction, PlayerAction>> findConflicts() const;
    /// 返回当前是否存在热键冲突。
    bool hasConflicts() const;

    /// 返回全部支持的动作列表，适合配置页枚举展示。
    static const std::vector<PlayerAction>& allActions();
    /// 将动作转换为配置文件键名。
    static std::string actionConfigKey(PlayerAction action);
    /// 由配置文件键名反查动作。
    static std::optional<PlayerAction> actionFromConfigKey(const std::string& key);
    /// 将 SDL 按键码序列化为文本 token。
    static std::string keyCodeToToken(int key_code);
    /// 将文本 token 解析为 SDL 按键码。
    static std::optional<int> keyCodeFromToken(const std::string& token);

private:
    std::unordered_map<PlayerAction, int> action_to_key_;
};

}  // namespace vp::input