#pragma once

#include <cstdint>
#include <string>

#include "filters/filter_registry.h"

#if defined(_WIN32)
#  if defined(VP_PLUGIN_BUILD)
#    define VP_PLUGIN_EXPORT __declspec(dllexport)
#  else
#    define VP_PLUGIN_EXPORT __declspec(dllimport)
#  endif
#else
#  define VP_PLUGIN_EXPORT __attribute__((visibility("default")))
#endif

namespace vp::plugin {

inline constexpr uint32_t kPluginApiVersion = 1;

struct PluginDescriptor {
    std::string id;
    std::string name;
    std::string version;
    std::string description;
    std::string library_path;
    uint32_t api_version{kPluginApiVersion};
    bool enabled{true};
};

class IPluginHost {
public:
    virtual ~IPluginHost() = default;

    virtual bool registerVideoFilter(const std::string& name, filters::VideoFilterFactory factory) = 0;
    virtual bool registerAudioFilter(const std::string& name, filters::AudioFilterFactory factory) = 0;

    virtual void logInfo(const std::string& message) = 0;
    virtual void logWarning(const std::string& message) = 0;
    virtual void logError(const std::string& message) = 0;
};

using PluginApiVersionFn = uint32_t (*)();
using PluginQueryDescriptorFn = bool (*)(PluginDescriptor*);
using PluginInitializeFn = bool (*)(IPluginHost*);
using PluginShutdownFn = void (*)(IPluginHost*);

inline constexpr const char* kPluginApiVersionSymbol = "vp_plugin_api_version";
inline constexpr const char* kPluginQueryDescriptorSymbol = "vp_plugin_query_descriptor";
inline constexpr const char* kPluginInitializeSymbol = "vp_plugin_initialize";
inline constexpr const char* kPluginShutdownSymbol = "vp_plugin_shutdown";

}  // namespace vp::plugin
