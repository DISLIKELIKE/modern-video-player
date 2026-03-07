#include "decoder/decoder_factory.h"

#include <algorithm>

namespace vp::decoder {

namespace {

bool supportsCodec(const DecoderCapability& capability, const std::string& codec_name) {
    return std::any_of(capability.supported_codecs.begin(),
                       capability.supported_codecs.end(),
                       [&codec_name](const std::string& candidate) {
                           return candidate == codec_name;
                       });
}

}  // namespace

std::vector<DecoderCapability> DecoderFactory::probeCapabilities() {
    std::vector<DecoderCapability> capabilities;

    DecoderCapability software{};
    software.backend = DecoderBackend::Software;
    software.hardware_accelerated = false;
    software.priority = 10;
    software.supported_codecs = {"h264", "hevc", "vp9", "av1", "aac", "mp3", "opus"};
    capabilities.push_back(std::move(software));

#if defined(_WIN32)
    DecoderCapability d3d11{};
    d3d11.backend = DecoderBackend::D3D11VA;
    d3d11.hardware_accelerated = true;
    d3d11.priority = 100;
    d3d11.supported_codecs = {"h264", "hevc", "vp9", "av1"};
    capabilities.push_back(std::move(d3d11));
#endif

    return capabilities;
}

DecoderBackend DecoderFactory::selectBestBackend(const std::string& codec_name, bool prefer_hardware) {
    const std::vector<DecoderCapability> capabilities = probeCapabilities();

    DecoderBackend fallback = DecoderBackend::Software;
    int best_priority = -1;

    for (const DecoderCapability& capability : capabilities) {
        if (!supportsCodec(capability, codec_name)) {
            continue;
        }
        if (!prefer_hardware && capability.hardware_accelerated) {
            continue;
        }
        if (capability.priority > best_priority) {
            best_priority = capability.priority;
            fallback = capability.backend;
        }
    }

    return fallback;
}

const char* DecoderFactory::backendName(DecoderBackend backend) {
    switch (backend) {
        case DecoderBackend::Software:
            return "Software";
        case DecoderBackend::CUDA:
            return "CUDA";
        case DecoderBackend::D3D11VA:
            return "D3D11VA";
        default:
            return "Unknown";
    }
}

}  // namespace vp::decoder

