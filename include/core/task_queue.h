#pragma once

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <queue>
#include <utility>

namespace vp::core {

template <typename Task>
class TaskQueue {
public:
    /// 创建有界任务队列。
    explicit TaskQueue(size_t capacity = 256) : capacity_(capacity) {}

    /// 在超时时间内尝试入队；停止态或队列持续满时返回 false。
    bool enqueue(Task task, std::chrono::milliseconds timeout = std::chrono::milliseconds(100)) {
        std::unique_lock<std::mutex> lock(mutex_);
        if (!not_full_cv_.wait_for(lock, timeout, [this] { return stopped_ || queue_.size() < capacity_; })) {
            return false;
        }
        if (stopped_) {
            return false;
        }
        queue_.push(std::move(task));
        not_empty_cv_.notify_one();
        return true;
    }

    /// 在超时时间内尝试出队；停止后且队列为空时返回 false。
    bool dequeue(Task& task, std::chrono::milliseconds timeout = std::chrono::milliseconds(100)) {
        std::unique_lock<std::mutex> lock(mutex_);
        if (!not_empty_cv_.wait_for(lock, timeout, [this] { return stopped_ || !queue_.empty(); })) {
            return false;
        }
        if (queue_.empty()) {
            return false;
        }
        task = std::move(queue_.front());
        queue_.pop();
        not_full_cv_.notify_one();
        return true;
    }

    /// 清空队列中的全部待处理任务。
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        while (!queue_.empty()) {
            queue_.pop();
        }
        not_full_cv_.notify_all();
    }

    /// 切换到停止态，并唤醒全部等待线程。
    void stop() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            stopped_ = true;
        }
        not_empty_cv_.notify_all();
        not_full_cv_.notify_all();
    }

    /// 恢复到工作态。
    void start() {
        std::lock_guard<std::mutex> lock(mutex_);
        stopped_ = false;
    }

    /// 返回当前待处理任务数。
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

    /// 返回队列是否为空。
    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }

private:
    std::queue<Task> queue_;
    size_t capacity_;
    bool stopped_{false};
    mutable std::mutex mutex_;
    std::condition_variable not_empty_cv_;
    std::condition_variable not_full_cv_;
};

}  // namespace vp::core