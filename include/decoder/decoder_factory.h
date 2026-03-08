#pragma once

#include <string>
#include <vector>

#include "decoder/decoder_capability.h"

namespace vp::decoder {

class DecoderFactory {
public:
    static std::vector<DecoderCapability> probeCapabilities();
    static std::vector<DecoderBackend> selectBackendOrder(const std::string& codec_name, bool prefer_hardware);
    static DecoderBackend selectBestBackend(const std::string& codec_name, bool prefer_hardware);
    static const char* backendName(DecoderBackend backend);
};

}  // namespace vp::decoder

