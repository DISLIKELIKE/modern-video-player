#include "core/frame.h"

extern "C" {
#include <libavutil/frame.h>
}

namespace vp::core {

/// 构造时直接分配 `AVFrame`，避免队列流转中重复判空分配。
VideoFrame::VideoFrame() : frame(av_frame_alloc()) {}

VideoFrame::~VideoFrame() {
    if (frame) {
        av_frame_free(&frame);
    }
}

/// 移动构造只转移帧所有权，不复制底层像素数据。
VideoFrame::VideoFrame(VideoFrame&& other) noexcept
    : frame(other.frame), pts(other.pts), duration(other.duration), valid(other.valid) {
    other.frame = nullptr;
    other.pts = 0.0;
    other.duration = 0.0;
    other.valid = false;
}

/// 移动赋值会先释放旧帧，再接管新的 `AVFrame` 所有权。
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

/// 返回帧宽度；底层 `AVFrame` 为空时返回 0。
int VideoFrame::getWidth() const {
    return frame ? frame->width : 0;
}

/// 返回帧高度；底层 `AVFrame` 为空时返回 0。
int VideoFrame::getHeight() const {
    return frame ? frame->height : 0;
}

/// 返回像素格式；底层 `AVFrame` 为空时返回 `AV_PIX_FMT_NONE`。
AVPixelFormat VideoFrame::getFormat() const {
    return frame ? static_cast<AVPixelFormat>(frame->format) : AV_PIX_FMT_NONE;
}

}  // namespace vp::core

