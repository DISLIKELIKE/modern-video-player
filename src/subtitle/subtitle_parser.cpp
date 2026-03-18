#include "subtitle/subtitle_parser.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <sstream>

#include "subtitle/ass_parser.h"
#include "subtitle/srt_parser.h"

namespace vp::subtitle {

bool operator==(const SubtitleColor& lhs, const SubtitleColor& rhs) {
    return lhs.r == rhs.r && lhs.g == rhs.g && lhs.b == rhs.b && lhs.a == rhs.a;
}

bool operator!=(const SubtitleColor& lhs, const SubtitleColor& rhs) {
    return !(lhs == rhs);
}

bool operator==(const SubtitleStyle& lhs, const SubtitleStyle& rhs) {
    return lhs.style_name == rhs.style_name &&
           lhs.font_family == rhs.font_family &&
           lhs.font_size == rhs.font_size &&
           lhs.bold == rhs.bold &&
           lhs.italic == rhs.italic &&
           lhs.underline == rhs.underline &&
           lhs.strikeout == rhs.strikeout &&
           lhs.primary_color == rhs.primary_color &&
           lhs.outline_color == rhs.outline_color &&
           lhs.background_color == rhs.background_color &&
           lhs.alignment == rhs.alignment &&
           lhs.margin_l == rhs.margin_l &&
           lhs.margin_r == rhs.margin_r &&
           lhs.margin_v == rhs.margin_v &&
           lhs.border_style == rhs.border_style &&
           lhs.outline == rhs.outline &&
           lhs.shadow == rhs.shadow &&
           lhs.has_position == rhs.has_position &&
           lhs.position_x == rhs.position_x &&
           lhs.position_y == rhs.position_y;
}

bool operator!=(const SubtitleStyle& lhs, const SubtitleStyle& rhs) {
    return !(lhs == rhs);
}

bool operator==(const SubtitleTextRun& lhs, const SubtitleTextRun& rhs) {
    return lhs.start == rhs.start && lhs.length == rhs.length && lhs.style == rhs.style;
}

bool operator!=(const SubtitleTextRun& lhs, const SubtitleTextRun& rhs) {
    return !(lhs == rhs);
}

bool operator==(const SubtitleItem& lhs, const SubtitleItem& rhs) {
    return lhs.index == rhs.index &&
           lhs.layer == rhs.layer &&
           lhs.start_seconds == rhs.start_seconds &&
           lhs.end_seconds == rhs.end_seconds &&
           lhs.text == rhs.text &&
           lhs.raw_text == rhs.raw_text &&
           lhs.play_res_x == rhs.play_res_x &&
           lhs.play_res_y == rhs.play_res_y &&
           lhs.style == rhs.style &&
           lhs.runs == rhs.runs;
}

bool operator!=(const SubtitleItem& lhs, const SubtitleItem& rhs) {
    return !(lhs == rhs);
}

namespace {

std::string toLowerAscii(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

std::string extractExtension(std::string extension_or_path) {
    if (extension_or_path.empty()) {
        return {};
    }

    const std::filesystem::path path(extension_or_path);
    if (!path.extension().empty()) {
        extension_or_path = path.extension().string();
    }

    if (!extension_or_path.empty() && extension_or_path.front() == '.') {
        extension_or_path.erase(extension_or_path.begin());
    }
    return toLowerAscii(extension_or_path);
}

}  // namespace

bool isSupportedSubtitleExtension(const std::string& extension_or_path) {
    const std::string extension = extractExtension(extension_or_path);
    return extension == "srt" || extension == "ass" || extension == "ssa";
}

std::unique_ptr<ISubtitleParser> createParserForPath(const std::string& file_path) {
    const std::string extension = extractExtension(file_path);
    if (extension == "srt") {
        return std::make_unique<SrtParser>();
    }
    if (extension == "ass" || extension == "ssa") {
        return std::make_unique<AssParser>();
    }
    return nullptr;
}

std::string flattenSubtitleText(const std::vector<SubtitleItem>& items) {
    std::ostringstream oss;
    bool first = true;
    for (const SubtitleItem& item : items) {
        if (item.text.empty()) {
            continue;
        }
        if (!first) {
            oss << '\n';
        }
        first = false;
        oss << item.text;
    }
    return oss.str();
}

}  // namespace vp::subtitle
