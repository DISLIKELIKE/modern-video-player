#include "streaming/dash_manifest_parser.h"

#include <algorithm>
#include <regex>

namespace vp::streaming {

namespace {

/// 去掉 XML 片段首尾空白，减少正则匹配后的噪声。
std::string trim(std::string value) {
    const size_t begin = value.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) {
        return {};
    }
    const size_t end = value.find_last_not_of(" \t\r\n");
    return value.substr(begin, end - begin + 1);
}

/// 查找首个指定 XML 标签的文本内容。
std::string findFirstTagValue(const std::string& text, const std::string& tag_name) {
    const std::regex tag_regex("<" + tag_name + R"(>([\s\S]*?)</)" + tag_name + ">", std::regex::icase);
    std::smatch match;
    if (!std::regex_search(text, match, tag_regex)) {
        return {};
    }
    return trim(match[1].str());
}

/// 从 DASH 节点属性串中提取指定属性值。
std::string extractAttribute(const std::string& text, const std::string& key) {
    const std::regex attribute_regex(key + "\\s*=\\s*\"([^\"]+)\"", std::regex::icase);
    std::smatch match;
    if (!std::regex_search(text, match, attribute_regex)) {
        return {};
    }
    return match[1].str();
}

/// 解析非负整数属性；非法输入统一回退为 0。
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

/// 从 DASH MPD 文本中提取表示列表、初始化段和媒体分片 URL。
bool DashManifestParser::parse(const std::string& text, DashManifest& manifest) {
    manifest.base_url = findFirstTagValue(text, "BaseURL");
    manifest.representations.clear();

    const std::regex rep_regex(R"dash(<Representation\b([^>]*)>([\s\S]*?)</Representation>)dash", std::regex::icase);
    auto begin = std::sregex_iterator(text.begin(), text.end(), rep_regex);
    auto end = std::sregex_iterator();

    for (auto it = begin; it != end; ++it) {
        DashRepresentation rep{};
        const std::string attributes = (*it)[1].str();
        const std::string body = (*it)[2].str();

        rep.id = extractAttribute(attributes, "id");
        rep.bandwidth = parsePositiveInt(extractAttribute(attributes, "bandwidth"));
        rep.base_url = findFirstTagValue(body, "BaseURL");

        const std::regex initialization_regex(R"dash(<Initialization[^>]*sourceURL="([^"]+)")dash", std::regex::icase);
        std::smatch initialization_match;
        if (std::regex_search(body, initialization_match, initialization_regex)) {
            rep.initialization_url = initialization_match[1].str();
        }

        const std::regex segment_regex(R"dash(<SegmentURL[^>]*media="([^"]+)")dash", std::regex::icase);
        auto segment_begin = std::sregex_iterator(body.begin(), body.end(), segment_regex);
        auto segment_end = std::sregex_iterator();
        for (auto segment_it = segment_begin; segment_it != segment_end; ++segment_it) {
            rep.segment_urls.push_back((*segment_it)[1].str());
        }

        if (!rep.id.empty() && rep.bandwidth > 0) {
            manifest.representations.push_back(std::move(rep));
        }
    }

    return !manifest.representations.empty();
}

}  // namespace vp::streaming
