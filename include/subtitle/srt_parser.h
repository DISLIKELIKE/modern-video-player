#pragma once

#include <vector>

#include "subtitle/subtitle_parser.h"

namespace vp::subtitle {

/// SRT 字幕解析器；将文本文件转换为按时间排序的字幕条目。
class SrtParser final : public ISubtitleParser {
public:
    /// 解析 `.srt` 文件内容。
    bool parseFile(const std::string& file_path) override;
    /// 返回解析后的字幕条目集合。
    const std::vector<SubtitleItem>& items() const override;

private:
    /// 解析 `HH:MM:SS,mmm` 时间码。
    static bool parseTimecode(const std::string& text, double& seconds);
    std::vector<SubtitleItem> items_;
};

}  // namespace vp::subtitle