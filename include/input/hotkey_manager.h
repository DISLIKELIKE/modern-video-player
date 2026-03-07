#pragma once

#include <optional>
#include <unordered_map>

namespace vp::input {

enum class PlayerAction {
    PlayPause,
    Stop,
    SeekForward,
    SeekBackward,
    VolumeUp,
    VolumeDown,
    ToggleFullscreen,
    Quit
};

class HotkeyManager {
public:
    HotkeyManager();

    void bind(PlayerAction action, int key_code);
    void unbind(PlayerAction action);

    std::optional<PlayerAction> actionForKey(int key_code) const;
    std::optional<int> keyForAction(PlayerAction action) const;

private:
    std::unordered_map<PlayerAction, int> action_to_key_;
};

}  // namespace vp::input

