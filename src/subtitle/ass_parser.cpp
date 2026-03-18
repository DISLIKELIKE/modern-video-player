#include "subtitle/ass_parser.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <unordered_map>
#include <utility>

namespace vp::subtitle {

namespace {

struct ParsedAssText {
    std::string visible_text;
    SubtitleStyle cue_style;
    std::vector<SubtitleTextRun> runs;
};

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
        "alpha",
        "bord",
        "shad",
        "pos",
        "fn",
        "fs",
        "an",
        "1c",
        "1a",
        "c",
        "a",
        "b",
        "i",
        "u",
        "s",
        "r",
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
    style.outline_color = SubtitleColor(0, 0, 0, 255);
    style.background_color = SubtitleColor(0, 0, 0, 170);
    style.alignment = 2;
    style.margin_l = 20;
    style.margin_r = 20;
    style.margin_v = 20;
    style.border_style = 1;
    style.outline = 2.0;
    style.shadow = 2.0;
    return style;
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
    if (::sscanf_s(text.c_str(), "%d:%d:%d.%d", &hh, &mm, &ss, &cs) != 4) {
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
                      size_t& cursor) {
    if (text.empty()) {
        return;
    }
    const size_t length = countUtf16CodeUnits(text);
    if (length == 0) {
        return;
    }
    result.visible_text += text;
    if (!result.runs.empty()) {
        SubtitleTextRun& last = result.runs.back();
        if (last.start + last.length == cursor && last.style == current_style) {
            last.length += length;
            cursor += length;
            return;
        }
    }
    result.runs.push_back(SubtitleTextRun{cursor, length, current_style});
    cursor += length;
}

void applyTagValue(const std::string& name,
                   const std::string& value,
                   const std::unordered_map<std::string, SubtitleStyle>& styles,
                   const SubtitleStyle& base_style,
                   SubtitleStyle& cue_style,
                   SubtitleStyle& current_style) {
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

    if (name == "alpha" || name == "1a") {
        const uint8_t alpha = parseAssAlpha(value, current_style.primary_color.a);
        current_style.primary_color.a = alpha;
        cue_style.primary_color.a = alpha;
        return;
    }

    if (name == "bord") {
        double outline = 0.0;
        if (parseDouble(value, outline) && outline >= 0.0) {
            current_style.outline = outline;
            cue_style.outline = outline;
        }
        return;
    }

    if (name == "shad") {
        double shadow = 0.0;
        if (parseDouble(value, shadow) && shadow >= 0.0) {
            current_style.shadow = shadow;
            cue_style.shadow = shadow;
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
        }
    }
}

void parseOverrideBlock(const std::string& block,
                        const std::unordered_map<std::string, SubtitleStyle>& styles,
                        const SubtitleStyle& base_style,
                        SubtitleStyle& cue_style,
                        SubtitleStyle& current_style) {
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
            name = toLowerAscii(block.substr(name_start, cursor - name_start));
        }
        std::string value;
        if (cursor < block.size() && block[cursor] == '(') {
            const size_t close = block.find(')', cursor + 1);
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
            applyTagValue(name, trimCopy(value), styles, base_style, cue_style, current_style);
        }
    }
}

ParsedAssText parseAssText(const std::string& raw_text,
                           const std::unordered_map<std::string, SubtitleStyle>& styles,
                           const SubtitleStyle& base_style) {
    ParsedAssText result;
    result.cue_style = base_style;
    SubtitleStyle current_style = base_style;
    size_t cursor = 0;
    size_t text_start = 0;

    while (text_start < raw_text.size()) {
        const size_t open = raw_text.find('{', text_start);
        const size_t chunk_end = (open == std::string::npos) ? raw_text.size() : open;
        appendStyledText(result,
                         current_style,
                         decodeAssTextChunk(raw_text.substr(text_start, chunk_end - text_start)),
                         cursor);
        if (open == std::string::npos) {
            break;
        }

        const size_t close = raw_text.find('}', open + 1);
        if (close == std::string::npos) {
            appendStyledText(result,
                             current_style,
                             decodeAssTextChunk(raw_text.substr(open)),
                             cursor);
            break;
        }

        parseOverrideBlock(raw_text.substr(open + 1, close - open - 1),
                           styles,
                           base_style,
                           result.cue_style,
                           current_style);
        text_start = close + 1;
    }

    if (result.runs.empty() && !result.visible_text.empty()) {
        result.runs.push_back(SubtitleTextRun{0, countUtf16CodeUnits(result.visible_text), current_style});
    }
    return result;
}

}  // namespace

bool AssParser::parseFile(const std::string& file_path) {
    items_.clear();

    std::ifstream input(file_path, std::ios::binary);
    if (!input.good()) {
        return false;
    }

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
            if (const auto it = field_values.find("outline"); it != field_values.end()) {
                parseDouble(it->second, style.outline);
            }
            if (const auto it = field_values.find("shadow"); it != field_values.end()) {
                parseDouble(it->second, style.shadow);
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
        item.raw_text = (text_it != field_values.end()) ? text_it->second : std::string{};
        ParsedAssText parsed = parseAssText(item.raw_text, styles, base_style);
        item.text = std::move(parsed.visible_text);
        item.style = std::move(parsed.cue_style);
        item.runs = std::move(parsed.runs);

        if (item.style.style_name.empty()) {
            item.style.style_name = style_name;
        }
        if (item.end_seconds < item.start_seconds) {
            continue;
        }
        if (item.text.empty()) {
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
