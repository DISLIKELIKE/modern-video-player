#include "streaming/hls_manifest_parser.h"

#include <regex>
#include <sstream>

namespace vp::streaming {

namespace {

std::string trim(std::string value) {
    const size_t begin = value.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) {
        return {};
    }
    const size_t end = value.find_last_not_of(" \t\r\n");
    return value.substr(begin, end - begin + 1);
}

std::string extractAttribute(const std::string& text, const std::string& key) {
    const std::regex attribute_regex(key + R"(=([^,]+|"[^"]+"))", std::regex::icase);
    std::smatch match;
    if (!std::regex_search(text, match, attribute_regex)) {
        return {};
    }
    std::string value = trim(match[1].str());
    if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
        value = value.substr(1, value.size() - 2);
    }
    return value;
}

int parsePositiveInt(const std::string& value) {
    if (value.empty()) {
        return 0;
    }
    try {
        return std::max(0, std::stoi(value));
    } catch (...) {
        return 0;
    }
}

}  // namespace

/// 解析 HLS 主清单或媒体清单，并识别变体流与分片 URL。
bool HlsManifestParser::parse(const std::string& text, HlsManifest& manifest) {
    manifest.is_master = false;
    manifest.segment_urls.clear();
    manifest.variants.clear();

    std::istringstream input(text);
    std::string line;
    bool awaiting_variant_uri = false;
    HlsVariantStream pending_variant{};
    while (std::getline(input, line)) {
        line = trim(line);
        if (line.empty()) {
            continue;
        }

        if (awaiting_variant_uri && line[0] != '#') {
            pending_variant.uri = line;
            manifest.variants.push_back(std::move(pending_variant));
            pending_variant = HlsVariantStream{};
            awaiting_variant_uri = false;
            manifest.is_master = true;
            continue;
        }

        if (line.rfind("#EXT-X-STREAM-INF:", 0) == 0) {
            awaiting_variant_uri = true;
            pending_variant = HlsVariantStream{};
            const std::string attributes = line.substr(std::string("#EXT-X-STREAM-INF:").size());
            pending_variant.bandwidth = parsePositiveInt(extractAttribute(attributes, "BANDWIDTH"));
            pending_variant.codecs = extractAttribute(attributes, "CODECS");
            pending_variant.resolution = extractAttribute(attributes, "RESOLUTION");
            continue;
        }

        if (line[0] == '#') {
            continue;
        }

        manifest.segment_urls.push_back(line);
    }

    return !manifest.segment_urls.empty() || !manifest.variants.empty();
}

}  // namespace vp::streaming

