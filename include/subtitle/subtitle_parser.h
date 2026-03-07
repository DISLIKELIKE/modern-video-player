#pragma once

#include <string>
#include <vector>

namespace vp::subtitle {

struct SubtitleItem {
    int index{0};
    double start_seconds{0.0};
    double end_seconds{0.0};
    std::string text;
};

class ISubtitleParser {
public:
    virtual ~ISubtitleParser() = default;
    virtual bool parseFile(const std::string& file_path) = 0;
    virtual const std::vector<SubtitleItem>& items() const = 0;
};

}  // namespace vp::subtitle

