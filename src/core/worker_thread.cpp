#include "core/worker_thread.h"

namespace vp::core {

WorkerThread::~WorkerThread() {
    stop();
}

bool WorkerThread::start(std::function<void()> thread_body) {
    if (!thread_body) {
        return false;
    }

    joinIfFinished();
    if (running_.exchange(true)) {
        return false;
    }

    stop_requested_.store(false);
    worker_ = std::thread([this, thread_body = std::move(thread_body)]() mutable {
        thread_body();
        running_.store(false);
    });
    return true;
}

void WorkerThread::requestStop() {
    stop_requested_.store(true);
}

void WorkerThread::join() {
    if (worker_.joinable() && worker_.get_id() != std::this_thread::get_id()) {
        worker_.join();
    }
}

void WorkerThread::joinIfFinished() {
    if (!running_.load()) {
        join();
    }
}

void WorkerThread::stop() {
    requestStop();
    join();
}

bool WorkerThread::isRunning() const {
    return running_.load();
}

bool WorkerThread::isJoinable() const {
    return worker_.joinable();
}

bool WorkerThread::stopRequested() const {
    return stop_requested_.load();
}

}  // namespace vp::core
