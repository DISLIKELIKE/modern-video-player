#pragma once

#include "platform/platform_capabilities.h"
#include "render/video_renderer.h"

namespace vp::render {

class RendererFactory {
public:
    static bool isSupported(VideoRendererType type, const platform::PlatformCapabilities& capabilities);
    static const char* rendererName(VideoRendererType type);
    static VideoRendererPtr create(VideoRendererType type);
};

}  // namespace vp::render
