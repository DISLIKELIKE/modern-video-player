#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace vp::input {

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
    PreviousChapter,
    NextChapter,
    PreviousItem,
    NextItem,
    ToggleSubtitle,
    ToggleFullscreen,
    Quit
};

class HotkeyManager {
public:
    HotkeyManager();

    void bind(PlayerAction action, int key_code);
    void unbind(PlayerAction action);
    void resetToDefaults();

    std::optional<PlayerAction> actionForKey(int key_code) const;
    std::optional<int> keyForAction(PlayerAction action) const;
    const std::unordered_map<PlayerAction, int>& bindings() const;
    std::vector<std::pair<PlayerAction, PlayerAction>> findConflicts() const;
    bool hasConflicts() const;

    static const std::vector<PlayerAction>& allActions();
    static std::string actionConfigKey(PlayerAction action);
    static std::optional<PlayerAction> actionFromConfigKey(const std::string& key);
    static std::string keyCodeToToken(int key_code);
    static std::optional<int> keyCodeFromToken(const std::string& token);

private:
    std::unordered_map<PlayerAction, int> action_to_key_;
};

}  // namespace vp::input

