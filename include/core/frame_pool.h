#pragma once

extern "C" {
#include <libavutil/frame.h>
}

#include <cstddef>
#include <memory>
#include <mutex>
#include <vector>

namespace vp::core {

class FramePool {
public:
    explicit FramePool(size_t capacity = 32);
    ~FramePool();

    AVFrame* acquire();
    void release(AVFrame* frame);
    void clear();

    size_t capacity() const;
    size_t available() const;

private:
    size_t capacity_;
    mutable std::mutex mutex_;
    std::vector<AVFrame*> idle_frames_;
};

using FramePoolPtr = std::shared_ptr<FramePool>;

}  // namespace vp::core

