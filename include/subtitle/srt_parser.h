#pragma once

#include <vector>

#include "subtitle/subtitle_parser.h"

namespace vp::subtitle {

class SrtParser final : public ISubtitleParser {
public:
    bool parseFile(const std::string& file_path) override;
    const std::vector<SubtitleItem>& items() const override;

private:
    static bool parseTimecode(const std::string& text, double& seconds);
    std::vector<SubtitleItem> items_;
};

}  // namespace vp::subtitle

