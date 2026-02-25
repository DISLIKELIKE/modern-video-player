#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>

namespace vp {

template<typename T>
class FrameQueue {
public:
    explicit FrameQueue(size_t max_size = 10)
        : max_size_(max_size), stopping_(false) {}

    ~FrameQueue() {
        stop();
    }

    bool push(T&& frame) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (stopping_.load()) {
            return false;
        }
        if (queue_.size() >= max_size_) {
            return false;
        }
        queue_.push(std::move(frame));
        cv_.notify_one();
        return true;
    }

    bool push(const T& frame) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (stopping_.load()) {
            return false;
        }
        if (queue_.size() >= max_size_) {
            return false;
        }
        queue_.push(frame);
        cv_.notify_one();
        return true;
    }

    bool pushWithWait(T&& frame, int timeout_ms = 100) {
        std::unique_lock<std::mutex> lock(mutex_);
        
        if (cv_.wait_for(lock, std::chrono::milliseconds(timeout_ms),
                        [this] { return queue_.size() < max_size_ || stopping_.load(); })) {
            if (stopping_.load()) {
                return false;
            }
            if (queue_.size() < max_size_) {
                queue_.push(std::move(frame));
                cv_.notify_one();
                return true;
            }
        }
        return false;
    }

    bool pushWithWait(const T& frame, int timeout_ms = 100) {
        std::unique_lock<std::mutex> lock(mutex_);
        
        if (cv_.wait_for(lock, std::chrono::milliseconds(timeout_ms),
                        [this] { return queue_.size() < max_size_ || stopping_.load(); })) {
            if (stopping_.load()) {
                return false;
            }
            if (queue_.size() < max_size_) {
                queue_.push(frame);
                cv_.notify_one();
                return true;
            }
        }
        return false;
    }

    bool pop(T& frame, int timeout_ms = 100) {
        std::unique_lock<std::mutex> lock(mutex_);
        
        if (cv_.wait_for(lock, std::chrono::milliseconds(timeout_ms), 
                        [this] { return !queue_.empty() || stopping_.load(); })) {
            if (stopping_.load() && queue_.empty()) {
                return false;
            }
            if (!queue_.empty()) {
                frame = std::move(queue_.front());
                queue_.pop();
                cv_.notify_one();
                return true;
            }
        }
        return false;
    }

    bool tryPop(T& frame) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!queue_.empty()) {
            frame = std::move(queue_.front());
            queue_.pop();
            cv_.notify_one();
            return true;
        }
        return false;
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        while (!queue_.empty()) {
            queue_.pop();
        }
        cv_.notify_all();
    }

    void stop() {
        stopping_.store(true);
        cv_.notify_all();
    }

    void start() {
        stopping_.store(false);
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }

    bool full() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size() >= max_size_;
    }

    size_t capacity() const { return max_size_; }

private:
    std::queue<T> queue_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    size_t max_size_;
    std::atomic<bool> stopping_;
};

}
