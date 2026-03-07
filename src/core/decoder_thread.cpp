#include "core/decoder_thread.h"

#include <chrono>

namespace vp::core {

DecoderThread::~DecoderThread() {
    stop();
}

void DecoderThread::start() {
    if (running_.exchange(true)) {
        resume();
        return;
    }

    paused_.store(false);
    worker_ = std::thread(&DecoderThread::threadMain, this);
}

void DecoderThread::pause() {
    if (!running_.load()) {
        return;
    }
    paused_.store(true);
}

void DecoderThread::resume() {
    if (!running_.load()) {
        return;
    }
    paused_.store(false);
    cv_.notify_all();
}

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

bool DecoderThread::waitIfPausedOrStopped() {
    if (!paused_.load()) {
        return running_.load();
    }

    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait(lock, [this] { return !paused_.load() || !running_.load(); });
    return running_.load();
}

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

