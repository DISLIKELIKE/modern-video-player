#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

namespace vp::core {

/// 可暂停的工作线程基类；子类只需实现 `onLoop()`。
class DecoderThread {
public:
    DecoderThread() = default;
    /// 析构时自动停止并回收后台线程。
    virtual ~DecoderThread();

    DecoderThread(const DecoderThread&) = delete;
    DecoderThread& operator=(const DecoderThread&) = delete;

    /// 启动后台线程；重复调用会退化为 `resume()`。
    void start();
    /// 请求线程进入暂停态。
    void pause();
    /// 恢复暂停中的线程。
    void resume();
    /// 请求线程停止并等待退出。
    void stop();

    /// 返回线程是否处于运行态。
    bool isRunning() const;
    /// 返回线程是否处于暂停态。
    bool isPaused() const;

protected:
    /// 子类的单次工作循环入口。
    virtual void onLoop() = 0;
    /// 停止请求到来时的可选回调。
    virtual void onStopRequested() {}

    /// 若线程被暂停则阻塞等待；若已停止则返回 false。
    bool waitIfPausedOrStopped();

private:
    /// 后台线程主循环。
    void threadMain();

    std::thread worker_;
    std::atomic<bool> running_{false};
    std::atomic<bool> paused_{false};
    std::mutex mutex_;
    std::condition_variable cv_;
};

}  // namespace vp::core