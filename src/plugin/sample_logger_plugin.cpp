#include "plugin/plugin_api.h"

#include <memory>
#include <string>
#include <vector>

#include "filters/video_filter.h"

namespace {

class SampleIdentityFilter final : public vp::filters::IVideoFilter {
public:
    std::string getName() const override {
        return "sample_identity";
    }

    vp::filters::FilterType getType() const override {
        return vp::filters::FilterType::Video;
    }

    void enable(bool enabled) override {
        enabled_ = enabled;
    }

    bool isEnabled() const override {
        return enabled_;
    }

    std::vector<vp::filters::FilterPin> getInputPins() const override {
        return {{"video_in", "rgba"}};
    }

    std::vector<vp::filters::FilterPin> getOutputPins() const override {
        return {{"video_out", "rgba"}};
    }

    void process(vp::core::VideoFrame&) override {
    }

    void setParameter(const std::string&, double) override {
    }

    double getParameter(const std::string&) const override {
        return 0.0;
    }

    std::vector<std::string> getParameterNames() const override {
        return {};
    }

private:
    bool enabled_{true};
};

}  // namespace

/// 导出插件 API 版本号，供宿主做 ABI 兼容性校验。
extern "C" VP_PLUGIN_EXPORT uint32_t vp_plugin_api_version() {
    return vp::plugin::kPluginApiVersion;
}

/// 导出插件元数据，供宿主在激活前读取插件信息。
extern "C" VP_PLUGIN_EXPORT bool vp_plugin_query_descriptor(vp::plugin::PluginDescriptor* descriptor) {
    if (!descriptor) {
        return false;
    }
    descriptor->id = "sample_logger_plugin";
    descriptor->name = "Sample Logger Plugin";
    descriptor->version = "0.1.0";
    descriptor->description = "Validates dynamic plugin loading, API compatibility, lifecycle, and filter registration.";
    descriptor->enabled = true;
    descriptor->api_version = vp::plugin::kPluginApiVersion;
    return true;
}

/// 插件初始化入口；注册示例滤镜并输出启动日志。
extern "C" VP_PLUGIN_EXPORT bool vp_plugin_initialize(vp::plugin::IPluginHost* host) {
    if (!host) {
        return false;
    }
    const bool registered = host->registerVideoFilter("sample_identity", [] {
        return std::make_unique<SampleIdentityFilter>();
    });
    if (!registered) {
        host->logError("sample_logger_plugin failed to register sample_identity filter");
        return false;
    }
    host->logInfo("sample_logger_plugin initialized");
    return true;
}

/// 插件卸载入口；用于输出收尾日志。
extern "C" VP_PLUGIN_EXPORT void vp_plugin_shutdown(vp::plugin::IPluginHost* host) {
    if (host) {
        host->logInfo("sample_logger_plugin shutdown");
    }
}
