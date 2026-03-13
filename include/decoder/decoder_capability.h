#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace vp::decoder {

/// 视频解码后端类型。
enum class DecoderBackend {
    Software,
    CUDA,
    D3D11VA
};

/// 单个解码后端的能力描述。
struct DecoderCapability {
    DecoderBackend backend{DecoderBackend::Software};
    bool hardware_accelerated{false};
    int priority{0};
    std::vector<std::string> supported_codecs;
};

/// 解码器抽象接口；用于统一表达后端与编解码支持能力。
class IDecoder {
public:
    virtual ~IDecoder() = default;
    /// 返回解码器所属后端类型。
    virtual DecoderBackend backend() const = 0;
    /// 判断是否支持指定编解码器名称。
    virtual bool supportsCodec(std::string_view codec_name) const = 0;
};

}  // namespace vp::decoder