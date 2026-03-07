#pragma once

#include <optional>
#include <string>
#include <unordered_map>

namespace vp::config {

class SettingsManager {
public:
    bool loadIni(const std::string& file_path);
    bool saveIni(const std::string& file_path) const;
    bool reload();

    void setString(const std::string& key, const std::string& value);
    void setInt(const std::string& key, int value);
    void setDouble(const std::string& key, double value);
    void setBool(const std::string& key, bool value);

    std::optional<std::string> getString(const std::string& key) const;
    std::optional<int> getInt(const std::string& key) const;
    std::optional<double> getDouble(const std::string& key) const;
    std::optional<bool> getBool(const std::string& key) const;

private:
    std::string loaded_path_;
    std::unordered_map<std::string, std::string> values_;
};

}  // namespace vp::config
