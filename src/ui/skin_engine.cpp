#include "ui/skin_engine.h"

#include <utility>

namespace vp::ui {

void SkinEngine::setTheme(SkinTheme theme) {
    theme_ = std::move(theme);
}

const SkinTheme& SkinEngine::theme() const {
    return theme_;
}

std::string SkinEngine::resolve(const std::string& key, const std::string& fallback) const {
    const auto it = theme_.variables.find(key);
    if (it == theme_.variables.end()) {
        return fallback;
    }
    return it->second;
}

}  // namespace vp::ui
