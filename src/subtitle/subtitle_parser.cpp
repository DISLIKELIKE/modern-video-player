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
           lhs.secondary_color == rhs.secondary_color &&
           lhs.outline_color == rhs.outline_color &&
           lhs.background_color == rhs.background_color &&
           lhs.alignment == rhs.alignment &&
           lhs.margin_l == rhs.margin_l &&
           lhs.margin_r == rhs.margin_r &&
           lhs.margin_v == rhs.margin_v &&
           lhs.border_style == rhs.border_style &&
           lhs.outline == rhs.outline &&
           lhs.shadow == rhs.shadow &&
           lhs.outline_x == rhs.outline_x &&
           lhs.outline_y == rhs.outline_y &&
           lhs.shadow_x == rhs.shadow_x &&
           lhs.shadow_y == rhs.shadow_y &&
           lhs.wrap_style == rhs.wrap_style &&
           lhs.spacing == rhs.spacing &&
           lhs.scale_x_percent == rhs.scale_x_percent &&
           lhs.scale_y_percent == rhs.scale_y_percent &&
           lhs.rotation_degrees == rhs.rotation_degrees &&
           lhs.rotation_x_degrees == rhs.rotation_x_degrees &&
           lhs.rotation_y_degrees == rhs.rotation_y_degrees &&
           lhs.shear_x == rhs.shear_x &&
           lhs.shear_y == rhs.shear_y &&
           lhs.has_rotation_origin == rhs.has_rotation_origin &&
           lhs.rotation_origin_x == rhs.rotation_origin_x &&
           lhs.rotation_origin_y == rhs.rotation_origin_y &&
           lhs.has_clip == rhs.has_clip &&
           lhs.inverse_clip == rhs.inverse_clip &&
           lhs.clip_x1 == rhs.clip_x1 &&
           lhs.clip_y1 == rhs.clip_y1 &&
           lhs.clip_x2 == rhs.clip_x2 &&
           lhs.clip_y2 == rhs.clip_y2 &&
           lhs.has_vector_clip == rhs.has_vector_clip &&
           lhs.vector_clip_scale == rhs.vector_clip_scale &&
           lhs.vector_clip_commands == rhs.vector_clip_commands &&
           lhs.has_position == rhs.has_position &&
           lhs.position_x == rhs.position_x &&
           lhs.position_y == rhs.position_y;
}

bool operator!=(const SubtitleStyle& lhs, const SubtitleStyle& rhs) {
    return !(lhs == rhs);
}

bool operator==(const SubtitleStyleTransition& lhs, const SubtitleStyleTransition& rhs) {
    return lhs.has_timing == rhs.has_timing &&
           lhs.start_seconds == rhs.start_seconds &&
           lhs.end_seconds == rhs.end_seconds &&
           lhs.accel == rhs.accel &&
           lhs.property_mask == rhs.property_mask &&
           lhs.target_style == rhs.target_style;
}

bool operator!=(const SubtitleStyleTransition& lhs, const SubtitleStyleTransition& rhs) {
    return !(lhs == rhs);
}

bool operator==(const SubtitleStyleAnimation& lhs, const SubtitleStyleAnimation& rhs) {
    return lhs.has_move == rhs.has_move &&
           lhs.move_has_timing == rhs.move_has_timing &&
           lhs.move_x1 == rhs.move_x1 &&
           lhs.move_y1 == rhs.move_y1 &&
           lhs.move_x2 == rhs.move_x2 &&
           lhs.move_y2 == rhs.move_y2 &&
           lhs.move_t1_seconds == rhs.move_t1_seconds &&
           lhs.move_t2_seconds == rhs.move_t2_seconds &&
           lhs.fade_mode == rhs.fade_mode &&
           lhs.fade_in_seconds == rhs.fade_in_seconds &&
           lhs.fade_out_seconds == rhs.fade_out_seconds &&
           lhs.fade_alpha1 == rhs.fade_alpha1 &&
           lhs.fade_alpha2 == rhs.fade_alpha2 &&
           lhs.fade_alpha3 == rhs.fade_alpha3 &&
           lhs.fade_t1_seconds == rhs.fade_t1_seconds &&
           lhs.fade_t2_seconds == rhs.fade_t2_seconds &&
           lhs.fade_t3_seconds == rhs.fade_t3_seconds &&
           lhs.fade_t4_seconds == rhs.fade_t4_seconds &&
           lhs.style_transitions == rhs.style_transitions;
}

bool operator!=(const SubtitleStyleAnimation& lhs, const SubtitleStyleAnimation& rhs) {
    return !(lhs == rhs);
}

bool operator==(const SubtitleTextRun& lhs, const SubtitleTextRun& rhs) {
    return lhs.start == rhs.start &&
           lhs.length == rhs.length &&
           lhs.style == rhs.style &&
           lhs.karaoke_mode == rhs.karaoke_mode &&
           lhs.karaoke_start_centiseconds == rhs.karaoke_start_centiseconds &&
           lhs.karaoke_end_centiseconds == rhs.karaoke_end_centiseconds;
}

bool operator!=(const SubtitleTextRun& lhs, const SubtitleTextRun& rhs) {
    return !(lhs == rhs);
}

bool operator==(const SubtitleBitmap& lhs, const SubtitleBitmap& rhs) {
    return lhs.x == rhs.x &&
           lhs.y == rhs.y &&
           lhs.width == rhs.width &&
           lhs.height == rhs.height &&
           lhs.rgba == rhs.rgba;
}

bool operator!=(const SubtitleBitmap& lhs, const SubtitleBitmap& rhs) {
    return !(lhs == rhs);
}

bool operator==(const SubtitleItem& lhs, const SubtitleItem& rhs) {
    return lhs.index == rhs.index &&
           lhs.layer == rhs.layer &&
           lhs.start_seconds == rhs.start_seconds &&
           lhs.end_seconds == rhs.end_seconds &&
           lhs.source_path == rhs.source_path &&
           lhs.text == rhs.text &&
           lhs.raw_text == rhs.raw_text &&
           lhs.play_res_x == rhs.play_res_x &&
           lhs.play_res_y == rhs.play_res_y &&
           lhs.style == rhs.style &&
           lhs.animation == rhs.animation &&
           lhs.is_vector_drawing == rhs.is_vector_drawing &&
           lhs.drawing_scale == rhs.drawing_scale &&
           lhs.drawing_commands == rhs.drawing_commands &&
           lhs.is_bitmap == rhs.is_bitmap &&
           lhs.bitmap_rects == rhs.bitmap_rects &&
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
