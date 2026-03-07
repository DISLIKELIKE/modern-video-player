#include "core/frame_pool.h"

namespace vp::core {

FramePool::FramePool(size_t capacity) : capacity_(capacity) {
    idle_frames_.reserve(capacity_);
    for (size_t i = 0; i < capacity_; ++i) {
        AVFrame* frame = av_frame_alloc();
        if (frame) {
            idle_frames_.push_back(frame);
        }
    }
}

FramePool::~FramePool() {
    clear();
}

AVFrame* FramePool::acquire() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (idle_frames_.empty()) {
        return av_frame_alloc();
    }

    AVFrame* frame = idle_frames_.back();
    idle_frames_.pop_back();
    av_frame_unref(frame);
    return frame;
}

void FramePool::release(AVFrame* frame) {
    if (!frame) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    av_frame_unref(frame);
    if (idle_frames_.size() >= capacity_) {
        av_frame_free(&frame);
        return;
    }
    idle_frames_.push_back(frame);
}

void FramePool::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    for (AVFrame* frame : idle_frames_) {
        if (frame) {
            av_frame_free(&frame);
        }
    }
    idle_frames_.clear();
}

size_t FramePool::capacity() const {
    return capacity_;
}

size_t FramePool::available() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return idle_frames_.size();
}

}  // namespace vp::core

