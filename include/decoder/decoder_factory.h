#pragma once

#include <string>
#include <vector>

#include "decoder/decoder_capability.h"

namespace vp::decoder {

/// 解码器后端选择工厂；负责探测能力并给出后端优先级。
class DecoderFactory {
public:
    /// 返回当前平台可用的解码后端能力列表。
    static std::vector<DecoderCapability> probeCapabilities();
    /// 根据编码名与硬解偏好返回推荐后端顺序。
    static std::vector<DecoderBackend> selectBackendOrder(const std::string& codec_name, bool prefer_hardware);
    /// 返回最优先的单个解码后端。
    static DecoderBackend selectBestBackend(const std::string& codec_name, bool prefer_hardware);
    /// 返回后端名称字符串。
    static const char* backendName(DecoderBackend backend);
};

}  // namespace vp::decoder