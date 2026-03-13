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
    /// 创建带最大容量限制的线程安全队列。
    explicit ThreadSafeQueue(size_t max_size = 100)
        : max_size_(max_size), stopped_(false), eof_(false) {}

    /// 析构时主动停止队列，唤醒所有等待线程。
    ~ThreadSafeQueue() {
        stop();
    }

    /// 在超时时间内尝试入队；停止态或队列持续满时返回 false。
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

    /// 在超时时间内尝试出队；停止且为空或超时时返回 false。
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

    /// 非阻塞尝试出队。
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

    /// 清空当前队列内容并唤醒等待线程。
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        while (!queue_.empty()) {
            queue_.pop();
        }
        cv_.notify_all();
    }

    /// 将队列切换到停止态，并唤醒等待中的生产者/消费者。
    void stop() {
        stopped_.store(true);
        cv_.notify_all();
    }

    /// 将队列恢复到工作态，并清除 EOF 标记。
    void start() {
        stopped_.store(false);
        eof_.store(false);
    }

    /// 设置 EOF 标记并唤醒等待线程。
    void setEof(bool eof) {
        eof_.store(eof);
        cv_.notify_all();
    }

    /// 返回是否已标记 EOF。
    bool isEof() const { return eof_.load(); }
    /// 返回队列是否处于停止态。
    bool isStopped() const { return stopped_.load(); }
    /// 返回当前元素个数。
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }
    /// 返回队列是否为空。
    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }
    /// 返回队列是否已满。
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