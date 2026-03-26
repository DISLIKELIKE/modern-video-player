#include "subtitle/libass_probe.h"

#include <algorithm>
#include <cctype>
#include <cstdarg>
#include <filesystem>
#include <limits>
#include <memory>
#include <string>
#include <vector>

#if defined(__linux__)
#include <libass/ass.h>
#endif

namespace vp::subtitle {

namespace {

std::string toLowerAscii(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

std::string subtitleExtension(const std::string& subtitle_path) {
    const std::filesystem::path path(subtitle_path);
    std::string extension = path.extension().string();
    if (!extension.empty() && extension.front() == '.') {
        extension.erase(extension.begin());
    }
    return toLowerAscii(extension);
}

bool isSupportedLibassInputExtension(const std::string& subtitle_path) {
    const std::string extension = subtitleExtension(subtitle_path);
    return extension == "ass" || extension == "ssa";
}

#if defined(__linux__)
void ignoreLibassMessage(int, const char*, va_list, void*) {}

void collectProbeTimestamps(const ASS_Track* track,
                            int sample_duration_ms,
                            int sample_step_ms,
                            std::vector<int>& out_timestamps_ms) {
    out_timestamps_ms.clear();
    out_timestamps_ms.reserve(256);
    out_timestamps_ms.push_back(0);

    if (sample_duration_ms > 0 && sample_step_ms > 0) {
        for (int timestamp_ms = sample_step_ms; timestamp_ms <= sample_duration_ms; timestamp_ms += sample_step_ms) {
            out_timestamps_ms.push_back(timestamp_ms);
        }
    }

    if (!track || track->n_events <= 0) {
        std::sort(out_timestamps_ms.begin(), out_timestamps_ms.end());
        out_timestamps_ms.erase(std::unique(out_timestamps_ms.begin(), out_timestamps_ms.end()), out_timestamps_ms.end());
        return;
    }

    constexpr int kMaxEventsToSample = 120;
    const int event_count = std::min(track->n_events, kMaxEventsToSample);
    for (int i = 0; i < event_count; ++i) {
        const ASS_Event& event = track->events[i];
        const int start_ms = std::max(0, event.Start);
        const int duration_ms = std::max(0, event.Duration);
        const int end_ms = start_ms + duration_ms;
        const int mid_ms = start_ms + duration_ms / 2;

        out_timestamps_ms.push_back(start_ms);
        out_timestamps_ms.push_back(std::max(start_ms, mid_ms));
        if (duration_ms > 0) {
            out_timestamps_ms.push_back(std::max(start_ms, end_ms - 20));
        }
    }

    std::sort(out_timestamps_ms.begin(), out_timestamps_ms.end());
    out_timestamps_ms.erase(std::unique(out_timestamps_ms.begin(), out_timestamps_ms.end()), out_timestamps_ms.end());
}
#endif

}  // namespace

LibassShapingProbeSummary probeLibassShaping(const std::string& subtitle_path,
                                             int frame_width,
                                             int frame_height,
                                             int sample_duration_ms,
                                             int sample_step_ms) {
    LibassShapingProbeSummary summary;

    std::error_code ec;
    const std::filesystem::path subtitle_file(subtitle_path);
    summary.file_accessible =
        std::filesystem::exists(subtitle_file, ec) && !ec &&
        std::filesystem::is_regular_file(subtitle_file, ec) && !ec;
    summary.parser_supported_extension = isSupportedLibassInputExtension(subtitle_path);

#if !defined(__linux__)
    (void)frame_width;
    (void)frame_height;
    (void)sample_duration_ms;
    (void)sample_step_ms;
    summary.error = "libass shaping probe requires Linux";
    return summary;
#else
    summary.supported_platform = true;
    if (!summary.file_accessible) {
        summary.error = "subtitle file does not exist";
        return summary;
    }
    if (!summary.parser_supported_extension) {
        summary.error = "only .ass/.ssa are supported by this probe";
        return summary;
    }

    ASS_Library* raw_library = ass_library_init();
    if (!raw_library) {
        summary.error = "ass_library_init failed";
        return summary;
    }
    summary.library_initialized = true;
    std::unique_ptr<ASS_Library, decltype(&ass_library_done)> library(raw_library, ass_library_done);
    ass_set_message_cb(library.get(), ignoreLibassMessage, nullptr);

    ASS_Renderer* raw_renderer = ass_renderer_init(library.get());
    if (!raw_renderer) {
        summary.error = "ass_renderer_init failed";
        return summary;
    }
    summary.renderer_initialized = true;
    std::unique_ptr<ASS_Renderer, decltype(&ass_renderer_done)> renderer(raw_renderer, ass_renderer_done);

    const int safe_frame_width = std::max(320, frame_width);
    const int safe_frame_height = std::max(180, frame_height);
    ass_set_frame_size(renderer.get(), safe_frame_width, safe_frame_height);
    ass_set_storage_size(renderer.get(), safe_frame_width, safe_frame_height);
    ass_set_fonts(renderer.get(), nullptr, "sans-serif", 1, nullptr, 1);

    ASS_Track* raw_track = ass_read_file(library.get(),
                                         const_cast<char*>(subtitle_path.c_str()),
                                         nullptr);
    if (!raw_track) {
        summary.error = "ass_read_file failed";
        return summary;
    }
    summary.track_loaded = true;
    std::unique_ptr<ASS_Track, decltype(&ass_free_track)> track(raw_track, ass_free_track);

    summary.events_loaded = track->n_events > 0;
    if (!summary.events_loaded) {
        summary.error = "subtitle track has no events";
        return summary;
    }

    std::vector<int> probe_timestamps_ms;
    collectProbeTimestamps(track.get(),
                           std::max(0, sample_duration_ms),
                           std::max(50, sample_step_ms),
                           probe_timestamps_ms);

    for (int timestamp_ms : probe_timestamps_ms) {
        int changed = 0;
        ASS_Image* image = ass_render_frame(renderer.get(), track.get(), timestamp_ms, &changed);
        ++summary.sampled_timestamps;
        if (changed != 0) {
            ++summary.changed_frames;
        }

        for (ASS_Image* iter = image; iter != nullptr; iter = iter->next) {
            if (iter->w <= 0 || iter->h <= 0 || !iter->bitmap) {
                continue;
            }
            ++summary.rendered_images;
            summary.max_image_width = std::max(summary.max_image_width, iter->w);
            summary.max_image_height = std::max(summary.max_image_height, iter->h);
        }
    }

    summary.rendered_output = summary.rendered_images > 0;
    if (!summary.rendered_output) {
        summary.error = "no rendered libass image produced";
    }
    return summary;
#endif
}

}  // namespace vp::subtitle
