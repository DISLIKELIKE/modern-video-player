#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "subtitle/subtitle_parser.h"

namespace vp::subtitle {

struct EmbeddedSubtitleTrackInfo {
    int stream_index{-1};
    std::string codec_name;
    std::string language;
    std::string title;
    bool is_default{false};
    bool is_forced{false};
    bool is_hearing_impaired{false};
    bool supported_text_codec{false};
    bool supported_bitmap_codec{false};
    bool supported_codec{false};
    int preference_score{0};
};

struct EmbeddedSubtitleLoadResult {
    bool subtitle_stream_found{false};
    bool supported_stream_found{false};
    bool loaded{false};
    int stream_index{-1};
    std::string codec_name;
    std::string language;
    std::string title;
    std::string source_label;
    bool bitmap_codec{false};
    size_t bitmap_item_count{0};
    std::vector<SubtitleItem> items;
};

std::vector<EmbeddedSubtitleTrackInfo> listEmbeddedSubtitleTracks(const std::string& media_source_path);
EmbeddedSubtitleLoadResult loadEmbeddedSubtitleTrack(const std::string& media_source_path, int stream_index);
int selectBestEmbeddedSubtitleStream(const std::vector<EmbeddedSubtitleTrackInfo>& tracks);
EmbeddedSubtitleLoadResult loadBestEmbeddedSubtitleTrack(const std::string& media_source_path);

}  // namespace vp::subtitle
