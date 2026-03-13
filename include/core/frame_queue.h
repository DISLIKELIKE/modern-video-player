#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <queue>

#include "core/frame.h"

namespace vp::core {

template <typename FrameType>
class FrameQueue {
public:
    /// 创建定长生产者/消费者队列。
    explicit FrameQueue(size_t capacity = 8) : capacity_(capacity), flushed_(false) {}
    /// 默认析构，不接管帧内外部资源。
    ~FrameQueue() = default;

    /// 在超时时间内尝试入队；队列满或被 flush 唤醒时返回 false。
    bool push(FrameType&& frame, std::chrono::milliseconds timeout = std::chrono::milliseconds(100)) {
        std::unique_lock<std::mutex> lock(mutex_);
        const bool has_space = not_full_.wait_for(lock, timeout, [this] {
            return queue_.size() < capacity_ || flushed_.load();
        });
        if (!has_space || flushed_.load()) {
            return false;
        }
        queue_.push(std::move(frame));
        not_empty_.notify_one();
        return true;
    }

    /// 在超时时间内尝试出队；队列空或超时时返回 false。
    bool pop(FrameType& frame, std::chrono::milliseconds timeout = std::chrono::milliseconds(100)) {
        std::unique_lock<std::mutex> lock(mutex_);
        const bool has_item = not_empty_.wait_for(lock, timeout, [this] {
            return !queue_.empty() || flushed_.load();
        });
        if (!has_item || queue_.empty()) {
            return false;
        }
        frame = std::move(queue_.front());
        queue_.pop();
        not_full_.notify_one();
        return true;
    }

    /// 清空全部待处理帧，并唤醒当前阻塞的生产者/消费者。
    void flush() {
        std::lock_guard<std::mutex> lock(mutex_);
        while (!queue_.empty()) {
            queue_.pop();
        }
        flushed_.store(true);
        not_empty_.notify_all();
        not_full_.notify_all();
        flushed_.store(false);
    }

    /// 动态调整容量，并唤醒等待中的生产者。
    void setCapacity(size_t capacity) {
        std::lock_guard<std::mutex> lock(mutex_);
        capacity_ = capacity;
        not_full_.notify_all();
    }

    /// 返回当前排队帧数。
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

    /// 返回当前配置容量。
    size_t capacity() const {
        return capacity_;
    }

    /// 返回队列是否为空。
    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }

    /// 返回队列是否已满。
    bool full() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size() >= capacity_;
    }

    /// 返回填充比例，范围通常为 [0, 1]。
    double getFillRatio() const {
        std::lock_guard<std::mutex> lock(mutex_);
        if (capacity_ == 0) {
            return 1.0;
        }
        return static_cast<double>(queue_.size()) / static_cast<double>(capacity_);
    }

private:
    std::queue<FrameType> queue_;
    mutable std::mutex mutex_;
    std::condition_variable not_empty_;
    std::condition_variable not_full_;
    size_t capacity_;
    std::atomic<bool> flushed_;
};

using VideoFrameQueue = FrameQueue<VideoFrame>;
using AudioFrameQueue = FrameQueue<AudioFrame>;

}  // namespace vp::core
