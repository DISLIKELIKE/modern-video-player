#include "filters/builtin_filters.h"

#include <memory>

#include "filters/filter_registry.h"

namespace vp::filters::builtin {

void registerBuiltinFilters() {
    auto& registry = FilterRegistry::instance();
    registry.registerVideoFilter("brightness", [] { return std::make_unique<BrightnessFilter>(); });
    registry.registerVideoFilter("contrast", [] { return std::make_unique<ContrastFilter>(); });
    registry.registerVideoFilter("saturation", [] { return std::make_unique<SaturationFilter>(); });
    registry.registerAudioFilter("volume_balance", [] { return std::make_unique<VolumeBalanceFilter>(); });
}

}  // namespace vp::filters::builtin
