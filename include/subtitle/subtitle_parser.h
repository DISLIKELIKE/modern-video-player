#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace vp::subtitle {

enum class SubtitleFormat {
    Unknown,
    Srt,
    Ass,
    Ssa,
};

struct SubtitleColor {
    constexpr SubtitleColor(uint8_t r_value = 255,
                            uint8_t g_value = 255,
                            uint8_t b_value = 255,
                            uint8_t a_value = 255)
        : r(r_value), g(g_value), b(b_value), a(a_value) {}

    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
};

bool operator==(const SubtitleColor& lhs, const SubtitleColor& rhs);
bool operator!=(const SubtitleColor& lhs, const SubtitleColor& rhs);

struct SubtitleStyle {
    std::string style_name;
    std::string font_family{"Microsoft YaHei UI"};
    double font_size{36.0};
    bool bold{false};
    bool italic{false};
    bool underline{false};
    bool strikeout{false};
    SubtitleColor primary_color{};
    SubtitleColor secondary_color{255, 255, 0, 255};
    SubtitleColor outline_color{0, 0, 0, 255};
    SubtitleColor background_color{0, 0, 0, 0};
    int alignment{2};
    int margin_l{20};
    int margin_r{20};
    int margin_v{20};
    int border_style{1};
    double outline{2.0};
    double shadow{2.0};
    double outline_x{2.0};
    double outline_y{2.0};
    double shadow_x{2.0};
    double shadow_y{2.0};
    int wrap_style{0};
    double spacing{0.0};
    double scale_x_percent{100.0};
    double scale_y_percent{100.0};
    double rotation_degrees{0.0};
    double rotation_x_degrees{0.0};
    double rotation_y_degrees{0.0};
    double shear_x{0.0};
    double shear_y{0.0};
    bool has_rotation_origin{false};
    double rotation_origin_x{0.0};
    double rotation_origin_y{0.0};
    bool has_clip{false};
    bool inverse_clip{false};
    double clip_x1{0.0};
    double clip_y1{0.0};
    double clip_x2{0.0};
    double clip_y2{0.0};
    bool has_vector_clip{false};
    int vector_clip_scale{1};
    std::string vector_clip_commands;
    bool has_position{false};
    double position_x{0.0};
    double position_y{0.0};
};

bool operator==(const SubtitleStyle& lhs, const SubtitleStyle& rhs);
bool operator!=(const SubtitleStyle& lhs, const SubtitleStyle& rhs);

constexpr uint64_t kSubtitleTransitionFontSize = 1ull << 0;
constexpr uint64_t kSubtitleTransitionPrimaryColor = 1ull << 1;
constexpr uint64_t kSubtitleTransitionSecondaryColor = 1ull << 2;
constexpr uint64_t kSubtitleTransitionOutlineColor = 1ull << 3;
constexpr uint64_t kSubtitleTransitionBackgroundColor = 1ull << 4;
constexpr uint64_t kSubtitleTransitionOutlineX = 1ull << 5;
constexpr uint64_t kSubtitleTransitionOutlineY = 1ull << 6;
constexpr uint64_t kSubtitleTransitionShadowX = 1ull << 7;
constexpr uint64_t kSubtitleTransitionShadowY = 1ull << 8;
constexpr uint64_t kSubtitleTransitionSpacing = 1ull << 9;
constexpr uint64_t kSubtitleTransitionScaleX = 1ull << 10;
constexpr uint64_t kSubtitleTransitionScaleY = 1ull << 11;
constexpr uint64_t kSubtitleTransitionRotationZ = 1ull << 12;
constexpr uint64_t kSubtitleTransitionRotationX = 1ull << 13;
constexpr uint64_t kSubtitleTransitionRotationY = 1ull << 14;
constexpr uint64_t kSubtitleTransitionShearX = 1ull << 15;
constexpr uint64_t kSubtitleTransitionShearY = 1ull << 16;
constexpr uint64_t kSubtitleTransitionRotationOrigin = 1ull << 17;

struct SubtitleStyleTransition {
    bool has_timing{false};
    double start_seconds{0.0};
    double end_seconds{0.0};
    double accel{1.0};
    uint64_t property_mask{0};
    SubtitleStyle target_style;
};

bool operator==(const SubtitleStyleTransition& lhs, const SubtitleStyleTransition& rhs);
bool operator!=(const SubtitleStyleTransition& lhs, const SubtitleStyleTransition& rhs);

enum class SubtitleKaraokeMode {
    None,
    Instant,
    Sweep,
};

enum class SubtitleFadeMode {
    None,
    Simple,
    Complex,
};

struct SubtitleStyleAnimation {
    bool has_move{false};
    bool move_has_timing{false};
    double move_x1{0.0};
    double move_y1{0.0};
    double move_x2{0.0};
    double move_y2{0.0};
    double move_t1_seconds{0.0};
    double move_t2_seconds{0.0};
    SubtitleFadeMode fade_mode{SubtitleFadeMode::None};
    double fade_in_seconds{0.0};
    double fade_out_seconds{0.0};
    uint8_t fade_alpha1{255};
    uint8_t fade_alpha2{255};
    uint8_t fade_alpha3{255};
    double fade_t1_seconds{0.0};
    double fade_t2_seconds{0.0};
    double fade_t3_seconds{0.0};
    double fade_t4_seconds{0.0};
    std::vector<SubtitleStyleTransition> style_transitions;
};

bool operator==(const SubtitleStyleAnimation& lhs, const SubtitleStyleAnimation& rhs);
bool operator!=(const SubtitleStyleAnimation& lhs, const SubtitleStyleAnimation& rhs);

struct SubtitleTextRun {
    size_t start{0};
    size_t length{0};
    SubtitleStyle style;
    SubtitleKaraokeMode karaoke_mode{SubtitleKaraokeMode::None};
    int karaoke_start_centiseconds{0};
    int karaoke_end_centiseconds{0};
};

bool operator==(const SubtitleTextRun& lhs, const SubtitleTextRun& rhs);
bool operator!=(const SubtitleTextRun& lhs, const SubtitleTextRun& rhs);

struct SubtitleBitmap {
    int x{0};
    int y{0};
    int width{0};
    int height{0};
    std::vector<uint8_t> rgba;
};

bool operator==(const SubtitleBitmap& lhs, const SubtitleBitmap& rhs);
bool operator!=(const SubtitleBitmap& lhs, const SubtitleBitmap& rhs);

struct SubtitleItem {
    int index{0};
    int layer{0};
    double start_seconds{0.0};
    double end_seconds{0.0};
    std::string source_path;
    std::string text;
    std::string raw_text;
    int play_res_x{0};
    int play_res_y{0};
    SubtitleStyle style;
    SubtitleStyleAnimation animation;
    bool is_vector_drawing{false};
    int drawing_scale{1};
    std::string drawing_commands;
    bool is_bitmap{false};
    std::vector<SubtitleBitmap> bitmap_rects;
    std::vector<SubtitleTextRun> runs;
};

bool operator==(const SubtitleItem& lhs, const SubtitleItem& rhs);
bool operator!=(const SubtitleItem& lhs, const SubtitleItem& rhs);

class ISubtitleParser {
public:
    virtual ~ISubtitleParser() = default;

    virtual bool parseFile(const std::string& file_path) = 0;
    virtual const std::vector<SubtitleItem>& items() const = 0;
    virtual SubtitleFormat format() const = 0;
};

bool isSupportedSubtitleExtension(const std::string& extension_or_path);
std::unique_ptr<ISubtitleParser> createParserForPath(const std::string& file_path);
std::string flattenSubtitleText(const std::vector<SubtitleItem>& items);

}  // namespace vp::subtitle
