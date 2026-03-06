#pragma once

extern "C" {
#include <libavutil/frame.h>
#include <libavutil/pixfmt.h>
}

#include <cstdint>
#include <vector>

namespace vp::core {

struct VideoFrame {
    AVFrame* frame{nullptr};
    double pts{0.0};
    double duration{0.0};
    bool valid{false};

    VideoFrame();
    ~VideoFrame();

    VideoFrame(const VideoFrame&) = delete;
    VideoFrame& operator=(const VideoFrame&) = delete;
    VideoFrame(VideoFrame&& other) noexcept;
    VideoFrame& operator=(VideoFrame&& other) noexcept;

    int getWidth() const;
    int getHeight() const;
    AVPixelFormat getFormat() const;
};

struct AudioFrame {
    std::vector<uint8_t> samples;
    double pts{0.0};
    double duration{0.0};
    int sample_rate{0};
    int channels{0};
    bool valid{false};
};

}  // namespace vp::core
