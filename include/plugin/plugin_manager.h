#pragma once

#include <string>
#include <vector>

namespace vp::plugin {

struct PluginDescriptor {
    std::string id;
    std::string name;
    std::string version;
    bool enabled{true};
};

class PluginManager {
public:
    void registerPlugin(PluginDescriptor descriptor);
    bool unregisterPlugin(const std::string& id);
    bool setEnabled(const std::string& id, bool enabled);

    std::vector<PluginDescriptor> listPlugins() const;

private:
    std::vector<PluginDescriptor> plugins_;
};

}  // namespace vp::plugin

