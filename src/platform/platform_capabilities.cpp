#include "platform/platform_capabilities.h"

#include <algorithm>

#include "platform/hw_device_factory.h"

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

std::vector<RendererSupport> detectRendererSupport(PlatformKind platform) {
    std::vector<RendererSupport> result;
    result.reserve(3);

#if defined(MVP_HAVE_D3D11_RENDERER) && MVP_HAVE_D3D11_RENDERER
    result.push_back({render::VideoRendererType::D3D11, true, true, 100});
#endif

#if defined(MVP_HAVE_OPENGL_RENDERER) && MVP_HAVE_OPENGL_RENDERER
    // Prefer OpenGL first on non-Windows platforms as Linux-first baseline.
    const int opengl_priority = platform == PlatformKind::Windows ? 20 : 100;
    result.push_back({render::VideoRendererType::OpenGL, true, true, opengl_priority});
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
