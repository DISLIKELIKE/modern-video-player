#pragma once

#include <string>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/buffer.h>
#include <libavutil/pixfmt.h>
}

#include "decoder/decoder_capability.h"

namespace vp::platform {

struct HardwareDeviceCreateRequest {
    decoder::DecoderBackend backend{decoder::DecoderBackend::Software};
    void* native_device_handle{nullptr};
    const char* device_name{nullptr};
};

struct HardwareDeviceCreateResult {
    AVBufferRef* device_context{nullptr};
    std::string detail;
};

class HwDeviceFactory {
public:
    static bool supportsBackend(decoder::DecoderBackend backend);
    static bool findCodecHardwarePixelFormat(const AVCodec* codec,
                                             decoder::DecoderBackend backend,
                                             AVPixelFormat* out_pixel_format,
                                             std::string* detail = nullptr);
    static bool createHardwareDeviceContext(const HardwareDeviceCreateRequest& request,
                                            HardwareDeviceCreateResult* out_result);
    static bool probeRuntimeAvailability(decoder::DecoderBackend backend, std::string* detail = nullptr);
};

}  // namespace vp::platform
