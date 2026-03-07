#include "streaming/dash_manifest_parser.h"

#include <regex>

namespace vp::streaming {

bool DashManifestParser::parse(const std::string& text, DashManifest& manifest) {
    manifest.representations.clear();

    const std::regex rep_regex(
        R"dash(<Representation[^>]*id="([^"]+)"[^>]*bandwidth="([0-9]+)")dash");
    auto begin = std::sregex_iterator(text.begin(), text.end(), rep_regex);
    auto end = std::sregex_iterator();

    for (auto it = begin; it != end; ++it) {
        DashRepresentation rep{};
        rep.id = (*it)[1].str();
        rep.bandwidth = std::stoi((*it)[2].str());
        manifest.representations.push_back(std::move(rep));
    }

    return !manifest.representations.empty();
}

}  // namespace vp::streaming
