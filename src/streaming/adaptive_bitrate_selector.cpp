#include "streaming/adaptive_bitrate_selector.h"

#include <algorithm>
#include <limits>

namespace vp::streaming {

/// 根据带宽估计与安全余量选择最优码率变体。
AdaptiveBitrateDecision AdaptiveBitrateSelector::chooseVariant(
    const std::vector<AdaptiveStreamVariant>& variants,
    int estimated_bandwidth_bps,
    double headroom_ratio) {
    AdaptiveBitrateDecision decision{};
    if (variants.empty()) {
        return decision;
    }

    // 预留 headroom 给吞吐抖动，避免频繁“刚好够带宽”导致的码率抖动。
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
        // 在可承载集合里选最高码率，尽量提高画质。
        if (!found_fit || bandwidth > best_bandwidth) {
            found_fit = true;
            best_bandwidth = bandwidth;
            decision.variant_index = i;
        }
    }

    if (!found_fit) {
        // 没有任何可承载档位时，回退到最低码率以优先保证连续播放。
        decision.variant_index = lowest_index;
        decision.fallback_to_lowest = true;
    }

    return decision;
}

}  // namespace vp::streaming
