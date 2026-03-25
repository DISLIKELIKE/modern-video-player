#include "subtitle/ass_parser.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>

namespace vp::subtitle {

namespace {

struct ParsedAssText {
    std::string visible_text;
    SubtitleStyle cue_style;
    SubtitleStyleAnimation cue_animation;
    bool has_vector_drawing{false};
    int drawing_scale{1};
    std::string drawing_commands;
    std::vector<SubtitleTextRun> runs;
};

struct PendingKaraokeState {
    bool pending{false};
    SubtitleKaraokeMode mode{SubtitleKaraokeMode::None};
    int duration_centiseconds{0};
    int cursor_centiseconds{0};
};

struct DrawingModeState {
    int scale{0};
    bool enabled() const { return scale > 0; }
};

std::vector<std::string> splitAndTrim(const std::string& text);
bool parseInt(const std::string& text, int& value);
bool parseDouble(const std::string& text, double& value);
void parseOverrideBlock(const std::string& block,
                        const std::unordered_map<std::string, SubtitleStyle>& styles,
                        const SubtitleStyle& base_style,
                        SubtitleStyle& cue_style,
                        SubtitleStyle& current_style,
                        SubtitleStyleAnimation& cue_animation,
                        SubtitleStyleAnimation& current_animation,
                        PendingKaraokeState& karaoke_state,
                        DrawingModeState& drawing_mode,
                        bool allow_transform_tags = true);

std::string trimCopy(const std::string& text) {
    size_t start = 0;
    size_t end = text.size();
    while (start < end && std::isspace(static_cast<unsigned char>(text[start])) != 0) {
        ++start;
    }
    while (end > start && std::isspace(static_cast<unsigned char>(text[end - 1])) != 0) {
        --end;
    }
    return text.substr(start, end - start);
}

std::string toLowerAscii(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

std::string normalizeFieldName(const std::string& value) {
    std::string normalized;
    normalized.reserve(value.size());
    for (unsigned char ch : value) {
        if (std::isalnum(ch) != 0) {
            normalized.push_back(static_cast<char>(std::tolower(ch)));
        }
    }
    return normalized;
}

size_t countUtf16CodeUnits(const std::string& text) {
    size_t count = 0;
    for (size_t i = 0; i < text.size();) {
        const unsigned char ch = static_cast<unsigned char>(text[i]);
        uint32_t codepoint = 0;
        size_t sequence_length = 1;

        if ((ch & 0x80u) == 0) {
            codepoint = ch;
        } else if ((ch & 0xE0u) == 0xC0u && i + 1 < text.size()) {
            codepoint = (static_cast<uint32_t>(ch & 0x1Fu) << 6u) |
                        static_cast<uint32_t>(static_cast<unsigned char>(text[i + 1]) & 0x3Fu);
            sequence_length = 2;
        } else if ((ch & 0xF0u) == 0xE0u && i + 2 < text.size()) {
            codepoint = (static_cast<uint32_t>(ch & 0x0Fu) << 12u) |
                        (static_cast<uint32_t>(static_cast<unsigned char>(text[i + 1]) & 0x3Fu) << 6u) |
                        static_cast<uint32_t>(static_cast<unsigned char>(text[i + 2]) & 0x3Fu);
            sequence_length = 3;
        } else if ((ch & 0xF8u) == 0xF0u && i + 3 < text.size()) {
            codepoint = (static_cast<uint32_t>(ch & 0x07u) << 18u) |
                        (static_cast<uint32_t>(static_cast<unsigned char>(text[i + 1]) & 0x3Fu) << 12u) |
                        (static_cast<uint32_t>(static_cast<unsigned char>(text[i + 2]) & 0x3Fu) << 6u) |
                        static_cast<uint32_t>(static_cast<unsigned char>(text[i + 3]) & 0x3Fu);
            sequence_length = 4;
        } else {
            codepoint = 0xFFFDu;
        }

        count += codepoint > 0xFFFFu ? 2u : 1u;
        i += sequence_length;
    }
    return count;
}

size_t findMatchingParenthesis(const std::string& text, size_t open_index) {
    if (open_index >= text.size() || text[open_index] != '(') {
        return std::string::npos;
    }

    size_t depth = 1;
    for (size_t index = open_index + 1; index < text.size(); ++index) {
        if (text[index] == '(') {
            ++depth;
        } else if (text[index] == ')') {
            if (--depth == 0) {
                return index;
            }
        }
    }
    return std::string::npos;
}

std::vector<std::string> splitTopLevelCommaSeparated(const std::string& text) {
    std::vector<std::string> parts;
    size_t start = 0;
    size_t depth = 0;
    for (size_t index = 0; index < text.size(); ++index) {
        if (text[index] == '(') {
            ++depth;
        } else if (text[index] == ')') {
            if (depth > 0) {
                --depth;
            }
        } else if (text[index] == ',' && depth == 0) {
            parts.push_back(trimCopy(text.substr(start, index - start)));
            start = index + 1;
        }
    }
    parts.push_back(trimCopy(text.substr(start)));
    return parts;
}

bool startsWithIgnoreCase(const std::string& text, size_t offset, const char* token) {
    for (size_t i = 0; token[i] != '\0'; ++i) {
        if (offset + i >= text.size()) {
            return false;
        }
        if (std::tolower(static_cast<unsigned char>(text[offset + i])) !=
            std::tolower(static_cast<unsigned char>(token[i]))) {
            return false;
        }
    }
    return true;
}

std::string tryMatchSupportedTag(const std::string& block, size_t offset) {
    static const char* kSupportedTags[] = {
        "iclip",
        "clip",
        "alpha",
        "fade",
        "move",
        "org",
        "xbord",
        "ybord",
        "xshad",
        "yshad",
        "fscx",
        "fscy",
        "fsp",
        "fax",
        "fay",
        "frz",
        "frx",
        "fry",
        "fad",
        "kf",
        "ko",
        "bord",
        "shad",
        "pos",
        "fn",
        "fs",
        "fr",
        "an",
        "q",
        "p",
        "4c",
        "4a",
        "3c",
        "3a",
        "2c",
        "2a",
        "1c",
        "1a",
        "c",
        "a",
        "b",
        "i",
        "u",
        "s",
        "r",
        "t",
    };

    for (const char* tag : kSupportedTags) {
        if (startsWithIgnoreCase(block, offset, tag)) {
            return tag;
        }
    }
    return {};
}

SubtitleStyle makeDefaultAssStyle() {
    SubtitleStyle style;
    style.style_name = "Default";
    style.font_family = "Microsoft YaHei UI";
    style.font_size = 36.0;
    style.bold = false;
    style.italic = false;
    style.underline = false;
    style.strikeout = false;
    style.primary_color = SubtitleColor(255, 255, 255, 255);
    style.secondary_color = SubtitleColor(255, 255, 0, 255);
    style.outline_color = SubtitleColor(0, 0, 0, 255);
    style.background_color = SubtitleColor(0, 0, 0, 170);
    style.alignment = 2;
    style.margin_l = 20;
    style.margin_r = 20;
    style.margin_v = 20;
    style.border_style = 1;
    style.outline = 2.0;
    style.shadow = 2.0;
    style.outline_x = 2.0;
    style.outline_y = 2.0;
    style.shadow_x = 2.0;
    style.shadow_y = 2.0;
    style.wrap_style = 0;
    style.spacing = 0.0;
    style.scale_x_percent = 100.0;
    style.scale_y_percent = 100.0;
    style.rotation_degrees = 0.0;
    return style;
}

void clearClipState(SubtitleStyle& style) {
    style.has_clip = false;
    style.inverse_clip = false;
    style.clip_x1 = 0.0;
    style.clip_y1 = 0.0;
    style.clip_x2 = 0.0;
    style.clip_y2 = 0.0;
    style.has_vector_clip = false;
    style.vector_clip_scale = 1;
    style.vector_clip_commands.clear();
}

void setClipRect(SubtitleStyle& style, bool inverse, double x1, double y1, double x2, double y2) {
    clearClipState(style);
    style.has_clip = true;
    style.inverse_clip = inverse;
    style.clip_x1 = std::min(x1, x2);
    style.clip_y1 = std::min(y1, y2);
    style.clip_x2 = std::max(x1, x2);
    style.clip_y2 = std::max(y1, y2);
}

void setVectorClip(SubtitleStyle& style, bool inverse, int scale, const std::string& commands) {
    clearClipState(style);
    style.has_vector_clip = true;
    style.inverse_clip = inverse;
    style.vector_clip_scale = std::max(1, scale);
    style.vector_clip_commands = commands;
}

void syncOutlineScalar(SubtitleStyle& style) {
    style.outline = (style.outline_x + style.outline_y) * 0.5;
}

void setOutline(SubtitleStyle& style, double value) {
    style.outline = value;
    style.outline_x = value;
    style.outline_y = value;
}

void setOutlineX(SubtitleStyle& style, double value) {
    style.outline_x = value;
    syncOutlineScalar(style);
}

void setOutlineY(SubtitleStyle& style, double value) {
    style.outline_y = value;
    syncOutlineScalar(style);
}

void syncShadowScalar(SubtitleStyle& style) {
    style.shadow = std::max(std::abs(style.shadow_x), std::abs(style.shadow_y));
}

void setShadow(SubtitleStyle& style, double value) {
    style.shadow = value;
    style.shadow_x = value;
    style.shadow_y = value;
}

void setShadowX(SubtitleStyle& style, double value) {
    style.shadow_x = value;
    syncShadowScalar(style);
}

void setShadowY(SubtitleStyle& style, double value) {
    style.shadow_y = value;
    syncShadowScalar(style);
}

bool normalizeWrapStyle(int value, int& normalized) {
    if (value < 0 || value > 3) {
        return false;
    }
    normalized = value;
    return true;
}

bool parseRectClip(const std::string& value, double& x1, double& y1, double& x2, double& y2) {
    const std::vector<std::string> parts = splitAndTrim(value);
    if (parts.size() != 4) {
        return false;
    }
    return parseDouble(parts[0], x1) &&
           parseDouble(parts[1], y1) &&
           parseDouble(parts[2], x2) &&
           parseDouble(parts[3], y2);
}

bool parseOrgTag(const std::string& value, double& x, double& y) {
    const std::vector<std::string> parts = splitAndTrim(value);
    if (parts.size() != 2) {
        return false;
    }
    return parseDouble(parts[0], x) && parseDouble(parts[1], y);
}

bool parseVectorClipTag(const std::string& value, int& scale, std::string& commands) {
    const std::string trimmed = trimCopy(value);
    if (trimmed.empty()) {
        return false;
    }

    const size_t comma = trimmed.find(',');
    if (comma != std::string::npos) {
        int parsed_scale = 0;
        const std::string first = trimCopy(trimmed.substr(0, comma));
        const std::string rest = trimCopy(trimmed.substr(comma + 1));
        if (!rest.empty() && parseInt(first, parsed_scale) && parsed_scale > 0) {
            scale = parsed_scale;
            commands = rest;
            return true;
        }
    }

    scale = 1;
    commands = trimmed;
    return true;
}

bool parseMoveTag(const std::string& value,
                  double& x1,
                  double& y1,
                  double& x2,
                  double& y2,
                  bool& has_timing,
                  double& t1_seconds,
                  double& t2_seconds) {
    const std::vector<std::string> parts = splitAndTrim(value);
    if (parts.size() != 4 && parts.size() != 6) {
        return false;
    }
    if (!parseDouble(parts[0], x1) ||
        !parseDouble(parts[1], y1) ||
        !parseDouble(parts[2], x2) ||
        !parseDouble(parts[3], y2)) {
        return false;
    }

    has_timing = false;
    t1_seconds = 0.0;
    t2_seconds = 0.0;
    if (parts.size() == 6) {
        double t1_ms = 0.0;
        double t2_ms = 0.0;
        if (!parseDouble(parts[4], t1_ms) || !parseDouble(parts[5], t2_ms)) {
            return false;
        }
        has_timing = true;
        t1_seconds = std::max(0.0, t1_ms / 1000.0);
        t2_seconds = std::max(t1_seconds, t2_ms / 1000.0);
    }
    return true;
}

bool parseFadTag(const std::string& value, double& fade_in_seconds, double& fade_out_seconds) {
    const std::vector<std::string> parts = splitAndTrim(value);
    if (parts.size() != 2) {
        return false;
    }
    double fade_in_ms = 0.0;
    double fade_out_ms = 0.0;
    if (!parseDouble(parts[0], fade_in_ms) || !parseDouble(parts[1], fade_out_ms)) {
        return false;
    }
    fade_in_seconds = std::max(0.0, fade_in_ms / 1000.0);
    fade_out_seconds = std::max(0.0, fade_out_ms / 1000.0);
    return true;
}

uint8_t convertAssAnimationAlpha(int ass_alpha) {
    return static_cast<uint8_t>(255 - std::clamp(ass_alpha, 0, 255));
}

bool parseFadeTag(const std::string& value,
                  uint8_t& alpha1,
                  uint8_t& alpha2,
                  uint8_t& alpha3,
                  double& t1_seconds,
                  double& t2_seconds,
                  double& t3_seconds,
                  double& t4_seconds) {
    const std::vector<std::string> parts = splitAndTrim(value);
    if (parts.size() != 7) {
        return false;
    }

    int a1 = 0;
    int a2 = 0;
    int a3 = 0;
    double t1_ms = 0.0;
    double t2_ms = 0.0;
    double t3_ms = 0.0;
    double t4_ms = 0.0;
    if (!parseInt(parts[0], a1) ||
        !parseInt(parts[1], a2) ||
        !parseInt(parts[2], a3) ||
        !parseDouble(parts[3], t1_ms) ||
        !parseDouble(parts[4], t2_ms) ||
        !parseDouble(parts[5], t3_ms) ||
        !parseDouble(parts[6], t4_ms)) {
        return false;
    }

    alpha1 = convertAssAnimationAlpha(a1);
    alpha2 = convertAssAnimationAlpha(a2);
    alpha3 = convertAssAnimationAlpha(a3);
    t1_seconds = std::max(0.0, t1_ms / 1000.0);
    t2_seconds = std::max(t1_seconds, t2_ms / 1000.0);
    t3_seconds = std::max(t2_seconds, t3_ms / 1000.0);
    t4_seconds = std::max(t3_seconds, t4_ms / 1000.0);
    return true;
}

bool parseInt(const std::string& text, int& value) {
    const std::string trimmed = trimCopy(text);
    if (trimmed.empty()) {
        return false;
    }
    char* end = nullptr;
    const long parsed = std::strtol(trimmed.c_str(), &end, 10);
    if (!end || *end != '\0') {
        return false;
    }
    value = static_cast<int>(parsed);
    return true;
}

bool parseDouble(const std::string& text, double& value) {
    const std::string trimmed = trimCopy(text);
    if (trimmed.empty()) {
        return false;
    }
    char* end = nullptr;
    const double parsed = std::strtod(trimmed.c_str(), &end);
    if (!end || *end != '\0') {
        return false;
    }
    value = parsed;
    return true;
}

bool parseBoolFlag(const std::string& text, bool& value) {
    int parsed = 0;
    if (!parseInt(text, parsed)) {
        return false;
    }
    value = parsed != 0;
    return true;
}

bool parseAssTimecode(const std::string& text, double& seconds) {
    int hh = 0;
    int mm = 0;
    int ss = 0;
    int cs = 0;
#if defined(_WIN32)
    if (::sscanf_s(text.c_str(), "%d:%d:%d.%d", &hh, &mm, &ss, &cs) != 4) {
#else
    if (std::sscanf(text.c_str(), "%d:%d:%d.%d", &hh, &mm, &ss, &cs) != 4) {
#endif
        return false;
    }
    if (hh < 0 || mm < 0 || mm >= 60 || ss < 0 || ss >= 60 || cs < 0) {
        return false;
    }

    double fraction = static_cast<double>(cs);
    if (cs >= 100) {
        fraction /= 1000.0;
    } else {
        fraction /= 100.0;
    }

    seconds = static_cast<double>(hh) * 3600.0 +
              static_cast<double>(mm) * 60.0 +
              static_cast<double>(ss) +
              fraction;
    return true;
}

SubtitleColor parseAssColor(const std::string& text, const SubtitleColor& fallback) {
    std::string value = trimCopy(text);
    if (value.empty()) {
        return fallback;
    }
    value = toLowerAscii(value);
    if (value.rfind("&h", 0) == 0) {
        value.erase(0, 2);
    }
    if (!value.empty() && value.back() == '&') {
        value.pop_back();
    }
    if (value.empty()) {
        return fallback;
    }

    try {
        const unsigned long parsed = std::stoul(value, nullptr, 16);
        const uint8_t rr = static_cast<uint8_t>(parsed & 0xFFu);
        const uint8_t gg = static_cast<uint8_t>((parsed >> 8u) & 0xFFu);
        const uint8_t bb = static_cast<uint8_t>((parsed >> 16u) & 0xFFu);
        const uint8_t aa = value.size() > 6 ? static_cast<uint8_t>((parsed >> 24u) & 0xFFu) : 0u;
        return SubtitleColor(rr, gg, bb, static_cast<uint8_t>(255u - aa));
    } catch (...) {
        return fallback;
    }
}

uint8_t parseAssAlpha(const std::string& text, uint8_t fallback_alpha) {
    std::string value = trimCopy(text);
    if (value.empty()) {
        return fallback_alpha;
    }
    value = toLowerAscii(value);
    if (value.rfind("&h", 0) == 0) {
        value.erase(0, 2);
    }
    if (!value.empty() && value.back() == '&') {
        value.pop_back();
    }
    if (value.empty()) {
        return fallback_alpha;
    }
    try {
        const unsigned long parsed = std::stoul(value, nullptr, 16);
        const uint8_t ass_alpha = static_cast<uint8_t>(parsed & 0xFFu);
        return static_cast<uint8_t>(255u - ass_alpha);
    } catch (...) {
        return fallback_alpha;
    }
}

int parseLegacyAlignment(int value) {
    switch (value) {
    case 1: return 1;
    case 2: return 2;
    case 3: return 3;
    case 5: return 7;
    case 6: return 8;
    case 7: return 9;
    case 9: return 4;
    case 10: return 5;
    case 11: return 6;
    default: return value;
    }
}

std::vector<std::string> splitAndTrim(const std::string& text) {
    std::vector<std::string> parts;
    size_t start = 0;
    while (start <= text.size()) {
        const size_t comma = text.find(',', start);
        if (comma == std::string::npos) {
            parts.push_back(trimCopy(text.substr(start)));
            break;
        }
        parts.push_back(trimCopy(text.substr(start, comma - start)));
        start = comma + 1;
    }
    return parts;
}

std::vector<std::string> splitWithLimit(const std::string& text, size_t expected_parts) {
    if (expected_parts == 0) {
        return {};
    }

    std::vector<std::string> parts;
    parts.reserve(expected_parts);
    size_t start = 0;
    while (parts.size() + 1 < expected_parts) {
        const size_t comma = text.find(',', start);
        if (comma == std::string::npos) {
            break;
        }
        parts.push_back(trimCopy(text.substr(start, comma - start)));
        start = comma + 1;
    }
    parts.push_back(trimCopy(text.substr(start)));
    while (parts.size() < expected_parts) {
        parts.emplace_back();
    }
    return parts;
}

std::unordered_map<std::string, std::string> mapFields(const std::vector<std::string>& format_fields,
                                                       const std::vector<std::string>& values) {
    std::unordered_map<std::string, std::string> result;
    const size_t count = std::min(format_fields.size(), values.size());
    for (size_t i = 0; i < count; ++i) {
        result.emplace(format_fields[i], values[i]);
    }
    return result;
}

std::string decodeAssTextChunk(const std::string& chunk) {
    std::string output;
    output.reserve(chunk.size());
    for (size_t i = 0; i < chunk.size(); ++i) {
        const char ch = chunk[i];
        if (ch == '\\' && i + 1 < chunk.size()) {
            const char next = chunk[i + 1];
            switch (next) {
            case 'N':
            case 'n':
                output.push_back('\n');
                ++i;
                continue;
            case 'h':
                output.push_back(' ');
                ++i;
                continue;
            case '\\':
            case '{':
            case '}':
                output.push_back(next);
                ++i;
                continue;
            default:
                output.push_back(next);
                ++i;
                continue;
            }
        }
        output.push_back(ch);
    }
    return output;
}

void appendStyledText(ParsedAssText& result,
                      const SubtitleStyle& current_style,
                      const std::string& text,
                      size_t& cursor,
                      PendingKaraokeState& karaoke_state) {
    if (text.empty()) {
        return;
    }
    const size_t length = countUtf16CodeUnits(text);
    if (length == 0) {
        return;
    }
    result.visible_text += text;
    const SubtitleKaraokeMode karaoke_mode =
        karaoke_state.pending ? karaoke_state.mode : SubtitleKaraokeMode::None;
    const int karaoke_start_centiseconds = karaoke_state.pending ? karaoke_state.cursor_centiseconds : 0;
    const int karaoke_end_centiseconds =
        karaoke_state.pending ? (karaoke_start_centiseconds + std::max(0, karaoke_state.duration_centiseconds)) : 0;
    if (!result.runs.empty()) {
        SubtitleTextRun& last = result.runs.back();
        if (last.start + last.length == cursor &&
            last.style == current_style &&
            last.karaoke_mode == SubtitleKaraokeMode::None &&
            karaoke_mode == SubtitleKaraokeMode::None) {
            last.length += length;
            cursor += length;
            return;
        }
    }
    SubtitleTextRun run{};
    run.start = cursor;
    run.length = length;
    run.style = current_style;
    run.karaoke_mode = karaoke_mode;
    run.karaoke_start_centiseconds = karaoke_start_centiseconds;
    run.karaoke_end_centiseconds = karaoke_end_centiseconds;
    result.runs.push_back(std::move(run));
    cursor += length;
    if (karaoke_state.pending) {
        karaoke_state.cursor_centiseconds = karaoke_end_centiseconds;
        karaoke_state.pending = false;
    }
}

void appendDrawingCommands(ParsedAssText& result,
                           const std::string& text,
                           const DrawingModeState& drawing_mode) {
    if (!drawing_mode.enabled()) {
        return;
    }

    const std::string trimmed = trimCopy(text);
    if (trimmed.empty()) {
        return;
    }

    result.has_vector_drawing = true;
    result.drawing_scale = std::max(1, drawing_mode.scale);
    if (!result.drawing_commands.empty()) {
        result.drawing_commands.push_back(' ');
    }
    result.drawing_commands += trimmed;
}

uint64_t buildSupportedTransitionMask(const SubtitleStyle& from_style, const SubtitleStyle& to_style) {
    uint64_t mask = 0;
    if (from_style.font_size != to_style.font_size) mask |= kSubtitleTransitionFontSize;
    if (from_style.primary_color != to_style.primary_color) mask |= kSubtitleTransitionPrimaryColor;
    if (from_style.secondary_color != to_style.secondary_color) mask |= kSubtitleTransitionSecondaryColor;
    if (from_style.outline_color != to_style.outline_color) mask |= kSubtitleTransitionOutlineColor;
    if (from_style.background_color != to_style.background_color) mask |= kSubtitleTransitionBackgroundColor;
    if (from_style.outline_x != to_style.outline_x) mask |= kSubtitleTransitionOutlineX;
    if (from_style.outline_y != to_style.outline_y) mask |= kSubtitleTransitionOutlineY;
    if (from_style.shadow_x != to_style.shadow_x) mask |= kSubtitleTransitionShadowX;
    if (from_style.shadow_y != to_style.shadow_y) mask |= kSubtitleTransitionShadowY;
    if (from_style.spacing != to_style.spacing) mask |= kSubtitleTransitionSpacing;
    if (from_style.scale_x_percent != to_style.scale_x_percent) mask |= kSubtitleTransitionScaleX;
    if (from_style.scale_y_percent != to_style.scale_y_percent) mask |= kSubtitleTransitionScaleY;
    if (from_style.rotation_degrees != to_style.rotation_degrees) mask |= kSubtitleTransitionRotationZ;
    if (from_style.rotation_x_degrees != to_style.rotation_x_degrees) mask |= kSubtitleTransitionRotationX;
    if (from_style.rotation_y_degrees != to_style.rotation_y_degrees) mask |= kSubtitleTransitionRotationY;
    if (from_style.shear_x != to_style.shear_x) mask |= kSubtitleTransitionShearX;
    if (from_style.shear_y != to_style.shear_y) mask |= kSubtitleTransitionShearY;
    if (from_style.has_rotation_origin != to_style.has_rotation_origin ||
        from_style.rotation_origin_x != to_style.rotation_origin_x ||
        from_style.rotation_origin_y != to_style.rotation_origin_y) {
        mask |= kSubtitleTransitionRotationOrigin;
    }
    return mask;
}

bool parseTransformTag(const std::string& value,
                       const std::unordered_map<std::string, SubtitleStyle>& styles,
                       const SubtitleStyle& base_style,
                       const SubtitleStyle& current_style,
                       const SubtitleStyleAnimation& current_animation,
                       SubtitleStyleTransition& transition) {
    const std::vector<std::string> parts = splitTopLevelCommaSeparated(value);
    if (parts.empty()) {
        return false;
    }

    std::string transform_block;
    double start_seconds = 0.0;
    double end_seconds = 0.0;
    double accel = 1.0;
    bool has_timing = false;

    if (parts.size() == 1) {
        transform_block = parts[0];
    } else if (parts.size() == 2) {
        if (!parseDouble(parts[0], accel)) {
            return false;
        }
        transform_block = parts[1];
    } else if (parts.size() == 3) {
        double start_ms = 0.0;
        double end_ms = 0.0;
        if (!parseDouble(parts[0], start_ms) || !parseDouble(parts[1], end_ms)) {
            return false;
        }
        has_timing = true;
        start_seconds = std::max(0.0, start_ms / 1000.0);
        end_seconds = std::max(0.0, end_ms / 1000.0);
        transform_block = parts[2];
    } else if (parts.size() == 4) {
        double start_ms = 0.0;
        double end_ms = 0.0;
        if (!parseDouble(parts[0], start_ms) || !parseDouble(parts[1], end_ms) || !parseDouble(parts[2], accel)) {
            return false;
        }
        has_timing = true;
        start_seconds = std::max(0.0, start_ms / 1000.0);
        end_seconds = std::max(0.0, end_ms / 1000.0);
        transform_block = parts[3];
    } else {
        return false;
    }

    transform_block = trimCopy(transform_block);
    if (transform_block.empty()) {
        return false;
    }

    SubtitleStyle temp_cue_style = current_style;
    SubtitleStyle temp_current_style = current_style;
    SubtitleStyleAnimation temp_cue_animation = current_animation;
    SubtitleStyleAnimation temp_current_animation = current_animation;
    PendingKaraokeState temp_karaoke_state{};
    DrawingModeState temp_drawing_mode{};
    parseOverrideBlock(transform_block,
                       styles,
                       base_style,
                       temp_cue_style,
                       temp_current_style,
                       temp_cue_animation,
                       temp_current_animation,
                       temp_karaoke_state,
                       temp_drawing_mode,
                       false);

    const uint64_t property_mask = buildSupportedTransitionMask(current_style, temp_current_style);
    if (property_mask == 0) {
        return false;
    }

    transition.has_timing = has_timing;
    transition.start_seconds = start_seconds;
    transition.end_seconds = end_seconds;
    transition.accel = accel > 0.0 ? accel : 1.0;
    transition.property_mask = property_mask;
    transition.target_style = temp_current_style;
    return true;
}

void applyTagValue(const std::string& name,
                   const std::string& value,
                   const std::unordered_map<std::string, SubtitleStyle>& styles,
                   const SubtitleStyle& base_style,
                   SubtitleStyle& cue_style,
                   SubtitleStyle& current_style,
                   SubtitleStyleAnimation& cue_animation,
                   SubtitleStyleAnimation& current_animation,
                   PendingKaraokeState& karaoke_state,
                   DrawingModeState& drawing_mode,
                   bool allow_transform_tags) {
    if (name == "r") {
        SubtitleStyle reset_style = base_style;
        const std::string style_name = toLowerAscii(trimCopy(value));
        if (!style_name.empty()) {
            const auto it = styles.find(style_name);
            if (it != styles.end()) {
                reset_style = it->second;
            }
        }
        current_style = reset_style;
        cue_style = reset_style;
        current_animation = {};
        cue_animation = current_animation;
        return;
    }

    if (name == "fn") {
        const std::string font_name = trimCopy(value);
        if (!font_name.empty()) {
            current_style.font_family = font_name;
            cue_style.font_family = font_name;
        }
        return;
    }

    if (name == "fs") {
        double font_size = 0.0;
        if (parseDouble(value, font_size) && font_size > 0.0) {
            current_style.font_size = font_size;
            cue_style.font_size = font_size;
        }
        return;
    }

    if (name == "b") {
        bool enabled = current_style.bold;
        if (parseBoolFlag(value, enabled)) {
            current_style.bold = enabled;
            cue_style.bold = enabled;
        }
        return;
    }

    if (name == "i") {
        bool enabled = current_style.italic;
        if (parseBoolFlag(value, enabled)) {
            current_style.italic = enabled;
            cue_style.italic = enabled;
        }
        return;
    }

    if (name == "u") {
        bool enabled = current_style.underline;
        if (parseBoolFlag(value, enabled)) {
            current_style.underline = enabled;
            cue_style.underline = enabled;
        }
        return;
    }

    if (name == "s") {
        bool enabled = current_style.strikeout;
        if (parseBoolFlag(value, enabled)) {
            current_style.strikeout = enabled;
            cue_style.strikeout = enabled;
        }
        return;
    }

    if (name == "c" || name == "1c") {
        const SubtitleColor color = parseAssColor(value, current_style.primary_color);
        current_style.primary_color = color;
        cue_style.primary_color = color;
        return;
    }

    if (name == "2c") {
        const SubtitleColor color = parseAssColor(value, current_style.secondary_color);
        current_style.secondary_color = color;
        cue_style.secondary_color = color;
        return;
    }

    if (name == "3c") {
        const SubtitleColor color = parseAssColor(value, current_style.outline_color);
        current_style.outline_color = color;
        cue_style.outline_color = color;
        return;
    }

    if (name == "4c") {
        const SubtitleColor color = parseAssColor(value, current_style.background_color);
        current_style.background_color = color;
        cue_style.background_color = color;
        return;
    }

    if (name == "alpha") {
        const uint8_t alpha = parseAssAlpha(value, current_style.primary_color.a);
        current_style.primary_color.a = alpha;
        current_style.secondary_color.a = alpha;
        current_style.outline_color.a = alpha;
        current_style.background_color.a = alpha;
        cue_style.primary_color.a = alpha;
        cue_style.secondary_color.a = alpha;
        cue_style.outline_color.a = alpha;
        cue_style.background_color.a = alpha;
        return;
    }

    if (name == "1a") {
        const uint8_t alpha = parseAssAlpha(value, current_style.primary_color.a);
        current_style.primary_color.a = alpha;
        cue_style.primary_color.a = alpha;
        return;
    }

    if (name == "2a") {
        const uint8_t alpha = parseAssAlpha(value, current_style.secondary_color.a);
        current_style.secondary_color.a = alpha;
        cue_style.secondary_color.a = alpha;
        return;
    }

    if (name == "3a") {
        const uint8_t alpha = parseAssAlpha(value, current_style.outline_color.a);
        current_style.outline_color.a = alpha;
        cue_style.outline_color.a = alpha;
        return;
    }

    if (name == "4a") {
        const uint8_t alpha = parseAssAlpha(value, current_style.background_color.a);
        current_style.background_color.a = alpha;
        cue_style.background_color.a = alpha;
        return;
    }

    if (name == "bord") {
        double outline = 0.0;
        if (parseDouble(value, outline) && outline >= 0.0) {
            setOutline(current_style, outline);
            setOutline(cue_style, outline);
        }
        return;
    }

    if (name == "shad") {
        double shadow = 0.0;
        if (parseDouble(value, shadow) && shadow >= 0.0) {
            setShadow(current_style, shadow);
            setShadow(cue_style, shadow);
        }
        return;
    }

    if (name == "xbord") {
        double outline_x = 0.0;
        if (parseDouble(value, outline_x) && outline_x >= 0.0) {
            setOutlineX(current_style, outline_x);
            setOutlineX(cue_style, outline_x);
        }
        return;
    }

    if (name == "ybord") {
        double outline_y = 0.0;
        if (parseDouble(value, outline_y) && outline_y >= 0.0) {
            setOutlineY(current_style, outline_y);
            setOutlineY(cue_style, outline_y);
        }
        return;
    }

    if (name == "xshad") {
        double shadow_x = 0.0;
        if (parseDouble(value, shadow_x)) {
            setShadowX(current_style, shadow_x);
            setShadowX(cue_style, shadow_x);
        }
        return;
    }

    if (name == "yshad") {
        double shadow_y = 0.0;
        if (parseDouble(value, shadow_y)) {
            setShadowY(current_style, shadow_y);
            setShadowY(cue_style, shadow_y);
        }
        return;
    }

    if (name == "q") {
        int wrap_style = 0;
        if (parseInt(value, wrap_style) && normalizeWrapStyle(wrap_style, wrap_style)) {
            current_style.wrap_style = wrap_style;
            cue_style.wrap_style = wrap_style;
        }
        return;
    }

    if (name == "fsp") {
        double spacing = 0.0;
        if (parseDouble(value, spacing)) {
            current_style.spacing = spacing;
            cue_style.spacing = spacing;
        }
        return;
    }

    if (name == "fscx") {
        double scale_x_percent = 0.0;
        if (parseDouble(value, scale_x_percent) && scale_x_percent > 0.0) {
            current_style.scale_x_percent = scale_x_percent;
            cue_style.scale_x_percent = scale_x_percent;
        }
        return;
    }

    if (name == "fscy") {
        double scale_y_percent = 0.0;
        if (parseDouble(value, scale_y_percent) && scale_y_percent > 0.0) {
            current_style.scale_y_percent = scale_y_percent;
            cue_style.scale_y_percent = scale_y_percent;
        }
        return;
    }

    if (name == "fr" || name == "frz") {
        double rotation_degrees = 0.0;
        if (parseDouble(value, rotation_degrees)) {
            current_style.rotation_degrees = rotation_degrees;
            cue_style.rotation_degrees = rotation_degrees;
        }
        return;
    }

    if (name == "frx") {
        double rotation_x_degrees = 0.0;
        if (parseDouble(value, rotation_x_degrees)) {
            current_style.rotation_x_degrees = rotation_x_degrees;
            cue_style.rotation_x_degrees = rotation_x_degrees;
        }
        return;
    }

    if (name == "fry") {
        double rotation_y_degrees = 0.0;
        if (parseDouble(value, rotation_y_degrees)) {
            current_style.rotation_y_degrees = rotation_y_degrees;
            cue_style.rotation_y_degrees = rotation_y_degrees;
        }
        return;
    }

    if (name == "fax") {
        double shear_x = 0.0;
        if (parseDouble(value, shear_x)) {
            current_style.shear_x = shear_x;
            cue_style.shear_x = shear_x;
        }
        return;
    }

    if (name == "fay") {
        double shear_y = 0.0;
        if (parseDouble(value, shear_y)) {
            current_style.shear_y = shear_y;
            cue_style.shear_y = shear_y;
        }
        return;
    }

    if (name == "org") {
        double origin_x = 0.0;
        double origin_y = 0.0;
        if (parseOrgTag(value, origin_x, origin_y)) {
            current_style.has_rotation_origin = true;
            current_style.rotation_origin_x = origin_x;
            current_style.rotation_origin_y = origin_y;
            cue_style.has_rotation_origin = true;
            cue_style.rotation_origin_x = origin_x;
            cue_style.rotation_origin_y = origin_y;
        }
        return;
    }

    if (name == "t") {
        if (!allow_transform_tags) {
            return;
        }
        SubtitleStyleTransition transition{};
        if (parseTransformTag(value, styles, base_style, current_style, current_animation, transition)) {
            current_animation.style_transitions.push_back(std::move(transition));
            cue_animation = current_animation;
        }
        return;
    }

    if (name == "p") {
        int drawing_scale = 0;
        if (parseInt(value, drawing_scale) && drawing_scale >= 0) {
            drawing_mode.scale = drawing_scale;
        }
        return;
    }

    if (name == "clip" || name == "iclip") {
        double x1 = 0.0;
        double y1 = 0.0;
        double x2 = 0.0;
        double y2 = 0.0;
        if (parseRectClip(value, x1, y1, x2, y2)) {
            setClipRect(current_style, name == "iclip", x1, y1, x2, y2);
            setClipRect(cue_style, name == "iclip", x1, y1, x2, y2);
            return;
        }

        int vector_clip_scale = 1;
        std::string vector_clip_commands;
        if (parseVectorClipTag(value, vector_clip_scale, vector_clip_commands)) {
            setVectorClip(current_style, name == "iclip", vector_clip_scale, vector_clip_commands);
            setVectorClip(cue_style, name == "iclip", vector_clip_scale, vector_clip_commands);
        }
        return;
    }

    if (name == "move") {
        double x1 = 0.0;
        double y1 = 0.0;
        double x2 = 0.0;
        double y2 = 0.0;
        bool has_timing = false;
        double t1_seconds = 0.0;
        double t2_seconds = 0.0;
        if (parseMoveTag(value, x1, y1, x2, y2, has_timing, t1_seconds, t2_seconds)) {
            current_animation.has_move = true;
            current_animation.move_has_timing = has_timing;
            current_animation.move_x1 = x1;
            current_animation.move_y1 = y1;
            current_animation.move_x2 = x2;
            current_animation.move_y2 = y2;
            current_animation.move_t1_seconds = t1_seconds;
            current_animation.move_t2_seconds = t2_seconds;
            cue_animation = current_animation;
            current_style.has_position = false;
            cue_style.has_position = false;
        }
        return;
    }

    if (name == "fad") {
        double fade_in_seconds = 0.0;
        double fade_out_seconds = 0.0;
        if (parseFadTag(value, fade_in_seconds, fade_out_seconds)) {
            current_animation.fade_mode = SubtitleFadeMode::Simple;
            current_animation.fade_in_seconds = fade_in_seconds;
            current_animation.fade_out_seconds = fade_out_seconds;
            current_animation.fade_alpha1 = 0;
            current_animation.fade_alpha2 = 255;
            current_animation.fade_alpha3 = 0;
            current_animation.fade_t1_seconds = 0.0;
            current_animation.fade_t2_seconds = 0.0;
            current_animation.fade_t3_seconds = 0.0;
            current_animation.fade_t4_seconds = 0.0;
            cue_animation = current_animation;
        }
        return;
    }

    if (name == "fade") {
        uint8_t alpha1 = 255;
        uint8_t alpha2 = 255;
        uint8_t alpha3 = 255;
        double t1_seconds = 0.0;
        double t2_seconds = 0.0;
        double t3_seconds = 0.0;
        double t4_seconds = 0.0;
        if (parseFadeTag(value, alpha1, alpha2, alpha3, t1_seconds, t2_seconds, t3_seconds, t4_seconds)) {
            current_animation.fade_mode = SubtitleFadeMode::Complex;
            current_animation.fade_in_seconds = 0.0;
            current_animation.fade_out_seconds = 0.0;
            current_animation.fade_alpha1 = alpha1;
            current_animation.fade_alpha2 = alpha2;
            current_animation.fade_alpha3 = alpha3;
            current_animation.fade_t1_seconds = t1_seconds;
            current_animation.fade_t2_seconds = t2_seconds;
            current_animation.fade_t3_seconds = t3_seconds;
            current_animation.fade_t4_seconds = t4_seconds;
            cue_animation = current_animation;
        }
        return;
    }

    if (name == "k" || name == "kf" || name == "ko") {
        int duration_centiseconds = 0;
        if (parseInt(value, duration_centiseconds) && duration_centiseconds >= 0) {
            karaoke_state.pending = true;
            karaoke_state.duration_centiseconds = duration_centiseconds;
            karaoke_state.mode = (name == "kf") ? SubtitleKaraokeMode::Sweep : SubtitleKaraokeMode::Instant;
        }
        return;
    }

    if (name == "an") {
        int alignment = 0;
        if (parseInt(value, alignment) && alignment >= 1 && alignment <= 9) {
            current_style.alignment = alignment;
            cue_style.alignment = alignment;
        }
        return;
    }

    if (name == "a") {
        int alignment = 0;
        if (parseInt(value, alignment)) {
            alignment = parseLegacyAlignment(alignment);
            if (alignment >= 1 && alignment <= 9) {
                current_style.alignment = alignment;
                cue_style.alignment = alignment;
            }
        }
        return;
    }

    if (name == "pos") {
        const size_t comma = value.find(',');
        if (comma == std::string::npos) {
            return;
        }
        double x = 0.0;
        double y = 0.0;
        if (parseDouble(value.substr(0, comma), x) && parseDouble(value.substr(comma + 1), y)) {
            current_style.has_position = true;
            current_style.position_x = x;
            current_style.position_y = y;
            cue_style.has_position = true;
            cue_style.position_x = x;
            cue_style.position_y = y;
            current_animation.has_move = false;
            current_animation.move_has_timing = false;
            cue_animation = current_animation;
        }
    }
}

void parseOverrideBlock(const std::string& block,
                        const std::unordered_map<std::string, SubtitleStyle>& styles,
                        const SubtitleStyle& base_style,
                        SubtitleStyle& cue_style,
                        SubtitleStyle& current_style,
                        SubtitleStyleAnimation& cue_animation,
                        SubtitleStyleAnimation& current_animation,
                        PendingKaraokeState& karaoke_state,
                        DrawingModeState& drawing_mode,
                        bool allow_transform_tags) {
    size_t cursor = 0;
    while (cursor < block.size()) {
        if (block[cursor] != '\\') {
            ++cursor;
            continue;
        }
        ++cursor;

        std::string name = tryMatchSupportedTag(block, cursor);
        if (!name.empty()) {
            cursor += name.size();
        } else {
            const size_t name_start = cursor;
            while (cursor < block.size() && std::isdigit(static_cast<unsigned char>(block[cursor])) != 0) {
                ++cursor;
            }
            while (cursor < block.size() && std::isalpha(static_cast<unsigned char>(block[cursor])) != 0) {
                ++cursor;
            }
            const std::string raw_name = block.substr(name_start, cursor - name_start);
            name = toLowerAscii(raw_name);
            if (raw_name == "K") {
                name = "kf";
            }
        }
        std::string value;
        if (cursor < block.size() && block[cursor] == '(') {
            const size_t close = findMatchingParenthesis(block, cursor);
            if (close == std::string::npos) {
                value = block.substr(cursor + 1);
                cursor = block.size();
            } else {
                value = block.substr(cursor + 1, close - cursor - 1);
                cursor = close + 1;
            }
        } else {
            const size_t value_start = cursor;
            while (cursor < block.size() && block[cursor] != '\\') {
                ++cursor;
            }
            value = block.substr(value_start, cursor - value_start);
        }

        if (!name.empty()) {
            applyTagValue(name,
                          trimCopy(value),
                          styles,
                          base_style,
                          cue_style,
                          current_style,
                          cue_animation,
                          current_animation,
                          karaoke_state,
                          drawing_mode,
                          allow_transform_tags);
        }
    }
}

ParsedAssText parseAssText(const std::string& raw_text,
                           const std::unordered_map<std::string, SubtitleStyle>& styles,
                           const SubtitleStyle& base_style) {
    ParsedAssText result;
    result.cue_style = base_style;
    SubtitleStyle current_style = base_style;
    SubtitleStyleAnimation current_animation{};
    PendingKaraokeState karaoke_state{};
    DrawingModeState drawing_mode{};
    size_t cursor = 0;
    size_t text_start = 0;

    while (text_start < raw_text.size()) {
        const size_t open = raw_text.find('{', text_start);
        const size_t chunk_end = (open == std::string::npos) ? raw_text.size() : open;
        if (drawing_mode.enabled()) {
            appendDrawingCommands(result,
                                  raw_text.substr(text_start, chunk_end - text_start),
                                  drawing_mode);
        } else {
            appendStyledText(result,
                             current_style,
                             decodeAssTextChunk(raw_text.substr(text_start, chunk_end - text_start)),
                             cursor,
                             karaoke_state);
        }
        if (open == std::string::npos) {
            break;
        }

        const size_t close = raw_text.find('}', open + 1);
        if (close == std::string::npos) {
            if (drawing_mode.enabled()) {
                appendDrawingCommands(result,
                                      raw_text.substr(open),
                                      drawing_mode);
            } else {
                appendStyledText(result,
                                 current_style,
                                 decodeAssTextChunk(raw_text.substr(open)),
                                 cursor,
                                 karaoke_state);
            }
            break;
        }

        parseOverrideBlock(raw_text.substr(open + 1, close - open - 1),
                           styles,
                           base_style,
                           result.cue_style,
                           current_style,
                           result.cue_animation,
                           current_animation,
                           karaoke_state,
                           drawing_mode,
                           true);
        text_start = close + 1;
    }

    if (result.runs.empty() && !result.visible_text.empty()) {
        SubtitleTextRun run{};
        run.start = 0;
        run.length = countUtf16CodeUnits(result.visible_text);
        run.style = current_style;
        result.runs.push_back(std::move(run));
    }
    return result;
}

}  // namespace

bool AssParser::parseFile(const std::string& file_path) {
    std::ifstream input(file_path, std::ios::binary);
    if (!input.good()) {
        return false;
    }

    return parseStream(input, file_path);
}

bool AssParser::parseText(const std::string& content, const std::string& source_path) {
    std::istringstream input(content);
    return parseStream(input, source_path);
}

bool AssParser::parseStream(std::istream& input, const std::string& source_path) {
    items_.clear();

    enum class Section {
        None,
        ScriptInfo,
        Styles,
        Events,
    };

    Section section = Section::None;
    int play_res_x = 0;
    int play_res_y = 0;
    SubtitleStyle default_style = makeDefaultAssStyle();
    std::unordered_map<std::string, SubtitleStyle> styles;
    std::vector<std::string> style_format;
    std::vector<std::string> event_format;

    std::string line;
    bool first_line = true;
    int next_index = 1;
    while (std::getline(input, line)) {
        if (first_line && line.size() >= 3 &&
            static_cast<unsigned char>(line[0]) == 0xEF &&
            static_cast<unsigned char>(line[1]) == 0xBB &&
            static_cast<unsigned char>(line[2]) == 0xBF) {
            line.erase(0, 3);
        }
        first_line = false;

        const std::string trimmed = trimCopy(line);
        if (trimmed.empty() || trimmed.front() == ';') {
            continue;
        }

        if (trimmed.front() == '[' && trimmed.back() == ']') {
            const std::string section_name = toLowerAscii(trimCopy(trimmed.substr(1, trimmed.size() - 2)));
            if (section_name == "script info") {
                section = Section::ScriptInfo;
            } else if (section_name == "v4+ styles" || section_name == "v4 styles") {
                section = Section::Styles;
            } else if (section_name == "events") {
                section = Section::Events;
            } else {
                section = Section::None;
            }
            continue;
        }

        if (section == Section::ScriptInfo) {
            const size_t colon = trimmed.find(':');
            if (colon == std::string::npos) {
                continue;
            }
            const std::string key = normalizeFieldName(trimmed.substr(0, colon));
            const std::string value = trimCopy(trimmed.substr(colon + 1));
            if (key == "playresx") {
                parseInt(value, play_res_x);
            } else if (key == "playresy") {
                parseInt(value, play_res_y);
            } else if (key == "wrapstyle") {
                int wrap_style = 0;
                if (parseInt(value, wrap_style) && normalizeWrapStyle(wrap_style, wrap_style)) {
                    default_style.wrap_style = wrap_style;
                }
            }
            continue;
        }

        if (section == Section::Styles) {
            if (trimmed.rfind("Format:", 0) == 0) {
                style_format.clear();
                for (const std::string& field : splitAndTrim(trimmed.substr(7))) {
                    style_format.push_back(normalizeFieldName(field));
                }
                continue;
            }
            if (trimmed.rfind("Style:", 0) != 0) {
                continue;
            }
            if (style_format.empty()) {
                style_format = {
                    "name", "fontname", "fontsize", "primarycolour", "secondarycolour",
                    "outlinecolour", "backcolour", "bold", "italic", "underline", "strikeout",
                    "scalex", "scaley", "spacing", "angle", "borderstyle", "outline", "shadow",
                    "alignment", "marginl", "marginr", "marginv", "encoding"
                };
            }
            const auto field_values = mapFields(style_format, splitWithLimit(trimmed.substr(6), style_format.size()));
            SubtitleStyle style = default_style;
            if (const auto it = field_values.find("name"); it != field_values.end()) {
                style.style_name = it->second;
            }
            if (const auto it = field_values.find("fontname"); it != field_values.end() && !it->second.empty()) {
                style.font_family = it->second;
            }
            if (const auto it = field_values.find("fontsize"); it != field_values.end()) {
                parseDouble(it->second, style.font_size);
            }
            if (const auto it = field_values.find("primarycolour"); it != field_values.end()) {
                style.primary_color = parseAssColor(it->second, style.primary_color);
            }
            if (const auto it = field_values.find("secondarycolour"); it != field_values.end()) {
                style.secondary_color = parseAssColor(it->second, style.secondary_color);
            }
            if (const auto it = field_values.find("outlinecolour"); it != field_values.end()) {
                style.outline_color = parseAssColor(it->second, style.outline_color);
            } else if (const auto fallback_it = field_values.find("tertiarycolour"); fallback_it != field_values.end()) {
                style.outline_color = parseAssColor(fallback_it->second, style.outline_color);
            }
            if (const auto it = field_values.find("backcolour"); it != field_values.end()) {
                style.background_color = parseAssColor(it->second, style.background_color);
            }
            if (const auto it = field_values.find("bold"); it != field_values.end()) {
                parseBoolFlag(it->second, style.bold);
            }
            if (const auto it = field_values.find("italic"); it != field_values.end()) {
                parseBoolFlag(it->second, style.italic);
            }
            if (const auto it = field_values.find("underline"); it != field_values.end()) {
                parseBoolFlag(it->second, style.underline);
            }
            if (const auto it = field_values.find("strikeout"); it != field_values.end()) {
                parseBoolFlag(it->second, style.strikeout);
            }
            if (const auto it = field_values.find("borderstyle"); it != field_values.end()) {
                parseInt(it->second, style.border_style);
            }
            if (const auto it = field_values.find("scalex"); it != field_values.end()) {
                double scale_x_percent = style.scale_x_percent;
                if (parseDouble(it->second, scale_x_percent) && scale_x_percent > 0.0) {
                    style.scale_x_percent = scale_x_percent;
                }
            }
            if (const auto it = field_values.find("scaley"); it != field_values.end()) {
                double scale_y_percent = style.scale_y_percent;
                if (parseDouble(it->second, scale_y_percent) && scale_y_percent > 0.0) {
                    style.scale_y_percent = scale_y_percent;
                }
            }
            if (const auto it = field_values.find("spacing"); it != field_values.end()) {
                parseDouble(it->second, style.spacing);
            }
            if (const auto it = field_values.find("angle"); it != field_values.end()) {
                parseDouble(it->second, style.rotation_degrees);
            }
            if (const auto it = field_values.find("outline"); it != field_values.end()) {
                double outline = style.outline;
                if (parseDouble(it->second, outline)) {
                    setOutline(style, outline);
                }
            }
            if (const auto it = field_values.find("shadow"); it != field_values.end()) {
                double shadow = style.shadow;
                if (parseDouble(it->second, shadow)) {
                    setShadow(style, shadow);
                }
            }
            if (const auto it = field_values.find("alignment"); it != field_values.end()) {
                parseInt(it->second, style.alignment);
            }
            if (const auto it = field_values.find("marginl"); it != field_values.end()) {
                parseInt(it->second, style.margin_l);
            }
            if (const auto it = field_values.find("marginr"); it != field_values.end()) {
                parseInt(it->second, style.margin_r);
            }
            if (const auto it = field_values.find("marginv"); it != field_values.end()) {
                parseInt(it->second, style.margin_v);
            }
            styles[toLowerAscii(style.style_name)] = style;
            if (toLowerAscii(style.style_name) == "default") {
                default_style = style;
            }
            continue;
        }

        if (section != Section::Events) {
            continue;
        }

        if (trimmed.rfind("Format:", 0) == 0) {
            event_format.clear();
            for (const std::string& field : splitAndTrim(trimmed.substr(7))) {
                event_format.push_back(normalizeFieldName(field));
            }
            continue;
        }
        if (trimmed.rfind("Dialogue:", 0) != 0) {
            continue;
        }
        if (event_format.empty()) {
            event_format = {
                "layer", "start", "end", "style", "name", "marginl", "marginr", "marginv", "effect", "text"
            };
        }

        const auto field_values = mapFields(event_format, splitWithLimit(trimmed.substr(9), event_format.size()));
        SubtitleItem item{};
        item.index = next_index++;
        item.play_res_x = play_res_x;
        item.play_res_y = play_res_y;

        if (const auto it = field_values.find("layer"); it != field_values.end()) {
            parseInt(it->second, item.layer);
        } else if (const auto fallback_it = field_values.find("marked"); fallback_it != field_values.end()) {
            parseInt(fallback_it->second, item.layer);
        }
        if (const auto it = field_values.find("start"); it == field_values.end() || !parseAssTimecode(it->second, item.start_seconds)) {
            continue;
        }
        if (const auto it = field_values.find("end"); it == field_values.end() || !parseAssTimecode(it->second, item.end_seconds)) {
            continue;
        }

        std::string style_name = "Default";
        if (const auto it = field_values.find("style"); it != field_values.end() && !it->second.empty()) {
            style_name = it->second;
        }
        SubtitleStyle base_style = default_style;
        if (const auto it = styles.find(toLowerAscii(style_name)); it != styles.end()) {
            base_style = it->second;
        }
        base_style.style_name = style_name;

        if (const auto it = field_values.find("marginl"); it != field_values.end()) {
            parseInt(it->second, base_style.margin_l);
        }
        if (const auto it = field_values.find("marginr"); it != field_values.end()) {
            parseInt(it->second, base_style.margin_r);
        }
        if (const auto it = field_values.find("marginv"); it != field_values.end()) {
            parseInt(it->second, base_style.margin_v);
        }

        const auto text_it = field_values.find("text");
        item.source_path = source_path;
        item.raw_text = (text_it != field_values.end()) ? text_it->second : std::string{};
        ParsedAssText parsed = parseAssText(item.raw_text, styles, base_style);
        item.text = std::move(parsed.visible_text);
        item.style = std::move(parsed.cue_style);
        item.animation = std::move(parsed.cue_animation);
        item.is_vector_drawing = parsed.has_vector_drawing && parsed.visible_text.empty();
        item.drawing_scale = std::max(1, parsed.drawing_scale);
        item.drawing_commands = std::move(parsed.drawing_commands);
        item.runs = std::move(parsed.runs);

        if (item.style.style_name.empty()) {
            item.style.style_name = style_name;
        }
        if (item.end_seconds < item.start_seconds) {
            continue;
        }
        if (item.text.empty() && !item.is_vector_drawing) {
            continue;
        }
        items_.push_back(std::move(item));
    }

    std::sort(items_.begin(), items_.end(), [](const SubtitleItem& lhs, const SubtitleItem& rhs) {
        if (lhs.start_seconds != rhs.start_seconds) {
            return lhs.start_seconds < rhs.start_seconds;
        }
        if (lhs.layer != rhs.layer) {
            return lhs.layer < rhs.layer;
        }
        if (lhs.end_seconds != rhs.end_seconds) {
            return lhs.end_seconds < rhs.end_seconds;
        }
        return lhs.index < rhs.index;
    });

    return !items_.empty();
}

const std::vector<SubtitleItem>& AssParser::items() const {
    return items_;
}

SubtitleFormat AssParser::format() const {
    return SubtitleFormat::Ass;
}

}  // namespace vp::subtitle
