#include "render/renderer_factory.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <optional>
#include <string>

#include "logger.h"
#include "render/d3d11_video_renderer.h"
#include "render/opengl_video_renderer.h"
#include "render/sdl_video_renderer.h"

namespace vp::render {

namespace {

std::string toLowerAscii(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return text;
}

std::optional<std::string> readEnvVar(const char* key) {
    if (!key || key[0] == '\0') {
        return std::nullopt;
    }
#if defined(_WIN32)
    char* value = nullptr;
    size_t length = 0;
    if (_dupenv_s(&value, &length, key) != 0 || !value) {
        return std::nullopt;
    }
    std::string copy(value);
    std::free(value);
    return copy;
#else
    const char* value = std::getenv(key);
    if (!value) {
        return std::nullopt;
    }
    return std::string(value);
#endif
}

}  // namespace

VideoRendererType RendererFactory::detectBestRendererType() {
    if (const auto backend_override = readEnvVar("MVP_RENDERER_BACKEND")) {
        const std::string value = toLowerAscii(*backend_override);
        if (value == "software" || value == "softwaresdl" || value == "sdl") {
            return VideoRendererType::SoftwareSDL;
        }
        if (value == "d3d11") {
            return VideoRendererType::D3D11;
        }
        if (value == "opengl" || value == "gl") {
            return VideoRendererType::OpenGL;
        }
        LOG_WARNING("Unknown MVP_RENDERER_BACKEND override: " << *backend_override);
    }

#if defined(_WIN32)
    if (const auto d3d11_driver_hint = readEnvVar("MVP_D3D11_DRIVER_HINT")) {
        const std::string value = toLowerAscii(*d3d11_driver_hint);
        if (value == "software") {
            return VideoRendererType::SoftwareSDL;
        }
    }
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
        return std::make_unique<D3D11VideoRenderer>();
    case VideoRendererType::OpenGL:
        return std::make_unique<OpenGLVideoRenderer>();
    case VideoRendererType::Auto:
    default:
        return std::make_unique<SdlVideoRenderer>();
    }
}

}  // namespace vp::render