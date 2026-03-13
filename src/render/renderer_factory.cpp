#include "render/renderer_factory.h"

#include "render/d3d11_video_renderer.h"
#include "logger.h"
#include "render/opengl_video_renderer.h"
#include "render/sdl_video_renderer.h"

namespace vp::render {

/// 按平台返回默认渲染后端；当前 Windows 优先 D3D11。
VideoRendererType RendererFactory::detectBestRendererType() {
#if defined(_WIN32)
    return VideoRendererType::D3D11;
#else
    return VideoRendererType::SoftwareSDL;
#endif
}

/// 创建渲染器实例；`Auto` 会先解析为平台默认后端。
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
