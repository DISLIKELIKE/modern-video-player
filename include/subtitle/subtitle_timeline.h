#pragma once

#include <vector>

#include "subtitle/subtitle_parser.h"

namespace vp::subtitle {

int resolveActiveSubtitleIndex(const std::vector<SubtitleItem>& items,
                               double position_seconds,
                               int active_index_hint = -1);

}  // namespace vp::subtitle
