#include "config/settings_manager.h"

#include <cctype>
#include <fstream>

namespace vp::config {

namespace {

/// 去掉配置键值两端空白，避免 INI 行中的多余空格影响解析。
std::string trimCopy(const std::string& text) {
    size_t start = 0;
    size_t end = text.size();
    while (start < end && std::isspace(static_cast<unsigned char>(text[start])) != 0) {
        ++start;
    }
    while (end > start && std::isspace(static_cast<unsigned char>(text[end - 1])) != 0) {
        --end;
    }
    return text.substr(start, end - start);
}

}  // namespace

/// 从磁盘读取简单键值 INI，并覆盖当前内存配置。
bool SettingsManager::loadIni(const std::string& file_path) {
    std::ifstream input(file_path);
    if (!input.good()) {
        return false;
    }

    values_.clear();
    loaded_path_ = file_path;

    std::string line;
    while (std::getline(input, line)) {
        std::string trimmed = trimCopy(line);
        if (trimmed.empty() || trimmed[0] == '#' || trimmed[0] == ';') {
            continue;
        }
        const size_t delimiter = trimmed.find('=');
        if (delimiter == std::string::npos) {
            continue;
        }
        std::string key = trimCopy(trimmed.substr(0, delimiter));
        std::string value = trimCopy(trimmed.substr(delimiter + 1));
        if (!key.empty()) {
            values_[key] = value;
        }
    }

    return true;
}

/// 将内存中的键值对按 `key=value` 形式写回磁盘。
bool SettingsManager::saveIni(const std::string& file_path) const {
    std::ofstream output(file_path, std::ios::trunc);
    if (!output.good()) {
        return false;
    }

    for (const auto& [key, value] : values_) {
        output << key << "=" << value << "\n";
    }
    return true;
}

/// 使用最近一次成功加载路径重新读取配置。
bool SettingsManager::reload() {
    if (loaded_path_.empty()) {
        return false;
    }
    return loadIni(loaded_path_);
}

/// 设置字符串配置项；若键已存在则直接覆盖旧值。
void SettingsManager::setString(const std::string& key, const std::string& value) {
    values_[key] = value;
}

/// 设置整数配置项，内部统一序列化为文本。
void SettingsManager::setInt(const std::string& key, int value) {
    values_[key] = std::to_string(value);
}

/// 设置浮点配置项，内部统一序列化为文本。
void SettingsManager::setDouble(const std::string& key, double value) {
    values_[key] = std::to_string(value);
}

/// 设置布尔配置项，内部以 `true/false` 文本保存。
void SettingsManager::setBool(const std::string& key, bool value) {
    values_[key] = value ? "true" : "false";
}

/// 获取原始字符串配置；键不存在时返回空。
std::optional<std::string> SettingsManager::getString(const std::string& key) const {
    auto it = values_.find(key);
    if (it == values_.end()) {
        return std::nullopt;
    }
    return it->second;
}

/// 获取整数配置；解析失败或键不存在时返回空。
std::optional<int> SettingsManager::getInt(const std::string& key) const {
    auto value = getString(key);
    if (!value) {
        return std::nullopt;
    }
    try {
        return std::stoi(*value);
    } catch (...) {
        return std::nullopt;
    }
}

/// 获取浮点配置；解析失败或键不存在时返回空。
std::optional<double> SettingsManager::getDouble(const std::string& key) const {
    auto value = getString(key);
    if (!value) {
        return std::nullopt;
    }
    try {
        return std::stod(*value);
    } catch (...) {
        return std::nullopt;
    }
}

/// 获取布尔配置；支持 `1/0/true/false` 文本形式。
std::optional<bool> SettingsManager::getBool(const std::string& key) const {
    auto value = getString(key);
    if (!value) {
        return std::nullopt;
    }
    if (*value == "1" || *value == "true" || *value == "True") {
        return true;
    }
    if (*value == "0" || *value == "false" || *value == "False") {
        return false;
    }
    return std::nullopt;
}

}  // namespace vp::config
