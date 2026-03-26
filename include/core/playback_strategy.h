#pragma once

#include <string>
#include <vector>

#include "decoder/decoder_capability.h"
#include "demuxer.h"
#include "platform/platform_capabilities.h"
#include "render/video_renderer.h"

namespace vp::core {

struct PlaybackPreferences {
    bool prefer_hardware_decode{true};
    render::VideoRendererType preferred_renderer{render::VideoRendererType::Auto};
};

struct PlaybackOpenRequest {
    MediaInfo media_info;
    std::string video_codec_name;
    PlaybackPreferences preferences;
    platform::PlatformCapabilities platform_capabilities;
};

struct PlaybackOpenPlan {
    std::vector<render::VideoRendererType> renderer_candidates;
    std::vector<decoder::DecoderBackend> video_decoder_candidates;
    bool allow_hardware_decode{true};
    std::string renderer_plan_reason{"default"};
    std::string decoder_plan_reason{"default"};
};

class PlaybackStrategy {
public:
    static PlaybackOpenPlan buildOpenPlan(const PlaybackOpenRequest& request);
};

}  // namespace vp::core
