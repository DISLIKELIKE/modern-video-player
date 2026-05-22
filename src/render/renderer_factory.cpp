#include "render/renderer_factory.h"

#include <algorithm>

#if defined(MVP_HAVE_D3D11_RENDERER) && MVP_HAVE_D3D11_RENDERER
#include "render/d3d11_video_renderer.h"
#endif
#if defined(MVP_HAVE_OPENGL_RENDERER) && MVP_HAVE_OPENGL_RENDERER
#include "render/opengl_video_renderer.h"
#endif
#if defined(MVP_HAVE_SOFTWARE_SDL_RENDERER) && MVP_HAVE_SOFTWARE_SDL_RENDERER
#include "render/sdl_video_renderer.h"
#endif
#if defined(MVP_HAVE_VULKAN_RENDERER) && MVP_HAVE_VULKAN_RENDERER
#include "render/vulkan_video_renderer.h"
#endif

namespace vp::render {

bool RendererFactory::isSupported(VideoRendererType type, const platform::PlatformCapabilities& capabilities) {
    return std::any_of(capabilities.renderer_support.begin(),
                       capabilities.renderer_support.end(),
                       [type](const platform::RendererSupport& support) {
                           return support.type == type && support.compiled_in && support.runtime_available;
                       });
}

const char* RendererFactory::rendererName(VideoRendererType type) {
    switch (type) {
    case VideoRendererType::Auto:
        return "Auto";
    case VideoRendererType::SoftwareSDL:
        return "SoftwareSDL";
    case VideoRendererType::D3D11:
        return "D3D11";
    case VideoRendererType::OpenGL:
        return "OpenGL";
    case VideoRendererType::Vulkan:
        return "Vulkan";
    default:
        return "Unknown";
    }
}

VideoRendererPtr RendererFactory::create(VideoRendererType type) {
    VideoRendererType final_type = type;
    if (final_type == VideoRendererType::Auto) {
        final_type = VideoRendererType::SoftwareSDL;
    }

    switch (final_type) {
    case VideoRendererType::SoftwareSDL:
#if defined(MVP_HAVE_SOFTWARE_SDL_RENDERER) && MVP_HAVE_SOFTWARE_SDL_RENDERER
        return std::make_unique<SdlVideoRenderer>();
#else
        return nullptr;
#endif
    case VideoRendererType::D3D11:
#if defined(MVP_HAVE_D3D11_RENDERER) && MVP_HAVE_D3D11_RENDERER
        return std::make_unique<D3D11VideoRenderer>();
#else
        return nullptr;
#endif
    case VideoRendererType::OpenGL:
#if defined(MVP_HAVE_OPENGL_RENDERER) && MVP_HAVE_OPENGL_RENDERER
        return std::make_unique<OpenGLVideoRenderer>();
#else
        return nullptr;
#endif
    case VideoRendererType::Vulkan:
#if defined(MVP_HAVE_VULKAN_RENDERER) && MVP_HAVE_VULKAN_RENDERER
        return std::make_unique<VulkanVideoRenderer>();
#else
        return nullptr;
#endif
    case VideoRendererType::Auto:
    default:
        return nullptr;
    }
}

}  // namespace vp::render
