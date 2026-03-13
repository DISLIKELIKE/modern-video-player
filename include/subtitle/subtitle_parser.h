#pragma once

#include <string>
#include <vector>

namespace vp::subtitle {

/// 单条字幕条目。
struct SubtitleItem {
    int index{0};
    double start_seconds{0.0};
    double end_seconds{0.0};
    std::string text;
};

/// 字幕解析器抽象接口。
class ISubtitleParser {
public:
    virtual ~ISubtitleParser() = default;
    /// 解析指定字幕文件；成功后可通过 `items()` 读取结果。
    virtual bool parseFile(const std::string& file_path) = 0;
    /// 返回最近一次解析结果。
    virtual const std::vector<SubtitleItem>& items() const = 0;
};

}  // namespace vp::subtitle