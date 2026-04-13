#pragma once

#include <atomic>
#include <functional>
#include <thread>

namespace vp::core {

class WorkerThread {
public:
    WorkerThread() = default;
    ~WorkerThread();

    WorkerThread(const WorkerThread&) = delete;
    WorkerThread& operator=(const WorkerThread&) = delete;

    bool start(std::function<void()> thread_body);
    void requestStop();
    void join();
    void joinIfFinished();
    void stop();

    bool isRunning() const;
    bool isJoinable() const;
    bool stopRequested() const;

private:
    std::thread worker_;
    std::atomic<bool> running_{false};
    std::atomic<bool> stop_requested_{false};
};

}  // namespace vp::core
