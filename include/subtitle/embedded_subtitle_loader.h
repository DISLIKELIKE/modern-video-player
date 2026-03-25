#pragma once

#include <string>
#include <vector>

#include "subtitle/subtitle_parser.h"

namespace vp::subtitle {

struct EmbeddedSubtitleLoadResult {
    bool subtitle_stream_found{false};
    bool supported_stream_found{false};
    bool loaded{false};
    int stream_index{-1};
    std::string codec_name;
    std::string language;
    std::string title;
    std::string source_label;
    std::vector<SubtitleItem> items;
};

EmbeddedSubtitleLoadResult loadBestEmbeddedSubtitleTrack(const std::string& media_source_path);

}  // namespace vp::subtitle
