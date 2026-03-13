#pragma once

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <string>

namespace vp::core {

/// 播放控制命令类型。
enum class CommandType {
    Play,
    Pause,
    Stop,
    Seek,
    SetVolume,
    SetSpeed
};

/// 通用控制命令载体；根据 `type` 读取对应参数字段。
struct Command {
    CommandType type{CommandType::Play};
    double double_value{0.0};
    float float_value{0.0f};
    std::string string_value;
};

/// 线程安全命令队列；用于在线程间传递离散控制请求。
class CommandQueue {
public:
    /// 推入一个控制命令并唤醒等待消费者。
    void push(Command cmd) {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(std::move(cmd));
        cv_.notify_one();
    }

    /// 在超时时间内尝试取出一条命令。
    bool pop(Command& cmd, std::chrono::milliseconds timeout) {
        std::unique_lock<std::mutex> lock(mutex_);
        if (!cv_.wait_for(lock, timeout, [this] { return !queue_.empty(); })) {
            return false;
        }
        cmd = std::move(queue_.front());
        queue_.pop();
        return true;
    }

    /// 清空所有待处理命令。
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