#pragma once

#include <string>
#include <vector>

namespace vp::streaming {

struct DashRepresentation {
    std::string id;
    int bandwidth{0};
};

struct DashManifest {
    std::vector<DashRepresentation> representations;
};

class DashManifestParser {
public:
    static bool parse(const std::string& text, DashManifest& manifest);
};

}  // namespace vp::streaming

