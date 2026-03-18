#pragma once

#include <vector>

#include "subtitle/subtitle_parser.h"

namespace vp::subtitle {

int resolveActiveSubtitleIndex(const std::vector<SubtitleItem>& items,
                               double position_seconds,
                               int active_index_hint = -1);

std::vector<int> collectActiveSubtitleIndices(const std::vector<SubtitleItem>& items,
                                              double position_seconds);

}  // namespace vp::subtitle
