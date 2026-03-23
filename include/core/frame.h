#pragma once

extern "C" {
#include <libavutil/frame.h>
#include <libavutil/pixfmt.h>
}

#include <cstdint>
#include <vector>

namespace vp::core {

using TimelineSerial = uint64_t;
constexpr TimelineSerial kInvalidTimelineSerial = 0;

/// 视频帧封装；拥有一个 `AVFrame` 并附带时间戳、时长等调度元数据。
struct VideoFrame {
    /// 持有的 FFmpeg 帧对象，所有权归 `VideoFrame`。
    AVFrame* frame{nullptr};
    /// 显示时间戳（秒）。
    double pts{0.0};
    /// 该帧估算显示时长（秒）。
    double duration{0.0};
    /// 该帧所属的时间线序号。
    TimelineSerial serial{kInvalidTimelineSerial};
    /// 标记该帧是否可用于渲染。
    bool valid{false};

    /// 分配内部 `AVFrame`。
    VideoFrame();
    /// 释放内部 `AVFrame`。
    ~VideoFrame();

    VideoFrame(const VideoFrame&) = delete;
    VideoFrame& operator=(const VideoFrame&) = delete;
    /// 移动构造会转移 `AVFrame` 所有权。
    VideoFrame(VideoFrame&& other) noexcept;
    /// 移动赋值会先释放旧帧，再接管新帧所有权。
    VideoFrame& operator=(VideoFrame&& other) noexcept;

    /// 返回帧宽度；空帧时返回 0。
    int getWidth() const;
    /// 返回帧高度；空帧时返回 0。
    int getHeight() const;
    /// 返回像素格式；空帧时返回 `AV_PIX_FMT_NONE`。
    AVPixelFormat getFormat() const;
};

/// 音频帧封装；保存重采样后的 PCM 数据及时间信息。
struct AudioFrame {
    std::vector<uint8_t> samples;
    double pts{0.0};
    double duration{0.0};
    TimelineSerial serial{kInvalidTimelineSerial};
    int sample_rate{0};
    int channels{0};
    bool valid{false};
};

}  // namespace vp::core
