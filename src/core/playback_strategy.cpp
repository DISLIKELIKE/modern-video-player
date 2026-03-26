#include "core/playback_strategy.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <optional>
#include <string>

#include "decoder/decoder_factory.h"

namespace vp::core {

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

void appendRendererUnique(std::vector<render::VideoRendererType>& out, render::VideoRendererType type) {
    if (type == render::VideoRendererType::Auto) {
        return;
    }
    if (std::find(out.begin(), out.end(), type) == out.end()) {
        out.push_back(type);
    }
}

std::optional<render::VideoRendererType> parseRendererOverride(const std::optional<std::string>& value) {
    if (!value || value->empty()) {
        return std::nullopt;
    }
    const std::string normalized = toLowerAscii(*value);
    if (normalized == "software" || normalized == "softwaresdl" || normalized == "sdl") {
        return render::VideoRendererType::SoftwareSDL;
    }
    if (normalized == "d3d11" || normalized == "direct3d11") {
        return render::VideoRendererType::D3D11;
    }
    if (normalized == "opengl" || normalized == "gl") {
        return render::VideoRendererType::OpenGL;
    }
    return std::nullopt;
}

std::vector<platform::RendererSupport> sortedRendererSupport(
    const std::vector<platform::RendererSupport>& support_list) {
    std::vector<platform::RendererSupport> sorted = support_list;
    std::sort(sorted.begin(),
              sorted.end(),
              [](const platform::RendererSupport& lhs, const platform::RendererSupport& rhs) {
                  return lhs.default_priority > rhs.default_priority;
              });
    return sorted;
}

}  // namespace

PlaybackOpenPlan PlaybackStrategy::buildOpenPlan(const PlaybackOpenRequest& request) {
    PlaybackOpenPlan plan;
    plan.allow_hardware_decode = request.preferences.prefer_hardware_decode;

    const bool has_video_stream = request.media_info.video_stream_idx >= 0;
    if (!has_video_stream) {
        plan.renderer_plan_reason = "no-video-stream";
        plan.decoder_plan_reason = "no-video-stream";
        return plan;
    }

    const auto renderer_override = parseRendererOverride(readEnvVar("MVP_RENDERER_BACKEND"));
    const bool has_renderer_override = renderer_override.has_value();

    std::vector<render::VideoRendererType> renderer_candidates;
    if (has_renderer_override) {
        appendRendererUnique(renderer_candidates, *renderer_override);
        plan.renderer_plan_reason = "renderer-override-env";
    } else if (request.preferences.preferred_renderer != render::VideoRendererType::Auto) {
        appendRendererUnique(renderer_candidates, request.preferences.preferred_renderer);
        plan.renderer_plan_reason = "renderer-override-preference";
    } else {
        plan.renderer_plan_reason = "platform-default-order";
    }

    const auto d3d11_driver_hint = readEnvVar("MVP_D3D11_DRIVER_HINT");
    const bool prefer_software_due_to_hint =
        !has_renderer_override &&
        request.platform_capabilities.platform == platform::PlatformKind::Windows &&
        d3d11_driver_hint.has_value() &&
        toLowerAscii(*d3d11_driver_hint) == "software";

    const auto sorted_support = sortedRendererSupport(request.platform_capabilities.renderer_support);
    if (prefer_software_due_to_hint) {
        appendRendererUnique(renderer_candidates, render::VideoRendererType::SoftwareSDL);
        plan.renderer_plan_reason = "d3d11-driver-hint-software";
    }
    for (const platform::RendererSupport& support : sorted_support) {
        if (!support.compiled_in || !support.runtime_available) {
            continue;
        }
        appendRendererUnique(renderer_candidates, support.type);
    }
    plan.renderer_candidates = std::move(renderer_candidates);

    decoder::DecoderSelectionContext decoder_context{};
    decoder_context.codec_name = request.video_codec_name.empty() ? "unknown" : request.video_codec_name;
    decoder_context.prefer_hardware = plan.allow_hardware_decode;
    decoder_context.renderer_type =
        plan.renderer_candidates.empty() ? render::VideoRendererType::Auto : plan.renderer_candidates.front();
    decoder_context.platform_capabilities = request.platform_capabilities;
    plan.video_decoder_candidates = decoder::DecoderFactory::selectBackendOrder(decoder_context);

    if (!plan.allow_hardware_decode) {
        plan.decoder_plan_reason = "hardware-decode-disabled";
    } else if (!plan.video_decoder_candidates.empty() &&
               plan.video_decoder_candidates.front() == decoder::DecoderBackend::D3D11VA) {
        plan.decoder_plan_reason = "hardware-first";
    } else {
        plan.decoder_plan_reason = "software-only";
    }

    return plan;
}

}  // namespace vp::core
