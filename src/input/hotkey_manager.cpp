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

constexpr std::array<ActionPair, 33> kActionKeyPairs{{
    {PlayerAction::PlayPause, "play_pause"},
    {PlayerAction::SeekBackward, "seek_backward"},
    {PlayerAction::SeekForward, "seek_forward"},
    {PlayerAction::VolumeUp, "volume_up"},
    {PlayerAction::VolumeDown, "volume_down"},
    {PlayerAction::ToggleMute, "toggle_mute"},
    {PlayerAction::SpeedDown, "speed_down"},
    {PlayerAction::SpeedUp, "speed_up"},
    {PlayerAction::SpeedReset, "speed_reset"},
    {PlayerAction::SetABRepeatStart, "ab_repeat_start"},
    {PlayerAction::SetABRepeatEnd, "ab_repeat_end"},
    {PlayerAction::ClearABRepeat, "ab_repeat_clear"},
    {PlayerAction::TakeScreenshot, "take_screenshot"},
    {PlayerAction::StepFrameBackward, "step_frame_backward"},
    {PlayerAction::StepFrameForward, "step_frame_forward"},
    {PlayerAction::SubtitleDelayDown, "subtitle_delay_down"},
    {PlayerAction::SubtitleDelayUp, "subtitle_delay_up"},
    {PlayerAction::SeekTo10Percent, "seek_to_10_percent"},
    {PlayerAction::SeekTo20Percent, "seek_to_20_percent"},
    {PlayerAction::SeekTo30Percent, "seek_to_30_percent"},
    {PlayerAction::SeekTo40Percent, "seek_to_40_percent"},
    {PlayerAction::SeekTo50Percent, "seek_to_50_percent"},
    {PlayerAction::SeekTo60Percent, "seek_to_60_percent"},
    {PlayerAction::SeekTo70Percent, "seek_to_70_percent"},
    {PlayerAction::SeekTo80Percent, "seek_to_80_percent"},
    {PlayerAction::SeekTo90Percent, "seek_to_90_percent"},
    {PlayerAction::PreviousChapter, "previous_chapter"},
    {PlayerAction::NextChapter, "next_chapter"},
    {PlayerAction::PreviousItem, "previous_item"},
    {PlayerAction::NextItem, "next_item"},
    {PlayerAction::ToggleSubtitle, "toggle_subtitle"},
    {PlayerAction::ToggleFullscreen, "toggle_fullscreen"},
    {PlayerAction::Quit, "quit"},
}};

/// 将按键 token 统一转为大写，便于配置解析与序列化保持一致。
std::string toUpperAscii(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::toupper(ch));
    });
    return value;
}

/// 解析内建命名按键 token，覆盖方向键、翻页键和常用符号键。
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
    if (upper_token == "COMMA" || upper_token == ",") {
        return SDLK_COMMA;
    }
    if (upper_token == "PERIOD" || upper_token == "DOT" || upper_token == ".") {
        return SDLK_PERIOD;
    }
    return std::nullopt;
}

}  // namespace

HotkeyManager::HotkeyManager() {
    resetToDefaults();
}

/// 重建默认热键表，作为配置缺省值和回退方案。
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
    bind(PlayerAction::SetABRepeatStart, 'a');
    bind(PlayerAction::SetABRepeatEnd, 'b');
    bind(PlayerAction::ClearABRepeat, 'c');
    bind(PlayerAction::TakeScreenshot, 's');
    bind(PlayerAction::StepFrameBackward, SDLK_COMMA);
    bind(PlayerAction::StepFrameForward, SDLK_PERIOD);
    bind(PlayerAction::SubtitleDelayDown, 'j');
    bind(PlayerAction::SubtitleDelayUp, 'k');
    bind(PlayerAction::SeekTo10Percent, '1');
    bind(PlayerAction::SeekTo20Percent, '2');
    bind(PlayerAction::SeekTo30Percent, '3');
    bind(PlayerAction::SeekTo40Percent, '4');
    bind(PlayerAction::SeekTo50Percent, '5');
    bind(PlayerAction::SeekTo60Percent, '6');
    bind(PlayerAction::SeekTo70Percent, '7');
    bind(PlayerAction::SeekTo80Percent, '8');
    bind(PlayerAction::SeekTo90Percent, '9');
    bind(PlayerAction::PreviousChapter, SDLK_HOME);
    bind(PlayerAction::NextChapter, SDLK_END);
    bind(PlayerAction::PreviousItem, SDLK_PAGEUP);
    bind(PlayerAction::NextItem, SDLK_PAGEDOWN);
    bind(PlayerAction::ToggleSubtitle, 'v');
    bind(PlayerAction::ToggleFullscreen, 'f');
    bind(PlayerAction::Quit, 'q');
}

/// 为指定动作绑定一个按键码；重复绑定会覆盖旧值。
void HotkeyManager::bind(PlayerAction action, int key_code) {
    action_to_key_[action] = key_code;
}

/// 移除指定动作的按键绑定。
void HotkeyManager::unbind(PlayerAction action) {
    action_to_key_.erase(action);
}

/// 按按键码查找对应动作；没有绑定时返回空。
std::optional<PlayerAction> HotkeyManager::actionForKey(int key_code) const {
    for (const auto& [action, key] : action_to_key_) {
        if (key == key_code) {
            return action;
        }
    }
    return std::nullopt;
}

/// 按动作查找绑定的按键码；未绑定时返回空。
std::optional<int> HotkeyManager::keyForAction(PlayerAction action) const {
    auto it = action_to_key_.find(action);
    if (it == action_to_key_.end()) {
        return std::nullopt;
    }
    return it->second;
}

/// 返回当前全部动作绑定表的只读视图。
const std::unordered_map<PlayerAction, int>& HotkeyManager::bindings() const {
    return action_to_key_;
}

/// 扫描当前绑定表，找出多个动作共用同一按键的冲突。
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

/// 判断当前绑定表是否存在按键冲突。
bool HotkeyManager::hasConflicts() const {
    return !findConflicts().empty();
}

/// 返回项目支持的全部动作枚举列表。
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

/// 将动作枚举映射为配置文件中的稳定键名。
std::string HotkeyManager::actionConfigKey(PlayerAction action) {
    for (const auto& pair : kActionKeyPairs) {
        if (pair.first == action) {
            return pair.second;
        }
    }
    return "unknown";
}

/// 将配置键名反解为动作枚举；未知键名返回空。
std::optional<PlayerAction> HotkeyManager::actionFromConfigKey(const std::string& key) {
    for (const auto& pair : kActionKeyPairs) {
        if (key == pair.second) {
            return pair.first;
        }
    }
    return std::nullopt;
}

/// 将 SDL 按键码序列化为配置文件可读 token。
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
    case SDLK_COMMA:
        return "COMMA";
    case SDLK_PERIOD:
        return "PERIOD";
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

/// 将配置 token 反解为 SDL 按键码。
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

