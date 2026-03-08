#include "decoder/decoder_factory.h"

#include <algorithm>
#include <unordered_set>

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

std::vector<DecoderBackend> DecoderFactory::selectBackendOrder(const std::string& codec_name, bool prefer_hardware) {
    const std::vector<DecoderCapability> capabilities = probeCapabilities();
    std::vector<DecoderCapability> candidates;
    candidates.reserve(capabilities.size());

    for (const DecoderCapability& capability : capabilities) {
        if (!supportsCodec(capability, codec_name)) {
            continue;
        }
        if (!prefer_hardware && capability.hardware_accelerated) {
            continue;
        }
        candidates.push_back(capability);
    }

    std::sort(candidates.begin(), candidates.end(), [](const DecoderCapability& lhs, const DecoderCapability& rhs) {
        return lhs.priority > rhs.priority;
    });

    std::vector<DecoderBackend> order;
    order.reserve(candidates.size() + 1);
    std::unordered_set<int> seen;

    for (const DecoderCapability& capability : candidates) {
        const int key = static_cast<int>(capability.backend);
        if (seen.insert(key).second) {
            order.push_back(capability.backend);
        }
    }

    const int software_key = static_cast<int>(DecoderBackend::Software);
    if (seen.find(software_key) == seen.end()) {
        order.push_back(DecoderBackend::Software);
    }

    return order;
}

DecoderBackend DecoderFactory::selectBestBackend(const std::string& codec_name, bool prefer_hardware) {
    const std::vector<DecoderBackend> order = selectBackendOrder(codec_name, prefer_hardware);
    if (order.empty()) {
        return DecoderBackend::Software;
    }
    return order.front();
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

