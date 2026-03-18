#pragma once

#include "render/video_renderer.h"

namespace vp::render {

class RendererFactory {
public:
    static VideoRendererType detectBestRendererType();
    static VideoRendererPtr create(VideoRendererType type);
};

}  // namespace vp::render
