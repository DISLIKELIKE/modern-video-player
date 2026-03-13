#pragma once

#include <string>
#include <unordered_map>

namespace vp::ui {

/// UI 主题变量集合。
struct SkinTheme {
    std::string name;
    std::unordered_map<std::string, std::string> variables;
};

/// 皮肤引擎；根据主题变量解析 UI 样式值。
class SkinEngine {
public:
    /// 设置当前主题。
    void setTheme(SkinTheme theme);
    /// 返回当前主题。
    const SkinTheme& theme() const;
    /// 解析指定变量；不存在时返回后备值。
    std::string resolve(const std::string& key, const std::string& fallback = "") const;

private:
    SkinTheme theme_{"default", {}};
};

}  // namespace vp::ui