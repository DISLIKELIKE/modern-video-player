#include "input/hotkey_manager.h"

#if __has_include(<SDL2/SDL.h>)
#include <SDL2/SDL.h>
#elif __has_include(<SDL.h>)
#include <SDL.h>
#else
#error "SDL2 headers not found"
#endif

#include <algorithm>
#include <array>
#include <cctype>
#include <unordered_map>

namespace vp::input {

namespace {

using ActionPair = std::pair<PlayerAction, const char*>;

constexpr std::array<ActionPair, 14> kActionKeyPairs{{
    {PlayerAction::PlayPause, "play_pause"},
    {PlayerAction::SeekBackward, "seek_backward"},
    {PlayerAction::SeekForward, "seek_forward"},
    {PlayerAction::VolumeUp, "volume_up"},
    {PlayerAction::VolumeDown, "volume_down"},
    {PlayerAction::ToggleMute, "toggle_mute"},
    {PlayerAction::SpeedDown, "speed_down"},
    {PlayerAction::SpeedUp, "speed_up"},
    {PlayerAction::SpeedReset, "speed_reset"},
    {PlayerAction::PreviousItem, "previous_item"},
    {PlayerAction::NextItem, "next_item"},
    {PlayerAction::ToggleSubtitle, "toggle_subtitle"},
    {PlayerAction::ToggleFullscreen, "toggle_fullscreen"},
    {PlayerAction::Quit, "quit"},
}};

std::string toUpperAscii(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::toupper(ch));
    });
    return value;
}

std::optional<int> parseNamedKeyToken(const std::string& upper_token) {
    if (upper_token == "SPACE") {
        return SDLK_SPACE;
    }
    if (upper_token == "LEFT") {
        return SDLK_LEFT;
    }
    if (upper_token == "RIGHT") {
        return SDLK_RIGHT;
    }
    if (upper_token == "UP") {
        return SDLK_UP;
    }
    if (upper_token == "DOWN") {
        return SDLK_DOWN;
    }
    if (upper_token == "PAGEUP") {
        return SDLK_PAGEUP;
    }
    if (upper_token == "PAGEDOWN") {
        return SDLK_PAGEDOWN;
    }
    if (upper_token == "LBRACKET" || upper_token == "[") {
        return SDLK_LEFTBRACKET;
    }
    if (upper_token == "RBRACKET" || upper_token == "]") {
        return SDLK_RIGHTBRACKET;
    }
    if (upper_token == "EQUALS" || upper_token == "PLUS" || upper_token == "=") {
        return SDLK_EQUALS;
    }
    if (upper_token == "MINUS" || upper_token == "HYPHEN" || upper_token == "-") {
        return SDLK_MINUS;
    }
    if (upper_token == "ESC" || upper_token == "ESCAPE") {
        return SDLK_ESCAPE;
    }
    if (upper_token == "ENTER") {
        return SDLK_RETURN;
    }
    return std::nullopt;
}

}  // namespace

HotkeyManager::HotkeyManager() {
    resetToDefaults();
}

void HotkeyManager::resetToDefaults() {
    action_to_key_.clear();
    bind(PlayerAction::PlayPause, ' ');
    bind(PlayerAction::SeekBackward, SDLK_LEFT);
    bind(PlayerAction::SeekForward, SDLK_RIGHT);
    bind(PlayerAction::VolumeUp, SDLK_UP);
    bind(PlayerAction::VolumeDown, SDLK_DOWN);
    bind(PlayerAction::ToggleMute, 'm');
    bind(PlayerAction::SpeedDown, SDLK_LEFTBRACKET);
    bind(PlayerAction::SpeedUp, SDLK_RIGHTBRACKET);
    bind(PlayerAction::SpeedReset, 'r');
    bind(PlayerAction::PreviousItem, SDLK_PAGEUP);
    bind(PlayerAction::NextItem, SDLK_PAGEDOWN);
    bind(PlayerAction::ToggleSubtitle, 'v');
    bind(PlayerAction::ToggleFullscreen, 'f');
    bind(PlayerAction::Quit, 'q');
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

const std::unordered_map<PlayerAction, int>& HotkeyManager::bindings() const {
    return action_to_key_;
}

std::vector<std::pair<PlayerAction, PlayerAction>> HotkeyManager::findConflicts() const {
    std::unordered_map<int, PlayerAction> key_owner;
    std::vector<std::pair<PlayerAction, PlayerAction>> conflicts;

    for (const PlayerAction action : allActions()) {
        const auto key_code = keyForAction(action);
        if (!key_code) {
            continue;
        }

        const auto [it, inserted] = key_owner.emplace(*key_code, action);
        if (!inserted) {
            conflicts.emplace_back(it->second, action);
        }
    }
    return conflicts;
}

bool HotkeyManager::hasConflicts() const {
    return !findConflicts().empty();
}

const std::vector<PlayerAction>& HotkeyManager::allActions() {
    static const std::vector<PlayerAction> actions = []() {
        std::vector<PlayerAction> values;
        values.reserve(kActionKeyPairs.size());
        for (const auto& pair : kActionKeyPairs) {
            values.push_back(pair.first);
        }
        return values;
    }();
    return actions;
}

std::string HotkeyManager::actionConfigKey(PlayerAction action) {
    for (const auto& pair : kActionKeyPairs) {
        if (pair.first == action) {
            return pair.second;
        }
    }
    return "unknown";
}

std::optional<PlayerAction> HotkeyManager::actionFromConfigKey(const std::string& key) {
    for (const auto& pair : kActionKeyPairs) {
        if (key == pair.second) {
            return pair.first;
        }
    }
    return std::nullopt;
}

std::string HotkeyManager::keyCodeToToken(int key_code) {
    switch (key_code) {
    case SDLK_SPACE:
        return "SPACE";
    case SDLK_LEFT:
        return "LEFT";
    case SDLK_RIGHT:
        return "RIGHT";
    case SDLK_UP:
        return "UP";
    case SDLK_DOWN:
        return "DOWN";
    case SDLK_PAGEUP:
        return "PAGEUP";
    case SDLK_PAGEDOWN:
        return "PAGEDOWN";
    case SDLK_LEFTBRACKET:
        return "LBRACKET";
    case SDLK_RIGHTBRACKET:
        return "RBRACKET";
    case SDLK_EQUALS:
        return "EQUALS";
    case SDLK_MINUS:
        return "MINUS";
    case SDLK_ESCAPE:
        return "ESC";
    case SDLK_RETURN:
        return "ENTER";
    default:
        break;
    }

    if ((key_code >= 'a' && key_code <= 'z') || (key_code >= '0' && key_code <= '9')) {
        std::string token(1, static_cast<char>(std::toupper(static_cast<unsigned char>(key_code))));
        return token;
    }

    const char* key_name = SDL_GetKeyName(key_code);
    if (!key_name || key_name[0] == '\0') {
        return "UNKNOWN";
    }
    return toUpperAscii(key_name);
}

std::optional<int> HotkeyManager::keyCodeFromToken(const std::string& token) {
    if (token.empty()) {
        return std::nullopt;
    }

    const std::string upper_token = toUpperAscii(token);
    if (const auto named_key = parseNamedKeyToken(upper_token)) {
        return named_key;
    }

    if (upper_token.size() == 1) {
        const unsigned char ch = static_cast<unsigned char>(upper_token[0]);
        if (std::isalnum(ch) != 0) {
            return static_cast<int>(std::tolower(ch));
        }
    }

    const int key_code = SDL_GetKeyFromName(upper_token.c_str());
    if (key_code == SDLK_UNKNOWN) {
        return std::nullopt;
    }
    return key_code;
}

}  // namespace vp::input

