#include "render/renderer_factory.h"

#include "render/d3d11_video_renderer.h"
#include "logger.h"
#include "render/opengl_video_renderer.h"
#include "render/sdl_video_renderer.h"

namespace vp::render {

VideoRendererType RendererFactory::detectBestRendererType() {
#if defined(_WIN32)
    return VideoRendererType::D3D11;
#else
    return VideoRendererType::SoftwareSDL;
#endif
}

VideoRendererPtr RendererFactory::create(VideoRendererType type) {
    VideoRendererType final_type = type;
    if (final_type == VideoRendererType::Auto) {
        final_type = detectBestRendererType();
    }

    switch (final_type) {
        case VideoRendererType::SoftwareSDL:
            return std::make_unique<SdlVideoRenderer>();
        case VideoRendererType::D3D11:
            LOG_WARNING("D3D11 renderer is experimental, fallback to SDL if init fails");
            return std::make_unique<D3D11VideoRenderer>();
        case VideoRendererType::OpenGL:
            LOG_WARNING("OpenGL renderer is experimental, fallback to SDL if init fails");
            return std::make_unique<OpenGLVideoRenderer>();
        case VideoRendererType::Auto:
        default:
            return std::make_unique<SdlVideoRenderer>();
    }
}

}  // namespace vp::render
