#include "plugin/plugin_manager.h"

#include <algorithm>
#include <exception>
#include <system_error>

#include "filters/filter_registry.h"
#include "logger.h"

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace vp::plugin {

namespace {

std::string dynamicLibraryErrorString() {
#if defined(_WIN32)
    const DWORD error = GetLastError();
    if (error == 0) {
        return "unknown Windows loader error";
    }

    LPSTR buffer = nullptr;
    const DWORD size = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        error,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<LPSTR>(&buffer),
        0,
        nullptr);
    std::string message = (size > 0 && buffer) ? std::string(buffer, size) : std::string("unknown Windows loader error");
    if (buffer) {
        LocalFree(buffer);
    }
    while (!message.empty() && (message.back() == '\r' || message.back() == '\n')) {
        message.pop_back();
    }
    return message;
#else
    const char* error = dlerror();
    return error ? std::string(error) : std::string("unknown dlopen error");
#endif
}

void* openDynamicLibrary(const std::filesystem::path& path, std::string* error_message) {
#if defined(_WIN32)
    HMODULE handle = LoadLibraryW(path.wstring().c_str());
    if (!handle && error_message) {
        *error_message = dynamicLibraryErrorString();
    }
    return reinterpret_cast<void*>(handle);
#else
    void* handle = dlopen(path.string().c_str(), RTLD_NOW);
    if (!handle && error_message) {
        *error_message = dynamicLibraryErrorString();
    }
    return handle;
#endif
}

void closeDynamicLibrary(void* handle) {
    if (!handle) {
        return;
    }
#if defined(_WIN32)
    FreeLibrary(reinterpret_cast<HMODULE>(handle));
#else
    dlclose(handle);
#endif
}

void* resolveDynamicSymbol(void* handle, const char* symbol_name) {
    if (!handle || !symbol_name) {
        return nullptr;
    }
#if defined(_WIN32)
    return reinterpret_cast<void*>(GetProcAddress(reinterpret_cast<HMODULE>(handle), symbol_name));
#else
    return dlsym(handle, symbol_name);
#endif
}

std::string pluginExtension() {
#if defined(_WIN32)
    return ".dll";
#elif defined(__APPLE__)
    return ".dylib";
#else
    return ".so";
#endif
}

std::string describePath(const std::filesystem::path& path) {
    return path.generic_string();
}

}  // namespace

PluginManager::~PluginManager() {
    unloadAll();
}

void PluginManager::registerPlugin(PluginDescriptor descriptor) {
    auto it = findPlugin(descriptor.id);
    if (it != plugins_.end()) {
        it->descriptor = std::move(descriptor);
        return;
    }

    PluginRecord record;
    record.descriptor = std::move(descriptor);
    record.active = record.descriptor.enabled;
    plugins_.push_back(std::move(record));
}

bool PluginManager::unregisterPlugin(const std::string& id) {
    auto it = findPlugin(id);
    if (it == plugins_.end()) {
        return false;
    }

    deactivatePlugin(*it);
    if (it->library_handle) {
        closeDynamicLibrary(it->library_handle);
        it->library_handle = nullptr;
    }
    plugins_.erase(it);
    return true;
}

bool PluginManager::setEnabled(const std::string& id, bool enabled) {
    auto it = findPlugin(id);
    if (it == plugins_.end()) {
        return false;
    }
    if (it->descriptor.enabled == enabled) {
        return true;
    }

    if (enabled) {
        std::string error_message;
        if (!activatePlugin(*it, &error_message)) {
            logError("Plugin activation failed for " + id + ": " + error_message);
            return false;
        }
        it->descriptor.enabled = true;
        return true;
    }

    deactivatePlugin(*it);
    it->descriptor.enabled = false;
    return true;
}

/// 从动态库加载插件、校验导出符号，并执行初始化。
bool PluginManager::loadPlugin(const std::filesystem::path& path, std::string* error_message) {
    std::error_code ec;
    const std::filesystem::path normalized_path = std::filesystem::weakly_canonical(path, ec);
    const std::filesystem::path plugin_path = ec ? path : normalized_path;

    if (!std::filesystem::exists(plugin_path)) {
        if (error_message) {
            *error_message = "plugin path does not exist: " + describePath(plugin_path);
        }
        return false;
    }
    if (!std::filesystem::is_regular_file(plugin_path)) {
        if (error_message) {
            *error_message = "plugin path is not a file: " + describePath(plugin_path);
        }
        return false;
    }

    PluginRecord record;
    record.dynamic = true;
    record.library_path = plugin_path;

    std::string loader_error;
    record.library_handle = openDynamicLibrary(plugin_path, &loader_error);
    if (!record.library_handle) {
        if (error_message) {
            *error_message = "failed to load plugin " + describePath(plugin_path) + ": " + loader_error;
        }
        return false;
    }

    record.initialize = reinterpret_cast<PluginInitializeFn>(resolveDynamicSymbol(record.library_handle, kPluginInitializeSymbol));
    record.shutdown = reinterpret_cast<PluginShutdownFn>(resolveDynamicSymbol(record.library_handle, kPluginShutdownSymbol));
    auto query_descriptor = reinterpret_cast<PluginQueryDescriptorFn>(resolveDynamicSymbol(record.library_handle, kPluginQueryDescriptorSymbol));
    auto query_api_version = reinterpret_cast<PluginApiVersionFn>(resolveDynamicSymbol(record.library_handle, kPluginApiVersionSymbol));

    if (!record.initialize || !record.shutdown || !query_descriptor || !query_api_version) {
        if (error_message) {
            *error_message = "plugin is missing required exported symbols: " + describePath(plugin_path);
        }
        closeDynamicLibrary(record.library_handle);
        return false;
    }

    const uint32_t api_version = query_api_version();
    if (api_version != kPluginApiVersion) {
        if (error_message) {
            *error_message = "plugin API version mismatch: expected " + std::to_string(kPluginApiVersion) +
                             ", got " + std::to_string(api_version) + " (" + describePath(plugin_path) + ")";
        }
        closeDynamicLibrary(record.library_handle);
        return false;
    }

    if (!query_descriptor(&record.descriptor)) {
        if (error_message) {
            *error_message = "plugin descriptor query failed: " + describePath(plugin_path);
        }
        closeDynamicLibrary(record.library_handle);
        return false;
    }

    if (record.descriptor.id.empty()) {
        if (error_message) {
            *error_message = "plugin descriptor id is empty: " + describePath(plugin_path);
        }
        closeDynamicLibrary(record.library_handle);
        return false;
    }
    if (record.descriptor.name.empty()) {
        record.descriptor.name = record.descriptor.id;
    }
    record.descriptor.api_version = api_version;
    record.descriptor.library_path = describePath(plugin_path);

    const auto duplicate = findPlugin(record.descriptor.id);
    if (duplicate != plugins_.end()) {
        if (error_message) {
            *error_message = "plugin already loaded: " + record.descriptor.id;
        }
        closeDynamicLibrary(record.library_handle);
        return false;
    }

    std::string activation_error;
    if (!activatePlugin(record, &activation_error)) {
        if (error_message) {
            *error_message = activation_error;
        }
        deactivatePlugin(record);
        closeDynamicLibrary(record.library_handle);
        return false;
    }

    record.descriptor.enabled = true;
    plugins_.push_back(std::move(record));
    return true;
}

/// 扫描目录中的插件动态库并按文件名顺序加载。
size_t PluginManager::loadPluginsFromDirectory(const std::filesystem::path& directory,
                                               std::vector<std::string>* errors) {
    std::error_code ec;
    if (!std::filesystem::exists(directory, ec)) {
        if (errors) {
            errors->push_back("plugin directory does not exist: " + describePath(directory));
        }
        return 0;
    }
    if (!std::filesystem::is_directory(directory, ec)) {
        if (errors) {
            errors->push_back("plugin path is not a directory: " + describePath(directory));
        }
        return 0;
    }

    std::vector<std::filesystem::path> candidates;
    for (const auto& entry : std::filesystem::directory_iterator(directory, ec)) {
        if (ec) {
            if (errors) {
                errors->push_back("failed to enumerate plugin directory: " + describePath(directory));
            }
            return 0;
        }
        if (!entry.is_regular_file()) {
            continue;
        }
        if (entry.path().extension() == pluginExtension()) {
            candidates.push_back(entry.path());
        }
    }

    std::sort(candidates.begin(), candidates.end());

    size_t loaded_count = 0;
    for (const auto& candidate : candidates) {
        std::string error_message;
        if (loadPlugin(candidate, &error_message)) {
            ++loaded_count;
        } else if (errors) {
            errors->push_back(error_message);
        }
    }
    return loaded_count;
}

/// 逆序卸载全部插件，确保先加载的插件最后释放。
void PluginManager::unloadAll(std::vector<std::string>* errors) {
    for (auto it = plugins_.rbegin(); it != plugins_.rend(); ++it) {
        try {
            deactivatePlugin(*it);
        } catch (const std::exception& ex) {
            if (errors) {
                errors->push_back("plugin shutdown exception for " + it->descriptor.id + ": " + ex.what());
            }
        } catch (...) {
            if (errors) {
                errors->push_back("plugin shutdown exception for " + it->descriptor.id + ": unknown error");
            }
        }

        if (it->library_handle) {
            closeDynamicLibrary(it->library_handle);
            it->library_handle = nullptr;
        }
    }
    plugins_.clear();
}

std::vector<PluginDescriptor> PluginManager::listPlugins() const {
    std::vector<PluginDescriptor> descriptors;
    descriptors.reserve(plugins_.size());
    for (const auto& plugin : plugins_) {
        descriptors.push_back(plugin.descriptor);
    }
    return descriptors;
}

bool PluginManager::registerVideoFilter(const std::string& name, filters::VideoFilterFactory factory) {
    if (!active_plugin_ || name.empty() || !factory) {
        return false;
    }
    filters::FilterRegistry::instance().registerVideoFilter(name, std::move(factory));
    active_plugin_->registered_video_filters.push_back(name);
    return true;
}

bool PluginManager::registerAudioFilter(const std::string& name, filters::AudioFilterFactory factory) {
    if (!active_plugin_ || name.empty() || !factory) {
        return false;
    }
    filters::FilterRegistry::instance().registerAudioFilter(name, std::move(factory));
    active_plugin_->registered_audio_filters.push_back(name);
    return true;
}

void PluginManager::logInfo(const std::string& message) {
    Logger::info("[plugin] " + message);
}

void PluginManager::logWarning(const std::string& message) {
    Logger::warning("[plugin] " + message);
}

void PluginManager::logError(const std::string& message) {
    Logger::error("[plugin] " + message);
}

/// 执行插件初始化，并跟踪其运行期注册的滤镜资源。
bool PluginManager::activatePlugin(PluginRecord& plugin, std::string* error_message) {
    if (plugin.active) {
        return true;
    }
    if (!plugin.initialize) {
        plugin.active = true;
        plugin.descriptor.enabled = true;
        return true;
    }

    plugin.registered_video_filters.clear();
    plugin.registered_audio_filters.clear();
    active_plugin_ = &plugin;
    try {
        if (!plugin.initialize(this)) {
            active_plugin_ = nullptr;
            for (const auto& name : plugin.registered_video_filters) {
                filters::FilterRegistry::instance().unregisterVideoFilter(name);
            }
            for (const auto& name : plugin.registered_audio_filters) {
                filters::FilterRegistry::instance().unregisterAudioFilter(name);
            }
            plugin.registered_video_filters.clear();
            plugin.registered_audio_filters.clear();
            if (error_message) {
                *error_message = "plugin initialize returned false: " + plugin.descriptor.id;
            }
            return false;
        }
    } catch (const std::exception& ex) {
        active_plugin_ = nullptr;
        for (const auto& name : plugin.registered_video_filters) {
            filters::FilterRegistry::instance().unregisterVideoFilter(name);
        }
        for (const auto& name : plugin.registered_audio_filters) {
            filters::FilterRegistry::instance().unregisterAudioFilter(name);
        }
        plugin.registered_video_filters.clear();
        plugin.registered_audio_filters.clear();
        if (error_message) {
            *error_message = "plugin initialize exception for " + plugin.descriptor.id + ": " + ex.what();
        }
        return false;
    } catch (...) {
        active_plugin_ = nullptr;
        for (const auto& name : plugin.registered_video_filters) {
            filters::FilterRegistry::instance().unregisterVideoFilter(name);
        }
        for (const auto& name : plugin.registered_audio_filters) {
            filters::FilterRegistry::instance().unregisterAudioFilter(name);
        }
        plugin.registered_video_filters.clear();
        plugin.registered_audio_filters.clear();
        if (error_message) {
            *error_message = "plugin initialize exception for " + plugin.descriptor.id + ": unknown error";
        }
        return false;
    }
    active_plugin_ = nullptr;
    plugin.active = true;
    plugin.descriptor.enabled = true;
    return true;
}

/// 执行插件卸载，并回收该插件注册的全部滤镜。
void PluginManager::deactivatePlugin(PluginRecord& plugin) {
    if (!plugin.active) {
        return;
    }

    if (plugin.shutdown) {
        active_plugin_ = &plugin;
        try {
            plugin.shutdown(this);
        } catch (const std::exception& ex) {
            Logger::warning(std::string("[plugin] shutdown exception for ") + plugin.descriptor.id + ": " + ex.what());
        } catch (...) {
            Logger::warning(std::string("[plugin] shutdown exception for ") + plugin.descriptor.id + ": unknown error");
        }
        active_plugin_ = nullptr;
    }

    for (const auto& name : plugin.registered_video_filters) {
        filters::FilterRegistry::instance().unregisterVideoFilter(name);
    }
    for (const auto& name : plugin.registered_audio_filters) {
        filters::FilterRegistry::instance().unregisterAudioFilter(name);
    }
    plugin.registered_video_filters.clear();
    plugin.registered_audio_filters.clear();
    plugin.active = false;
    plugin.descriptor.enabled = false;
}

std::vector<PluginManager::PluginRecord>::iterator PluginManager::findPlugin(const std::string& id) {
    return std::find_if(plugins_.begin(), plugins_.end(), [&id](const PluginRecord& plugin) {
        return plugin.descriptor.id == id;
    });
}

std::vector<PluginManager::PluginRecord>::const_iterator PluginManager::findPlugin(const std::string& id) const {
    return std::find_if(plugins_.begin(), plugins_.end(), [&id](const PluginRecord& plugin) {
        return plugin.descriptor.id == id;
    });
}

}  // namespace vp::plugin
