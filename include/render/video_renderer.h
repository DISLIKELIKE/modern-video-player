#pragma once

#include <memory>
#include <string>

#include "core/frame.h"

namespace vp::render {

enum class VideoRendererType {
    Auto,
    SoftwareSDL,
    D3D11,
    OpenGL
};

struct VideoRendererConfig {
    int width{0};
    int height{0};
    std::string title{"Video Player"};
};

class IVideoRenderer {
public:
    virtual ~IVideoRenderer() = default;

    virtual bool init(const VideoRendererConfig& config) = 0;
    virtual void close() = 0;

    virtual void renderFrame(const core::VideoFrame& frame) = 0;
    virtual void present() = 0;
    virtual void clear() = 0;

    virtual void handleEvents() = 0;
    virtual bool shouldQuit() const = 0;

    virtual bool consumeTogglePauseRequest() = 0;
    virtual bool consumeSeekRequest(double& normalized_position) = 0;
    virtual bool consumeVolumeChangeRequest(float& volume) = 0;
    virtual void setOverlayState(double position, double duration, float volume, bool paused) = 0;
};

using VideoRendererPtr = std::unique_ptr<IVideoRenderer>;

}  // namespace vp::render

