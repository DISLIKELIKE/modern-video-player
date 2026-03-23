#pragma once



#include <atomic>

#include <chrono>

#include <condition_variable>

#include <cstddef>

#include <cstdint>

#include <mutex>

#include <queue>

#include <utility>



namespace vp {



template <typename T>

class ThreadSafeQueue {

public:

    explicit ThreadSafeQueue(size_t max_size = 100)

        : max_size_(max_size), stopped_(false), eof_(false) {}



    ~ThreadSafeQueue() {

        stop();

    }



    bool push(const T& item, int timeout_ms = 100) {

        std::unique_lock<std::mutex> lock(mutex_);

        const uint64_t observed_generation = generation_.load();



        if (stopped_.load()) {

            return false;

        }



        if (cv_.wait_for(lock, std::chrono::milliseconds(timeout_ms),

                         [this, observed_generation] {

                             return queue_.size() < max_size_ || stopped_.load() ||

                                    generation_.load() != observed_generation;

                         })) {

            if (stopped_.load() || generation_.load() != observed_generation) {

                return false;

            }

            if (queue_.size() < max_size_) {

                queue_.push(item);

                cv_.notify_one();

                return true;

            }

        }

        return false;

    }



    bool push(T&& item, int timeout_ms = 100) {

        std::unique_lock<std::mutex> lock(mutex_);

        const uint64_t observed_generation = generation_.load();



        if (stopped_.load()) {

            return false;

        }



        if (cv_.wait_for(lock, std::chrono::milliseconds(timeout_ms),

                         [this, observed_generation] {

                             return queue_.size() < max_size_ || stopped_.load() ||

                                    generation_.load() != observed_generation;

                         })) {

            if (stopped_.load() || generation_.load() != observed_generation) {

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

        const uint64_t observed_generation = generation_.load();



        if (cv_.wait_for(lock, std::chrono::milliseconds(timeout_ms),

                         [this, observed_generation] {

                             return !queue_.empty() || stopped_.load() || eof_.load() ||

                                    generation_.load() != observed_generation;

                         })) {

            if ((stopped_.load() && queue_.empty()) || generation_.load() != observed_generation) {

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

        generation_.fetch_add(1);

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

    uint64_t generation() const { return generation_.load(); }



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

    std::atomic<uint64_t> generation_{1};

};



}  // namespace vp

