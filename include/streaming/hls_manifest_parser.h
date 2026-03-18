#pragma once

#include <string>
#include <vector>

namespace vp::streaming {

struct HlsVariantStream {
    std::string uri;
    int bandwidth{0};
    std::string codecs;
    std::string resolution;
};

struct HlsManifest {
    bool is_master{false};
    std::vector<std::string> segment_urls;
    std::vector<HlsVariantStream> variants;
};

class HlsManifestParser {
public:
    static bool parse(const std::string& text, HlsManifest& manifest);
};

}  // namespace vp::streaming
