#pragma once

#include <string>
#include <vector>

namespace vp::streaming {

struct HlsManifest {
    std::vector<std::string> segment_urls;
};

class HlsManifestParser {
public:
    static bool parse(const std::string& text, HlsManifest& manifest);
};

}  // namespace vp::streaming

