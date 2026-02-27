#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>

namespace vp {

template<typename T>
class ThreadSafeQueue {
public:
    explicit ThreadSafeQueue(size_t max_size = 100)
        : max_size_(max_size), stopped_(false), eof_(false) {}

    ~ThreadSafeQueue() {
        stop();
    }

    bool push(T item, int timeout_ms = 100) {
        std::unique_lock<std::mutex> lock(mutex_);

        if (stopped_.load()) {
            return false;
        }

        if (cv_.wait_for(lock, std::chrono::milliseconds(timeout_ms),
                        [this] { return queue_.size() < max_size_ || stopped_.load(); })) {
            if (stopped_.load()) {
                return false;
            }
            if (queue_.size() < max_size_) {
                queue_.push(std::move(item));
                cv_.notify_one();
                return true;
            }
        }
        return false;
    }

    bool pop(T& item, int timeout_ms = 100) {
        std::unique_lock<std::mutex> lock(mutex_);

        if (cv_.wait_for(lock, std::chrono::milliseconds(timeout_ms),
                        [this] { return !queue_.empty() || stopped_.load() || eof_.load(); })) {
            if (stopped_.load() && queue_.empty()) {
                return false;
            }
            if (!queue_.empty()) {
                item = std::move(queue_.front());
                queue_.pop();
                cv_.notify_one();
                return true;
            }
        }
        return false;
    }

    bool tryPop(T& item) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!queue_.empty()) {
            item = std::move(queue_.front());
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
        stopped_.store(true);
        cv_.notify_all();
    }

    void start() {
        stopped_.store(false);
        eof_.store(false);
    }

    void setEof(bool eof) {
        eof_.store(eof);
        cv_.notify_all();
    }

    bool isEof() const { return eof_.load(); }
    bool isStopped() const { return stopped_.load(); }
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

private:
    std::queue<T> queue_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    size_t max_size_;
    std::atomic<bool> stopped_;
    std::atomic<bool> eof_;
};

}
