#include "streaming/adaptive_bitrate_selector.h"

#include <algorithm>
#include <limits>

namespace vp::streaming {

AdaptiveBitrateDecision AdaptiveBitrateSelector::chooseVariant(
    const std::vector<AdaptiveStreamVariant>& variants,
    int estimated_bandwidth_bps,
    double headroom_ratio) {
    AdaptiveBitrateDecision decision{};
    if (variants.empty()) {
        return decision;
    }

    const double clamped_headroom = std::clamp(headroom_ratio, 0.1, 1.0);
    const double effective_bandwidth = std::max(0, estimated_bandwidth_bps) * clamped_headroom;

    size_t lowest_index = 0;
    int lowest_bandwidth = std::numeric_limits<int>::max();
    for (size_t i = 0; i < variants.size(); ++i) {
        const int bandwidth = variants[i].bandwidth > 0 ? variants[i].bandwidth : std::numeric_limits<int>::max();
        if (bandwidth < lowest_bandwidth) {
            lowest_bandwidth = bandwidth;
            lowest_index = i;
        }
    }

    bool found_fit = false;
    int best_bandwidth = -1;
    for (size_t i = 0; i < variants.size(); ++i) {
        const int bandwidth = variants[i].bandwidth;
        if (bandwidth <= 0 || static_cast<double>(bandwidth) > effective_bandwidth) {
            continue;
        }
        if (!found_fit || bandwidth > best_bandwidth) {
            found_fit = true;
            best_bandwidth = bandwidth;
            decision.variant_index = i;
        }
    }

    if (!found_fit) {
        decision.variant_index = lowest_index;
        decision.fallback_to_lowest = true;
    }

    return decision;
}

}  // namespace vp::streaming
