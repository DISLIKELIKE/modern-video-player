#pragma once

#include <cstddef>
#include <string>

namespace vp::subtitle {

struct LibassShapingProbeSummary {
    bool supported_platform{false};
    bool file_accessible{false};
    bool parser_supported_extension{false};
    bool library_initialized{false};
    bool renderer_initialized{false};
    bool track_loaded{false};
    bool events_loaded{false};
    bool rendered_output{false};
    size_t sampled_timestamps{0};
    size_t changed_frames{0};
    size_t rendered_images{0};
    int max_image_width{0};
    int max_image_height{0};
    std::string error;
};

LibassShapingProbeSummary probeLibassShaping(const std::string& subtitle_path,
                                             int frame_width = 1280,
                                             int frame_height = 720,
                                             int sample_duration_ms = 12000,
                                             int sample_step_ms = 250);

}  // namespace vp::subtitle

