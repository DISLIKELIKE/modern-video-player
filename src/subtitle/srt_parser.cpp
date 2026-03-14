#include "subtitle/srt_parser.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

namespace vp::subtitle {

namespace {

/// 去掉字幕行两端空白，避免编号和时间码解析受空格影响。
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

}  // namespace

/// 逐块解析 SRT 文本，生成按出现顺序排列的字幕条目。
bool SrtParser::parseFile(const std::string& file_path) {
    items_.clear();

    std::ifstream input(file_path);
    if (!input.good()) {
        return false;
    }

    std::string line;
    while (std::getline(input, line)) {
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
        bool first_line = true;
        while (std::getline(input, line)) {
            if (trimCopy(line).empty()) {
                break;
            }
            if (!first_line) {
                content << '\n';
            }
            first_line = false;
            content << line;
        }
        item.text = content.str();
        items_.push_back(std::move(item));
    }

    return !items_.empty();
}

/// 返回最近一次解析得到的字幕条目集合。
const std::vector<SubtitleItem>& SrtParser::items() const {
    return items_;
}

/// 解析单个 SRT 时间码文本并换算为秒。
bool SrtParser::parseTimecode(const std::string& text, double& seconds) {
    int hh = 0;
    int mm = 0;
    int ss = 0;
    int ms = 0;

    if (std::sscanf(text.c_str(), "%d:%d:%d,%d", &hh, &mm, &ss, &ms) != 4) {
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
