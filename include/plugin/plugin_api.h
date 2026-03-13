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

/// 当前插件 ABI 版本号；宿主与插件必须一致。
inline constexpr uint32_t kPluginApiVersion = 1;

/// 插件元数据描述。
struct PluginDescriptor {
    std::string id;
    std::string name;
    std::string version;
    std::string description;
    std::string library_path;
    uint32_t api_version{kPluginApiVersion};
    bool enabled{true};
};

/// 插件宿主接口；插件通过它注册滤镜并输出日志。
class IPluginHost {
public:
    virtual ~IPluginHost() = default;

    /// 注册视频滤镜工厂。
    virtual bool registerVideoFilter(const std::string& name, filters::VideoFilterFactory factory) = 0;
    /// 注册音频滤镜工厂。
    virtual bool registerAudioFilter(const std::string& name, filters::AudioFilterFactory factory) = 0;

    /// 输出 Info 级插件日志。
    virtual void logInfo(const std::string& message) = 0;
    /// 输出 Warning 级插件日志。
    virtual void logWarning(const std::string& message) = 0;
    /// 输出 Error 级插件日志。
    virtual void logError(const std::string& message) = 0;
};

/// 导出 API 版本函数签名。
using PluginApiVersionFn = uint32_t (*)();
/// 导出元数据查询函数签名。
using PluginQueryDescriptorFn = bool (*)(PluginDescriptor*);
/// 导出初始化函数签名。
using PluginInitializeFn = bool (*)(IPluginHost*);
/// 导出卸载函数签名。
using PluginShutdownFn = void (*)(IPluginHost*);

/// 插件导出 API 版本符号名。
inline constexpr const char* kPluginApiVersionSymbol = "vp_plugin_api_version";
/// 插件导出描述查询符号名。
inline constexpr const char* kPluginQueryDescriptorSymbol = "vp_plugin_query_descriptor";
/// 插件导出初始化符号名。
inline constexpr const char* kPluginInitializeSymbol = "vp_plugin_initialize";
/// 插件导出卸载符号名。
inline constexpr const char* kPluginShutdownSymbol = "vp_plugin_shutdown";

}  // namespace vp::plugin