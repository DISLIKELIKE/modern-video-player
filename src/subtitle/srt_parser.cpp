#include "subtitle/srt_parser.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

namespace vp::subtitle {

namespace {

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

size_t countUtf8CodePoints(const std::string& text) {
    size_t count = 0;
    for (unsigned char ch : text) {
        if ((ch & 0xC0u) != 0x80u) {
            ++count;
        }
    }
    return count;
}

SubtitleStyle makeDefaultSrtStyle() {
    SubtitleStyle style;
    style.style_name = "srt-default";
    style.font_family = "Microsoft YaHei UI";
    style.font_size = 36.0;
    style.bold = true;
    style.primary_color = SubtitleColor(255, 255, 255, 250);
    style.outline_color = SubtitleColor(0, 0, 0, 255);
    style.background_color = SubtitleColor(5, 5, 5, 148);
    style.alignment = 2;
    style.margin_l = 24;
    style.margin_r = 24;
    style.margin_v = 24;
    style.border_style = 3;
    style.outline = 0.0;
    style.shadow = 2.0;
    return style;
}

}  // namespace

bool SrtParser::parseFile(const std::string& file_path) {
    items_.clear();

    std::ifstream input(file_path, std::ios::binary);
    if (!input.good()) {
        return false;
    }

    const SubtitleStyle default_style = makeDefaultSrtStyle();
    std::string line;
    bool first_line = true;
    while (std::getline(input, line)) {
        if (first_line && line.size() >= 3 &&
            static_cast<unsigned char>(line[0]) == 0xEF &&
            static_cast<unsigned char>(line[1]) == 0xBB &&
            static_cast<unsigned char>(line[2]) == 0xBF) {
            line.erase(0, 3);
        }
        first_line = false;

        const std::string index_line = trimCopy(line);
        if (index_line.empty()) {
            continue;
        }

        SubtitleItem item{};
        try {
            item.index = std::stoi(index_line);
        } catch (...) {
            continue;
        }

        std::string timeline;
        if (!std::getline(input, timeline)) {
            break;
        }
        const std::string trimmed_timeline = trimCopy(timeline);
        const size_t arrow = trimmed_timeline.find("-->");
        if (arrow == std::string::npos) {
            continue;
        }

        const std::string start_text = trimCopy(trimmed_timeline.substr(0, arrow));
        const std::string end_text = trimCopy(trimmed_timeline.substr(arrow + 3));
        if (!parseTimecode(start_text, item.start_seconds) || !parseTimecode(end_text, item.end_seconds)) {
            continue;
        }

        std::ostringstream content;
        bool first_content_line = true;
        while (std::getline(input, line)) {
            if (trimCopy(line).empty()) {
                break;
            }
            if (!first_content_line) {
                content << '\n';
            }
            first_content_line = false;
            content << line;
        }

        item.text = content.str();
        item.raw_text = item.text;
        item.style = default_style;
        const size_t visible_length = countUtf8CodePoints(item.text);
        if (visible_length > 0) {
            item.runs.push_back(SubtitleTextRun{0, visible_length, item.style});
        }
        items_.push_back(std::move(item));
    }

    return !items_.empty();
}

const std::vector<SubtitleItem>& SrtParser::items() const {
    return items_;
}

SubtitleFormat SrtParser::format() const {
    return SubtitleFormat::Srt;
}

bool SrtParser::parseTimecode(const std::string& text, double& seconds) {
    int hh = 0;
    int mm = 0;
    int ss = 0;
    int ms = 0;

#if defined(_WIN32)
    if (::sscanf_s(text.c_str(), "%d:%d:%d,%d", &hh, &mm, &ss, &ms) != 4) {
#else
    if (std::sscanf(text.c_str(), "%d:%d:%d,%d", &hh, &mm, &ss, &ms) != 4) {
#endif
        return false;
    }
    if (hh < 0 || mm < 0 || mm >= 60 || ss < 0 || ss >= 60 || ms < 0 || ms >= 1000) {
        return false;
    }

    seconds = static_cast<double>(hh) * 3600.0 +
              static_cast<double>(mm) * 60.0 +
              static_cast<double>(ss) +
              static_cast<double>(ms) / 1000.0;
    return true;
}

}  // namespace vp::subtitle
