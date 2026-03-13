#include "ui/skin_engine.h"

#include <utility>

namespace vp::ui {

void SkinEngine::setTheme(SkinTheme theme) {
    theme_ = std::move(theme);
}

const SkinTheme& SkinEngine::theme() const {
    return theme_;
}

/// 解析主题变量；变量缺失时回退到调用方给定默认值。
std::string SkinEngine::resolve(const std::string& key, const std::string& fallback) const {
    auto it = theme_.variables.find(key);
    if (it == theme_.variables.end()) {
        return fallback;
    }
    return it->second;
}

}  // namespace vp::ui
