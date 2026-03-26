#pragma once

#include <string>
#include <vector>

#include "decoder/decoder_capability.h"
#include "platform/platform_capabilities.h"
#include "render/video_renderer.h"

namespace vp::decoder {

struct DecoderSelectionContext {
    std::string codec_name;
    bool prefer_hardware{true};
    render::VideoRendererType renderer_type{render::VideoRendererType::Auto};
    platform::PlatformCapabilities platform_capabilities{};
};

class DecoderFactory {
public:
    static std::vector<DecoderCapability> probeCapabilities(const platform::PlatformCapabilities& capabilities);
    static std::vector<DecoderBackend> selectBackendOrder(const DecoderSelectionContext& context);
    static DecoderBackend selectBestBackend(const DecoderSelectionContext& context);
    static const char* backendName(DecoderBackend backend);
};

}  // namespace vp::decoder
