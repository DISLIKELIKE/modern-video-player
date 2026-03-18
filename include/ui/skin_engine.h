#pragma once

#include <string>
#include <unordered_map>

namespace vp::ui {

struct SkinTheme {
    std::string name;
    std::unordered_map<std::string, std::string> variables;
};

class SkinEngine {
public:
    void setTheme(SkinTheme theme);
    const SkinTheme& theme() const;
    std::string resolve(const std::string& key, const std::string& fallback = "") const;

private:
    SkinTheme theme_{"default", {}};
};

}  // namespace vp::ui
