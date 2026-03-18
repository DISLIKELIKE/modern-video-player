#pragma once

#include <string>
#include <vector>

#include "subtitle/subtitle_parser.h"

namespace vp::subtitle {

class AssParser final : public ISubtitleParser {
public:
    bool parseFile(const std::string& file_path) override;
    const std::vector<SubtitleItem>& items() const override;
    SubtitleFormat format() const override;

private:
    std::vector<SubtitleItem> items_;
};

}  // namespace vp::subtitle
