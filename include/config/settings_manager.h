#pragma once

#include <optional>
#include <string>
#include <unordered_map>

namespace vp::config {

/// 简单 INI 风格配置管理器；内部统一以字符串存储键值。
class SettingsManager {
public:
    /// 从磁盘加载 INI 文件；成功后会记录最后一次加载路径。
    bool loadIni(const std::string& file_path);
    /// 将当前键值对保存为 INI 文件。
    bool saveIni(const std::string& file_path) const;
    /// 重新加载最近一次成功读取的配置文件。
    bool reload();

    /// 设置字符串配置项。
    void setString(const std::string& key, const std::string& value);
    /// 设置整型配置项。
    void setInt(const std::string& key, int value);
    /// 设置浮点配置项。
    void setDouble(const std::string& key, double value);
    /// 设置布尔配置项。
    void setBool(const std::string& key, bool value);

    /// 获取字符串配置项；不存在时返回空。
    std::optional<std::string> getString(const std::string& key) const;
    /// 获取整型配置项；转换失败时返回空。
    std::optional<int> getInt(const std::string& key) const;
    /// 获取浮点配置项；转换失败时返回空。
    std::optional<double> getDouble(const std::string& key) const;
    /// 获取布尔配置项；仅识别常见 true/false/1/0 表达。
    std::optional<bool> getBool(const std::string& key) const;

private:
    std::string loaded_path_;
    std::unordered_map<std::string, std::string> values_;
};

}  // namespace vp::config