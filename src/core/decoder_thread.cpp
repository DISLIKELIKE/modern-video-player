#include "core/decoder_thread.h"

#include <chrono>

namespace vp::core {

DecoderThread::~DecoderThread() {
    stop();
}

/// 启动后台线程；若线程已存在，则仅恢复暂停态。
void DecoderThread::start() {
    if (running_.exchange(true)) {
        resume();
        return;
    }

    paused_.store(false);
    worker_ = std::thread(&DecoderThread::threadMain, this);
}

/// 标记线程进入暂停态；真正阻塞发生在下一次循环检查。
void DecoderThread::pause() {
    if (!running_.load()) {
        return;
    }
    paused_.store(true);
}

/// 清除暂停标记并唤醒工作线程继续处理任务。
void DecoderThread::resume() {
    if (!running_.load()) {
        return;
    }
    paused_.store(false);
    cv_.notify_all();
}

/// 请求线程停止，触发停止回调并等待工作线程退出。
void DecoderThread::stop() {
    if (!running_.exchange(false)) {
        return;
    }

    paused_.store(false);
    onStopRequested();
    cv_.notify_all();

    if (worker_.joinable()) {
        worker_.join();
    }
}

bool DecoderThread::isRunning() const {
    return running_.load();
}

bool DecoderThread::isPaused() const {
    return paused_.load();
}

/// 在暂停态阻塞等待；若运行标记已清除则返回 false。
bool DecoderThread::waitIfPausedOrStopped() {
    if (!paused_.load()) {
        return running_.load();
    }

    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait(lock, [this] { return !paused_.load() || !running_.load(); });
    return running_.load();
}

/// 工作线程主循环；反复执行 `onLoop()`，并在暂停/停止时协调状态。
void DecoderThread::threadMain() {
    while (running_.load()) {
        if (!waitIfPausedOrStopped()) {
            break;
        }

        onLoop();
        if (!running_.load()) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

}  // namespace vp::core

