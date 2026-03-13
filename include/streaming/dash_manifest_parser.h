#pragma once

#include <string>
#include <vector>

namespace vp::streaming {

/// DASH 清单中的单个码率表示。
struct DashRepresentation {
    std::string id;
    int bandwidth{0};
    std::string base_url;
    std::string initialization_url;
    std::vector<std::string> segment_urls;
};

/// DASH 清单解析结果。
struct DashManifest {
    std::string base_url;
    std::vector<DashRepresentation> representations;
};

/// DASH MPD 文本解析器。
class DashManifestParser {
public:
    /// 从清单文本中提取基础 URL、码率表示和分片 URL。
    static bool parse(const std::string& text, DashManifest& manifest);
};

}  // namespace vp::streaming