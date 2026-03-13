#pragma once

#include <string>
#include <vector>

namespace vp::streaming {

/// HLS 主清单中的单个变体流。
struct HlsVariantStream {
    std::string uri;
    int bandwidth{0};
    std::string codecs;
    std::string resolution;
};

/// HLS 清单解析结果；支持主清单和媒体分片清单。
struct HlsManifest {
    bool is_master{false};
    std::vector<std::string> segment_urls;
    std::vector<HlsVariantStream> variants;
};

/// HLS 播放列表文本解析器。
class HlsManifestParser {
public:
    /// 从清单文本中提取分片 URL 或变体流列表。
    static bool parse(const std::string& text, HlsManifest& manifest);
};

}  // namespace vp::streaming