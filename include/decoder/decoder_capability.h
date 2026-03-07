#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace vp::decoder {

enum class DecoderBackend {
    Software,
    CUDA,
    D3D11VA
};

struct DecoderCapability {
    DecoderBackend backend{DecoderBackend::Software};
    bool hardware_accelerated{false};
    int priority{0};
    std::vector<std::string> supported_codecs;
};

class IDecoder {
public:
    virtual ~IDecoder() = default;
    virtual DecoderBackend backend() const = 0;
    virtual bool supportsCodec(std::string_view codec_name) const = 0;
};

}  // namespace vp::decoder

