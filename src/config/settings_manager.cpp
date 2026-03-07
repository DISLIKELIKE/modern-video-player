#include "config/settings_manager.h"

#include <cctype>
#include <fstream>

namespace vp::config {

namespace {

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

bool SettingsManager::reload() {
    if (loaded_path_.empty()) {
        return false;
    }
    return loadIni(loaded_path_);
}

void SettingsManager::setString(const std::string& key, const std::string& value) {
    values_[key] = value;
}

void SettingsManager::setInt(const std::string& key, int value) {
    values_[key] = std::to_string(value);
}

void SettingsManager::setBool(const std::string& key, bool value) {
    values_[key] = value ? "true" : "false";
}

std::optional<std::string> SettingsManager::getString(const std::string& key) const {
    auto it = values_.find(key);
    if (it == values_.end()) {
        return std::nullopt;
    }
    return it->second;
}

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

