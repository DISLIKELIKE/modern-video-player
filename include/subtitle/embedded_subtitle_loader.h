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

enum class EmbeddedSubtitleForcedPolicy {
    Auto = 0,
    PreferForced,
    AvoidForced,
    OnlyForced
};

enum class EmbeddedSubtitleSdhPolicy {
    Auto = 0,
    PreferSdh,
    AvoidSdh,
    OnlySdh
};

struct EmbeddedSubtitleSelectionPolicy {
    std::vector<std::string> preferred_languages;
    EmbeddedSubtitleForcedPolicy forced_policy{EmbeddedSubtitleForcedPolicy::Auto};
    EmbeddedSubtitleSdhPolicy sdh_policy{EmbeddedSubtitleSdhPolicy::Auto};
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
    size_t bitmap_rect_count{0};
    size_t bitmap_multi_rect_item_count{0};
    size_t bitmap_max_rects_per_item{0};
    std::vector<SubtitleItem> items;
};

struct EmbeddedSubtitleLivePacketProbeResult {
    bool input_opened{false};
    bool subtitle_stream_found{false};
    bool supported_stream_found{false};
    bool decoder_opened{false};
    bool eof_reached{false};
    int stream_index{-1};
    std::string codec_name;
    size_t max_packets{0};
    size_t packets_read{0};
    size_t subtitle_packets_read{0};
    size_t decoded_text_events{0};
    size_t decoded_bitmap_rects{0};
    bool monotonic_timestamps{true};
    bool produced_output{false};
};

std::vector<EmbeddedSubtitleTrackInfo> listEmbeddedSubtitleTracks(const std::string& media_source_path);
EmbeddedSubtitleLoadResult loadEmbeddedSubtitleTrack(const std::string& media_source_path, int stream_index);
int selectBestEmbeddedSubtitleStream(const std::vector<EmbeddedSubtitleTrackInfo>& tracks);
int selectBestEmbeddedSubtitleStream(const std::vector<EmbeddedSubtitleTrackInfo>& tracks,
                                     const EmbeddedSubtitleSelectionPolicy& policy);
EmbeddedSubtitleLoadResult loadBestEmbeddedSubtitleTrack(const std::string& media_source_path);
EmbeddedSubtitleLivePacketProbeResult probeEmbeddedSubtitleLivePacketPath(const std::string& media_source_path,
                                                                          int stream_index,
                                                                          size_t max_packets = 120);

}  // namespace vp::subtitle
