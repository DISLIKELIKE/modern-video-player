#include "core/frame.h"

extern "C" {
#include <libavutil/frame.h>
}

namespace vp::core {

VideoFrame::VideoFrame() : frame(av_frame_alloc()) {}

VideoFrame::~VideoFrame() {
    if (frame) {
        av_frame_free(&frame);
    }
}

VideoFrame::VideoFrame(VideoFrame&& other) noexcept
    : frame(other.frame), pts(other.pts), duration(other.duration), valid(other.valid) {
    other.frame = nullptr;
    other.pts = 0.0;
    other.duration = 0.0;
    other.valid = false;
}

VideoFrame& VideoFrame::operator=(VideoFrame&& other) noexcept {
    if (this == &other) {
        return *this;
    }

    if (frame) {
        av_frame_free(&frame);
    }

    frame = other.frame;
    pts = other.pts;
    duration = other.duration;
    valid = other.valid;

    other.frame = nullptr;
    other.pts = 0.0;
    other.duration = 0.0;
    other.valid = false;
    return *this;
}

int VideoFrame::getWidth() const {
    return frame ? frame->width : 0;
}

int VideoFrame::getHeight() const {
    return frame ? frame->height : 0;
}

AVPixelFormat VideoFrame::getFormat() const {
    return frame ? static_cast<AVPixelFormat>(frame->format) : AV_PIX_FMT_NONE;
}

}  // namespace vp::core

