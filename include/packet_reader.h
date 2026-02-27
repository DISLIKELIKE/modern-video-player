#pragma once

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <memory>
#include <thread>

namespace vp {

class PacketRef {
public:
    PacketRef();
    ~PacketRef();

    PacketRef(PacketRef&& other) noexcept;
    PacketRef& operator=(PacketRef&& other) noexcept;

    PacketRef(const PacketRef&) = delete;
    PacketRef& operator=(const PacketRef&) = delete;

    bool isValid() const { return packet_ != nullptr; }
    AVPacket* get() { return packet_; }
    const AVPacket* get() const { return packet_; }

    int getStreamIndex() const { return stream_index_; }
    void setStreamIndex(int idx) { stream_index_ = idx; }

    void reset();

private:
    AVPacket* packet_;
    int stream_index_;
};

template<typename T>
class PacketQueue {
public:
    explicit PacketQueue(size_t max_size = 100)
        : max_size_(max_size), stopping_(false), eof_(false) {}

    ~PacketQueue() {
        stop();
    }

    bool push(T&& packet) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (stopping_.load()) {
            return false;
        }
        if (queue_.size() >= max_size_) {
            return false;
        }
        queue_.push(std::move(packet));
        cv_.notify_one();
        return true;
    }

    bool pushWithWait(T&& packet, int timeout_ms = 100) {
        std::unique_lock<std::mutex> lock(mutex_);

        if (cv_.wait_for(lock, std::chrono::milliseconds(timeout_ms),
                        [this] { return queue_.size() < max_size_ || stopping_.load(); })) {
            if (stopping_.load()) {
                return false;
            }
            if (queue_.size() < max_size_) {
                queue_.push(std::move(packet));
                cv_.notify_one();
                return true;
            }
        }
        return false;
    }

    bool pop(T& packet, int timeout_ms = 100) {
        std::unique_lock<std::mutex> lock(mutex_);

        if (cv_.wait_for(lock, std::chrono::milliseconds(timeout_ms),
                        [this] { return !queue_.empty() || stopping_.load() || eof_.load(); })) {
            if (stopping_.load() && queue_.empty()) {
                return false;
            }
            if (!queue_.empty()) {
                packet = std::move(queue_.front());
                queue_.pop();
                cv_.notify_one();
                return true;
            }
            if (eof_.load() && queue_.empty()) {
                return false;
            }
        }
        return false;
    }

    bool tryPop(T& packet) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!queue_.empty()) {
            packet = std::move(queue_.front());
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
        eof_.store(false);
    }

    void setEof(bool eof) {
        std::lock_guard<std::mutex> lock(mutex_);
        eof_.store(eof);
        cv_.notify_all();
    }

    bool isEof() const {
        return eof_.load();
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
    std::atomic<bool> eof_;
};

class PacketReaderThread {
public:
    PacketReaderThread();
    ~PacketReaderThread();

    bool start(AVFormatContext* fmt_ctx, int video_stream_idx, int audio_stream_idx,
               PacketQueue<PacketRef>* video_queue, PacketQueue<PacketRef>* audio_queue);
    void stop();
    void pause();
    void resume();
    void flush();

    bool isRunning() const { return running_.load(); }
    bool isEof() const { return eof_reached_.load(); }

private:
    void readLoop();

    AVFormatContext* format_ctx_;
    int video_stream_idx_;
    int audio_stream_idx_;

    PacketQueue<PacketRef>* video_queue_;
    PacketQueue<PacketRef>* audio_queue_;

    std::thread thread_;
    std::mutex mutex_;
    std::condition_variable cv_;

    std::atomic<bool> running_;
    std::atomic<bool> paused_;
    std::atomic<bool> stop_requested_;
    std::atomic<bool> eof_reached_;
};

}
