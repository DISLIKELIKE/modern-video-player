#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace vp::streaming {

/// 自适应流中的单个码率变体。
struct AdaptiveStreamVariant {
    // 变体标识（如清晰度标签或清单中的 id）。
    std::string id;
    // 名义码率（bps），<=0 视为不可用于正常选择。
    int bandwidth{0};
};

/// 自适应码率选择结果。
struct AdaptiveBitrateDecision {
    // 选中的变体索引（对应 variants）。
    size_t variant_index{0};
    // true 表示无可承载档位，已回退到最低码率。
    bool fallback_to_lowest{false};
};

/// 自适应码率选择器；根据估算带宽挑选最合适的变体。
class AdaptiveBitrateSelector {
public:
    // 基于估算带宽与安全系数选择最合适变体。
    // headroom_ratio 默认 0.85，用于预留网络波动空间。
    static AdaptiveBitrateDecision chooseVariant(
        const std::vector<AdaptiveStreamVariant>& variants,
        int estimated_bandwidth_bps,
        double headroom_ratio = 0.85);
};

}  // namespace vp::streaming
