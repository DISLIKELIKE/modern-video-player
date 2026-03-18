#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "plugin/plugin_api.h"

namespace vp::plugin {

class PluginManager : public IPluginHost {
public:
    ~PluginManager() override;

    void registerPlugin(PluginDescriptor descriptor);
    bool unregisterPlugin(const std::string& id);
    bool setEnabled(const std::string& id, bool enabled);

    bool loadPlugin(const std::filesystem::path& path, std::string* error_message = nullptr);
    size_t loadPluginsFromDirectory(const std::filesystem::path& directory, std::vector<std::string>* errors = nullptr);
    void unloadAll(std::vector<std::string>* errors = nullptr);

    std::vector<PluginDescriptor> listPlugins() const;

    bool registerVideoFilter(const std::string& name, filters::VideoFilterFactory factory) override;
    bool registerAudioFilter(const std::string& name, filters::AudioFilterFactory factory) override;
    void logInfo(const std::string& message) override;
    void logWarning(const std::string& message) override;
    void logError(const std::string& message) override;

private:
    struct PluginRecord {
        PluginDescriptor descriptor;
        std::filesystem::path library_path;
        void* library_handle{nullptr};
        PluginInitializeFn initialize{nullptr};
        PluginShutdownFn shutdown{nullptr};
        bool dynamic{false};
        bool active{false};
        std::vector<std::string> registered_video_filters;
        std::vector<std::string> registered_audio_filters;
    };

    bool activatePlugin(PluginRecord& plugin, std::string* error_message = nullptr);
    void deactivatePlugin(PluginRecord& plugin);
    std::vector<PluginRecord>::iterator findPlugin(const std::string& id);
    std::vector<PluginRecord>::const_iterator findPlugin(const std::string& id) const;

    std::vector<PluginRecord> plugins_;
    PluginRecord* active_plugin_{nullptr};
};

}  // namespace vp::plugin
