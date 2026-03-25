#pragma once

#include <istream>
#include <string>
#include <vector>

#include "subtitle/subtitle_parser.h"

namespace vp::subtitle {

class SrtParser final : public ISubtitleParser {
public:
    bool parseFile(const std::string& file_path) override;
    bool parseText(const std::string& content, const std::string& source_path);
    const std::vector<SubtitleItem>& items() const override;
    SubtitleFormat format() const override;

private:
    static bool parseTimecode(const std::string& text, double& seconds);
    bool parseStream(std::istream& input, const std::string& source_path);

    std::vector<SubtitleItem> items_;
};

}  // namespace vp::subtitle
