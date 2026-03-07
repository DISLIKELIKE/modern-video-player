#include "input/hotkey_manager.h"

namespace vp::input {

HotkeyManager::HotkeyManager() {
    bind(PlayerAction::PlayPause, ' ');
    bind(PlayerAction::Stop, 'S');
    bind(PlayerAction::SeekForward, 'D');
    bind(PlayerAction::SeekBackward, 'A');
    bind(PlayerAction::VolumeUp, '=');
    bind(PlayerAction::VolumeDown, '-');
    bind(PlayerAction::ToggleFullscreen, 'F');
    bind(PlayerAction::Quit, 'Q');
}

void HotkeyManager::bind(PlayerAction action, int key_code) {
    action_to_key_[action] = key_code;
}

void HotkeyManager::unbind(PlayerAction action) {
    action_to_key_.erase(action);
}

std::optional<PlayerAction> HotkeyManager::actionForKey(int key_code) const {
    for (const auto& [action, key] : action_to_key_) {
        if (key == key_code) {
            return action;
        }
    }
    return std::nullopt;
}

std::optional<int> HotkeyManager::keyForAction(PlayerAction action) const {
    auto it = action_to_key_.find(action);
    if (it == action_to_key_.end()) {
        return std::nullopt;
    }
    return it->second;
}

}  // namespace vp::input

