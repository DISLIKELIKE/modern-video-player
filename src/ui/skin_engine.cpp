#include "ui/skin_engine.h"

#include <utility>

namespace vp::ui {

/// 替换当前主题对象；后续变量解析都基于新主题。
void SkinEngine::setTheme(SkinTheme theme) {
    theme_ = std::move(theme);
}

/// 返回当前主题对象的只读视图。
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
