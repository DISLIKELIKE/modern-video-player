#pragma once

#include "render/video_renderer.h"

namespace vp::render {

/// 渲染器工厂；负责平台默认后端判断和实例创建。
class RendererFactory {
public:
    /// 依据当前平台返回推荐渲染后端类型。
    static VideoRendererType detectBestRendererType();
    /// 创建指定类型的渲染器；`Auto` 会先解析为平台默认值。
    static VideoRendererPtr create(VideoRendererType type);
};

}  // namespace vp::render