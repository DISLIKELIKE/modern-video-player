#include "plugin/plugin_manager.h"

#include <algorithm>

namespace vp::plugin {

void PluginManager::registerPlugin(PluginDescriptor descriptor) {
    auto it = std::find_if(plugins_.begin(), plugins_.end(), [&descriptor](const PluginDescriptor& plugin) {
        return plugin.id == descriptor.id;
    });
    if (it != plugins_.end()) {
        *it = std::move(descriptor);
        return;
    }
    plugins_.push_back(std::move(descriptor));
}

bool PluginManager::unregisterPlugin(const std::string& id) {
    auto it = std::remove_if(plugins_.begin(), plugins_.end(), [&id](const PluginDescriptor& plugin) {
        return plugin.id == id;
    });
    if (it == plugins_.end()) {
        return false;
    }
    plugins_.erase(it, plugins_.end());
    return true;
}

bool PluginManager::setEnabled(const std::string& id, bool enabled) {
    auto it = std::find_if(plugins_.begin(), plugins_.end(), [&id](const PluginDescriptor& plugin) {
        return plugin.id == id;
    });
    if (it == plugins_.end()) {
        return false;
    }
    it->enabled = enabled;
    return true;
}

std::vector<PluginDescriptor> PluginManager::listPlugins() const {
    return plugins_;
}

}  // namespace vp::plugin

