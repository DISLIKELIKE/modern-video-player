#include "core/scheduler.h"

#include <chrono>
#include <exception>

#include "logger.h"

namespace vp::core {

Scheduler::Scheduler() = default;

Scheduler::~Scheduler() {
    stop();
}

void Scheduler::setVideoDecoder(std::function<bool(VideoFrame&)> decoder) {
    video_decoder_ = std::move(decoder);
}

void Scheduler::setAudioDecoder(std::function<bool(AudioFrame&)> decoder) {
    audio_decoder_ = std::move(decoder);
}

void Scheduler::setVideoQueue(FrameQueue<VideoFrame>* queue) {
    video_queue_ = queue;
}

void Scheduler::setAudioQueue(FrameQueue<AudioFrame>* queue) {
    audio_queue_ = queue;
}

void Scheduler::setRenderCallback(std::function<void(VideoFrame&&)> callback) {
    render_callback_ = std::move(callback);
}

void Scheduler::setClock(Clock* clock) {
    clock_ = clock;
}

void Scheduler::start() {
    if (running_.exchange(true)) {
        paused_.store(false);
        return;
    }

    paused_.store(false);
    video_restart_count_.store(0);
    audio_restart_count_.store(0);

    video_thread_ = std::thread([this] {
        runProtectedLoop([this] { videoDecoderLoop(); }, video_restart_count_);
    });
    audio_thread_ = std::thread([this] {
        runProtectedLoop([this] { audioDecoderLoop(); }, audio_restart_count_);
    });
    render_thread_ = std::thread(&Scheduler::renderLoop, this);
}

void Scheduler::pause() {
    paused_.store(true);
}

void Scheduler::resume() {
    paused_.store(false);
}

void Scheduler::stop() {
    running_.store(false);
    paused_.store(false);

    if (video_thread_.joinable()) {
        video_thread_.join();
    }
    if (audio_thread_.joinable()) {
        audio_thread_.join();
    }
    if (render_thread_.joinable()) {
        render_thread_.join();
    }
}

void Scheduler::flush() {
    if (video_queue_) {
        video_queue_->flush();
    }
    if (audio_queue_) {
        audio_queue_->flush();
    }
}

size_t Scheduler::getVideoQueueSize() const {
    return video_queue_ ? video_queue_->size() : 0;
}

size_t Scheduler::getAudioQueueSize() const {
    return audio_queue_ ? audio_queue_->size() : 0;
}

void Scheduler::videoDecoderLoop() {
    while (running_.load()) {
        if (paused_.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }
        if (!video_decoder_ || !video_queue_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }

        while (running_.load() && video_queue_->getFillRatio() >= 0.8) {
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            if (video_queue_->getFillRatio() < 0.5) {
                break;
            }
        }

        VideoFrame frame;
        if (!video_decoder_(frame) || !frame.valid) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }
        video_queue_->push(std::move(frame), std::chrono::milliseconds(20));
    }
}

void Scheduler::audioDecoderLoop() {
    while (running_.load()) {
        if (paused_.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }
        if (!audio_decoder_ || !audio_queue_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }

        while (running_.load() && audio_queue_->getFillRatio() >= 0.8) {
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            if (audio_queue_->getFillRatio() < 0.5) {
                break;
            }
        }

        AudioFrame frame;
        if (!audio_decoder_(frame) || !frame.valid) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }
        audio_queue_->push(std::move(frame), std::chrono::milliseconds(20));
    }
}

void Scheduler::renderLoop() {
    while (running_.load()) {
        if (paused_.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }
        if (!video_queue_ || !render_callback_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }

        VideoFrame frame;
        if (!video_queue_->pop(frame, std::chrono::milliseconds(20))) {
            continue;
        }
        if (!frame.valid) {
            continue;
        }

        if (clock_) {
            const double master = clock_->getTime();
            const double diff = frame.pts - master;
            if (diff > 0.0) {
                const double wait_s = std::min(diff, 0.05);
                std::this_thread::sleep_for(std::chrono::duration<double>(wait_s));
            } else if (diff < -0.25) {
                continue;
            }
        }

        render_callback_(std::move(frame));
    }
}

template <typename Func>
void Scheduler::runProtectedLoop(Func&& fn, std::atomic<int>& restart_counter) {
    while (running_.load()) {
        try {
            fn();
            return;
        } catch (const std::exception& ex) {
            LOG_ERROR("Scheduler thread crashed: " << ex.what());
        } catch (...) {
            LOG_ERROR("Scheduler thread crashed with unknown exception");
        }

        const int count = ++restart_counter;
        if (count > 1) {
            LOG_ERROR("Scheduler thread restart limit reached");
            running_.store(false);
            return;
        }
        LOG_WARNING("Restarting scheduler worker thread");
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

}  // namespace vp::core
