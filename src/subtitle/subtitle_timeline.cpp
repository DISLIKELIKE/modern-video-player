#include "subtitle/subtitle_timeline.h"

#include <algorithm>
#include <iterator>

namespace vp::subtitle {

namespace {

/// 判断当前位置是否仍落在某条字幕的显示时间区间内。
bool isWithinSubtitleRange(const SubtitleItem& item, double position_seconds) {
    return position_seconds >= item.start_seconds && position_seconds <= item.end_seconds;
}

}  // namespace

/// 根据播放位置查找当前命中的字幕索引；优先复用上一次命中结果。
int resolveActiveSubtitleIndex(const std::vector<SubtitleItem>& items,
                               double position_seconds,
                               int active_index_hint) {
    if (items.empty()) {
        return -1;
    }

    const auto within_item = [&](int index) -> bool {
        if (index < 0 || index >= static_cast<int>(items.size())) {
            return false;
        }
        return isWithinSubtitleRange(items[static_cast<size_t>(index)], position_seconds);
    };

    if (within_item(active_index_hint)) {
        return active_index_hint;
    }

    auto it = std::upper_bound(
        items.begin(),
        items.end(),
        position_seconds,
        [](double value, const SubtitleItem& item) {
            return value < item.start_seconds;
        });

    if (it == items.begin()) {
        return -1;
    }

    --it;
    if (!isWithinSubtitleRange(*it, position_seconds)) {
        return -1;
    }

    return static_cast<int>(std::distance(items.begin(), it));
}

}  // namespace vp::subtitle
