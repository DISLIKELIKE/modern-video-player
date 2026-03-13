#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "plugin/plugin_api.h"

namespace vp::plugin {

/// 插件管理器；负责动态库加载、初始化、卸载与插件滤镜注册。
class PluginManager : public IPluginHost {
public:
    /// 析构时会卸载全部已加载插件。
    ~PluginManager();

    /// 注册一个静态插件描述；不会主动加载动态库。
    void registerPlugin(PluginDescriptor descriptor);
    /// 取消注册指定插件，并在需要时先执行卸载。
    bool unregisterPlugin(const std::string& id);
    /// 启用或禁用指定插件。
    bool setEnabled(const std::string& id, bool enabled);

    /// 从动态库路径加载单个插件。
    bool loadPlugin(const std::filesystem::path& path, std::string* error_message = nullptr);
    /// 扫描目录并加载其中可识别的插件动态库。
    size_t loadPluginsFromDirectory(const std::filesystem::path& directory, std::vector<std::string>* errors = nullptr);
    /// 卸载全部插件，并可收集卸载错误。
    void unloadAll(std::vector<std::string>* errors = nullptr);

    /// 返回当前插件描述列表快照。
    std::vector<PluginDescriptor> listPlugins() const;

    /// 插件回调：注册视频滤镜工厂。
    bool registerVideoFilter(const std::string& name, filters::VideoFilterFactory factory) override;
    /// 插件回调：注册音频滤镜工厂。
    bool registerAudioFilter(const std::string& name, filters::AudioFilterFactory factory) override;
    /// 插件回调：输出 Info 日志。
    void logInfo(const std::string& message) override;
    /// 插件回调：输出 Warning 日志。
    void logWarning(const std::string& message) override;
    /// 插件回调：输出 Error 日志。
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

    /// 执行插件初始化并记录其注册的滤镜。
    bool activatePlugin(PluginRecord& plugin, std::string* error_message = nullptr);
    /// 执行插件卸载并回收其注册的滤镜。
    void deactivatePlugin(PluginRecord& plugin);
    std::vector<PluginRecord>::iterator findPlugin(const std::string& id);
    std::vector<PluginRecord>::const_iterator findPlugin(const std::string& id) const;

    std::vector<PluginRecord> plugins_;
    PluginRecord* active_plugin_{nullptr};
};

}  // namespace vp::plugin