#include "platform/platform_capabilities.h"

#include <algorithm>

#include "platform/hw_device_factory.h"

#if defined(MVP_HAVE_VULKAN_RENDERER) && MVP_HAVE_VULKAN_RENDERER
#include <vulkan/vulkan.h>
#endif

namespace vp::platform {

namespace {

PlatformKind detectPlatformKind() {
#if defined(_WIN32)
    return PlatformKind::Windows;
#elif defined(__linux__)
    return PlatformKind::Linux;
#elif defined(__APPLE__)
    return PlatformKind::MacOS;
#else
    return PlatformKind::Unknown;
#endif
}

#if defined(MVP_HAVE_VULKAN_RENDERER) && MVP_HAVE_VULKAN_RENDERER
bool probeVulkanRuntimeAvailability(PlatformKind platform) {
    if (platform != PlatformKind::Linux && platform != PlatformKind::Windows) {
        return false;
    }

    uint32_t extension_count = 0;
    const VkResult extension_result =
        vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
    if (extension_result != VK_SUCCESS) {
        return false;
    }

    VkApplicationInfo app_info{};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "modern-video-player-capability-probe";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "modern-video-player";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
#if defined(VK_API_VERSION_1_0)
    app_info.apiVersion = VK_API_VERSION_1_0;
#else
    app_info.apiVersion = VK_MAKE_VERSION(1, 0, 0);
#endif

    VkInstanceCreateInfo instance_info{};
    instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_info.pApplicationInfo = &app_info;

    VkInstance instance = VK_NULL_HANDLE;
    if (vkCreateInstance(&instance_info, nullptr, &instance) != VK_SUCCESS || instance == VK_NULL_HANDLE) {
        return false;
    }

    uint32_t physical_device_count = 0;
    const VkResult physical_device_result =
        vkEnumeratePhysicalDevices(instance, &physical_device_count, nullptr);
    vkDestroyInstance(instance, nullptr);
    return physical_device_result == VK_SUCCESS && physical_device_count > 0;
}
#endif

std::vector<RendererSupport> detectRendererSupport(PlatformKind platform) {
    std::vector<RendererSupport> result;
    result.reserve(4);

#if defined(MVP_HAVE_D3D11_RENDERER) && MVP_HAVE_D3D11_RENDERER
    result.push_back({render::VideoRendererType::D3D11, true, true, 100});
#endif

#if defined(MVP_HAVE_OPENGL_RENDERER) && MVP_HAVE_OPENGL_RENDERER
    // Prefer OpenGL first on non-Windows platforms as Linux-first baseline.
    const int opengl_priority = platform == PlatformKind::Windows ? 20 : 100;
    result.push_back({render::VideoRendererType::OpenGL, true, true, opengl_priority});
#endif

#if defined(MVP_HAVE_VULKAN_RENDERER) && MVP_HAVE_VULKAN_RENDERER
    // Linux-first Vulkan rollout: keep Vulkan above OpenGL in default renderer order.
    const int vulkan_priority = platform == PlatformKind::Linux ? 120 : 30;
    const bool vulkan_runtime_available = probeVulkanRuntimeAvailability(platform);
    result.push_back({render::VideoRendererType::Vulkan, true, vulkan_runtime_available, vulkan_priority});
#endif

#if defined(MVP_HAVE_SOFTWARE_SDL_RENDERER) && MVP_HAVE_SOFTWARE_SDL_RENDERER
    const int software_priority = platform == PlatformKind::Windows ? 80 : 60;
    result.push_back({render::VideoRendererType::SoftwareSDL, true, true, software_priority});
#endif

    std::sort(result.begin(),
              result.end(),
              [](const RendererSupport& lhs, const RendererSupport& rhs) {
                  return lhs.default_priority > rhs.default_priority;
              });
    return result;
}

std::vector<DecoderBackendSupport> detectDecoderSupport() {
    std::vector<DecoderBackendSupport> result;
    result.reserve(5);

#if defined(MVP_HAVE_D3D11VA_DECODER) && MVP_HAVE_D3D11VA_DECODER
    result.push_back({decoder::DecoderBackend::D3D11VA, true, true, true, 100});
#endif
#if defined(MVP_HAVE_DXVA2_DECODER) && MVP_HAVE_DXVA2_DECODER
    // Switch is available, but runtime path is not implemented in current baseline.
    result.push_back({decoder::DecoderBackend::DXVA2, true, false, true, 90});
#endif
#if defined(MVP_HAVE_VAAPI_DECODER) && MVP_HAVE_VAAPI_DECODER
    bool vaapi_runtime_available = false;
#if defined(__linux__)
    vaapi_runtime_available = HwDeviceFactory::probeRuntimeAvailability(decoder::DecoderBackend::VAAPI, nullptr);
#endif
    result.push_back({decoder::DecoderBackend::VAAPI, true, vaapi_runtime_available, true, 90});
#endif
#if defined(MVP_HAVE_VIDEOTOOLBOX_DECODER) && MVP_HAVE_VIDEOTOOLBOX_DECODER
    // Switch is available, but runtime path is not implemented in current baseline.
    result.push_back({decoder::DecoderBackend::VideoToolbox, true, false, true, 90});
#endif

    // Software decode is always the mandatory fallback.
    result.push_back({decoder::DecoderBackend::Software, true, true, false, 10});

    std::sort(result.begin(),
              result.end(),
              [](const DecoderBackendSupport& lhs, const DecoderBackendSupport& rhs) {
                  return lhs.default_priority > rhs.default_priority;
              });
    return result;
}

}  // namespace

PlatformCapabilities PlatformCapabilitiesProbe::detect() {
    PlatformCapabilities capabilities;
    capabilities.platform = detectPlatformKind();
    capabilities.renderer_support = detectRendererSupport(capabilities.platform);
    capabilities.decoder_support = detectDecoderSupport();
#if defined(MVP_HAVE_SOFTWARE_SDL_RENDERER) && MVP_HAVE_SOFTWARE_SDL_RENDERER
    capabilities.has_sdl_windowing = true;
    capabilities.has_sdl_audio = true;
#else
    capabilities.has_sdl_windowing = false;
    capabilities.has_sdl_audio = false;
#endif
    return capabilities;
}

const char* platformKindName(PlatformKind kind) {
    switch (kind) {
    case PlatformKind::Windows:
        return "Windows";
    case PlatformKind::Linux:
        return "Linux";
    case PlatformKind::MacOS:
        return "MacOS";
    case PlatformKind::Unknown:
    default:
        return "Unknown";
    }
}

}  // namespace vp::platform
