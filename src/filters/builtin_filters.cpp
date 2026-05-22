#include "filters/builtin_filters.h"

#include <memory>

#include "filters/filter_registry.h"

namespace vp::filters::builtin {

/// 将内建亮度、对比度、饱和度和音量滤镜注册到全局表。
void registerBuiltinFilters() {
    auto& registry = FilterRegistry::instance();
    registry.registerVideoFilter("brightness", [] { return std::make_unique<BrightnessFilter>(); });
    registry.registerVideoFilter("contrast", [] { return std::make_unique<ContrastFilter>(); });
    registry.registerVideoFilter("saturation", [] { return std::make_unique<SaturationFilter>(); });
    registry.registerAudioFilter("volume_balance", [] { return std::make_unique<VolumeBalanceFilter>(); });
}

}  // namespace vp::filters::builtin
