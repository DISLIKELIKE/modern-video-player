#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace vp::streaming {

struct AdaptiveStreamVariant {
    std::string id;
    int bandwidth{0};
};

struct AdaptiveBitrateDecision {
    size_t variant_index{0};
    bool fallback_to_lowest{false};
};

class AdaptiveBitrateSelector {
public:
    static AdaptiveBitrateDecision chooseVariant(
        const std::vector<AdaptiveStreamVariant>& variants,
        int estimated_bandwidth_bps,
        double headroom_ratio = 0.85);
};

}  // namespace vp::streaming
