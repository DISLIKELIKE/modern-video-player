#include "decoder/decoder_factory.h"

#include <algorithm>
#include <cctype>
#include <unordered_set>

namespace vp::decoder {

namespace {

std::string toLowerAscii(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return text;
}

bool supportsCodec(const DecoderCapability& capability, const std::string& codec_name) {
    return std::any_of(capability.supported_codecs.begin(),
                       capability.supported_codecs.end(),
                       [&codec_name](const std::string& candidate) {
                           return candidate == codec_name;
                       });
}

bool isBackendRuntimeAvailable(const platform::PlatformCapabilities& capabilities, DecoderBackend backend) {
    return std::any_of(capabilities.decoder_support.begin(),
                       capabilities.decoder_support.end(),
                       [backend](const platform::DecoderBackendSupport& support) {
                           return support.backend == backend && support.compiled_in && support.runtime_available;
                       });
}

}  // namespace

std::vector<DecoderCapability> DecoderFactory::probeCapabilities(const platform::PlatformCapabilities& capabilities) {
    std::vector<DecoderCapability> result;
    result.reserve(capabilities.decoder_support.size());

    for (const platform::DecoderBackendSupport& support : capabilities.decoder_support) {
        if (!support.compiled_in || !support.runtime_available) {
            continue;
        }

        DecoderCapability capability{};
        capability.backend = support.backend;
        capability.hardware_accelerated = support.hardware_accelerated;
        capability.priority = support.default_priority;

        switch (support.backend) {
        case DecoderBackend::D3D11VA:
            capability.supported_codecs = {"h264", "hevc", "vp9", "av1"};
            break;
        case DecoderBackend::DXVA2:
            capability.supported_codecs = {"h264", "hevc", "vp9"};
            break;
        case DecoderBackend::VAAPI:
            capability.supported_codecs = {"h264", "hevc", "vp9", "av1"};
            break;
        case DecoderBackend::VideoToolbox:
            capability.supported_codecs = {"h264", "hevc", "vp9"};
            break;
        case DecoderBackend::Software:
            capability.supported_codecs = {"h264", "hevc", "vp9", "av1", "aac", "mp3", "opus"};
            break;
        case DecoderBackend::CUDA:
        default:
            capability.supported_codecs.clear();
            break;
        }

        result.push_back(std::move(capability));
    }

    std::sort(result.begin(), result.end(), [](const DecoderCapability& lhs, const DecoderCapability& rhs) {
        return lhs.priority > rhs.priority;
    });
    return result;
}

std::vector<DecoderBackend> DecoderFactory::selectBackendOrder(const DecoderSelectionContext& context) {
    const std::string normalized_codec_name = toLowerAscii(context.codec_name);
    const std::vector<DecoderCapability> capabilities = probeCapabilities(context.platform_capabilities);

    std::vector<DecoderCapability> candidates;
    candidates.reserve(capabilities.size());
    for (const DecoderCapability& capability : capabilities) {
        if (!supportsCodec(capability, normalized_codec_name)) {
            continue;
        }
        if (!context.prefer_hardware && capability.hardware_accelerated) {
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

    // Keep software decode as mandatory fallback in all contexts.
    const int software_key = static_cast<int>(DecoderBackend::Software);
    if (seen.find(software_key) == seen.end() &&
        isBackendRuntimeAvailable(context.platform_capabilities, DecoderBackend::Software)) {
        order.push_back(DecoderBackend::Software);
    }

    return order;
}

DecoderBackend DecoderFactory::selectBestBackend(const DecoderSelectionContext& context) {
    const std::vector<DecoderBackend> order = selectBackendOrder(context);
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
    case DecoderBackend::DXVA2:
        return "DXVA2";
    case DecoderBackend::VAAPI:
        return "VAAPI";
    case DecoderBackend::VideoToolbox:
        return "VideoToolbox";
    default:
        return "Unknown";
    }
}

}  // namespace vp::decoder
