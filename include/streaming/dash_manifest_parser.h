#pragma once

#include <string>
#include <vector>

namespace vp::streaming {

struct DashRepresentation {
    std::string id;
    int bandwidth{0};
    std::string base_url;
    std::string initialization_url;
    std::vector<std::string> segment_urls;
};

struct DashManifest {
    std::string base_url;
    std::vector<DashRepresentation> representations;
};

class DashManifestParser {
public:
    static bool parse(const std::string& text, DashManifest& manifest);
};

}  // namespace vp::streaming

