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
    SubtitleColor outline_color{0, 0, 0, 255};
    SubtitleColor background_color{0, 0, 0, 0};
    int alignment{2};
    int margin_l{20};
    int margin_r{20};
    int margin_v{20};
    int border_style{1};
    double outline{2.0};
    double shadow{2.0};
    bool has_position{false};
    double position_x{0.0};
    double position_y{0.0};
};

bool operator==(const SubtitleStyle& lhs, const SubtitleStyle& rhs);
bool operator!=(const SubtitleStyle& lhs, const SubtitleStyle& rhs);

struct SubtitleTextRun {
    size_t start{0};
    size_t length{0};
    SubtitleStyle style;
};

bool operator==(const SubtitleTextRun& lhs, const SubtitleTextRun& rhs);
bool operator!=(const SubtitleTextRun& lhs, const SubtitleTextRun& rhs);

struct SubtitleItem {
    int index{0};
    int layer{0};
    double start_seconds{0.0};
    double end_seconds{0.0};
    std::string text;
    std::string raw_text;
    int play_res_x{0};
    int play_res_y{0};
    SubtitleStyle style;
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
