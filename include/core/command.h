#pragma once

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <string>

namespace vp::core {

enum class CommandType {
    Play,
    Pause,
    Stop,
    Seek,
    SetVolume,
    SetSpeed
};

struct Command {
    CommandType type{CommandType::Play};
    double double_value{0.0};
    float float_value{0.0f};
    std::string string_value;
};

class CommandQueue {
public:
    void push(Command cmd) {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(std::move(cmd));
        cv_.notify_one();
    }

    bool pop(Command& cmd, std::chrono::milliseconds timeout) {
        std::unique_lock<std::mutex> lock(mutex_);
        if (!cv_.wait_for(lock, timeout, [this] { return !queue_.empty(); })) {
            return false;
        }
        cmd = std::move(queue_.front());
        queue_.pop();
        return true;
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        while (!queue_.empty()) {
            queue_.pop();
        }
    }

private:
    std::queue<Command> queue_;
    std::mutex mutex_;
    std::condition_variable cv_;
};

}  // namespace vp::core

