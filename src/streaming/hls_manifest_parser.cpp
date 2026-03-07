#include "streaming/hls_manifest_parser.h"

#include <sstream>

namespace vp::streaming {

bool HlsManifestParser::parse(const std::string& text, HlsManifest& manifest) {
    manifest.segment_urls.clear();

    std::istringstream input(text);
    std::string line;
    while (std::getline(input, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        manifest.segment_urls.push_back(line);
    }

    return !manifest.segment_urls.empty();
}

}  // namespace vp::streaming

