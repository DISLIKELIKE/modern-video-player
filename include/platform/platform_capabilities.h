#pragma once

#include <vector>

#include "decoder/decoder_capability.h"
#include "render/video_renderer.h"

namespace vp::platform {

enum class PlatformKind {
    Unknown,
    Windows,
    Linux,
    MacOS
};

struct RendererSupport {
    render::VideoRendererType type{render::VideoRendererType::Auto};
    bool compiled_in{false};
    bool runtime_available{false};
    int default_priority{0};
};

struct DecoderBackendSupport {
    decoder::DecoderBackend backend{decoder::DecoderBackend::Software};
    bool compiled_in{false};
    bool runtime_available{false};
    bool hardware_accelerated{false};
    int default_priority{0};
};

struct PlatformCapabilities {
    PlatformKind platform{PlatformKind::Unknown};
    std::vector<RendererSupport> renderer_support;
    std::vector<DecoderBackendSupport> decoder_support;
    bool has_sdl_windowing{false};
    bool has_sdl_audio{false};
};

class PlatformCapabilitiesProbe {
public:
    static PlatformCapabilities detect();
};

const char* platformKindName(PlatformKind kind);

}  // namespace vp::platform
