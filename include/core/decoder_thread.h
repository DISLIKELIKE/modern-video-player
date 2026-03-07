#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

namespace vp::core {

class DecoderThread {
public:
    DecoderThread() = default;
    virtual ~DecoderThread();

    DecoderThread(const DecoderThread&) = delete;
    DecoderThread& operator=(const DecoderThread&) = delete;

    void start();
    void pause();
    void resume();
    void stop();

    bool isRunning() const;
    bool isPaused() const;

protected:
    virtual void onLoop() = 0;
    virtual void onStopRequested() {}

    bool waitIfPausedOrStopped();

private:
    void threadMain();

    std::thread worker_;
    std::atomic<bool> running_{false};
    std::atomic<bool> paused_{false};
    std::mutex mutex_;
    std::condition_variable cv_;
};

}  // namespace vp::core

