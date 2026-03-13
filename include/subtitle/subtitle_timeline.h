#pragma once

#include <vector>

#include "subtitle/subtitle_parser.h"

namespace vp::subtitle {

/// 根据播放时间解析当前应显示的字幕索引。
/// @param active_index_hint 上一次命中的字幕索引，可用于减少搜索成本。
/// @return 找到时返回字幕索引，否则返回 -1。
int resolveActiveSubtitleIndex(const std::vector<SubtitleItem>& items,
                               double position_seconds,
                               int active_index_hint = -1);

}  // namespace vp::subtitle