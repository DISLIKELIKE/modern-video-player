#include "core/scheduler.h"

#include <algorithm>
#include <chrono>
#include <exception>

#include "logger.h"

namespace vp::core {

Scheduler::Scheduler() = default;

Scheduler::~Scheduler() {
    stop();
}

/// 注入视频解码回调；由视频解码线程循环拉取并填充 `VideoFrame`。
void Scheduler::setVideoDecoder(std::function<bool(VideoFrame&)> decoder) {
    video_decoder_ = std::move(decoder);
}

/// 注入音频解码回调；由音频解码线程循环拉取并填充 `AudioFrame`。
void Scheduler::setAudioDecoder(std::function<bool(AudioFrame&)> decoder) {
    audio_decoder_ = std::move(decoder);
}

/// 注入视频帧队列；调度器把已解码视频帧压入该队列等待渲染。
void Scheduler::setVideoQueue(FrameQueue<VideoFrame>* queue) {
    video_queue_ = queue;
}

/// 注入音频帧队列；调度器把已解码音频帧压入该队列等待消费。
void Scheduler::setAudioQueue(FrameQueue<AudioFrame>* queue) {
    audio_queue_ = queue;
}

/// 注入视频渲染回调；命中渲染节拍后由渲染线程调用。
void Scheduler::setRenderCallback(std::function<void(VideoFrame&&)> callback) {
    render_callback_ = std::move(callback);
}

/// 注入空闲回调；当没有可渲染帧时用于执行 EOF 检查或空闲收尾。
void Scheduler::setIdleCallback(std::function<void()> callback) {
    idle_callback_ = std::move(callback);
}

/// 设置主时钟对象，渲染线程会据此决定等待还是丢帧。
void Scheduler::setClock(Clock* clock) {
    clock_ = clock;
}

/// 启动视频解码、音频解码和渲染线程，并重置统计信息。
void Scheduler::start() {
    if (running_.exchange(true)) {
        paused_.store(false);
        return;
    }

    paused_.store(false);
    video_restart_count_.store(0);
    audio_restart_count_.store(0);
    video_decoded_frames_.store(0);
    audio_decoded_frames_.store(0);
    rendered_frames_.store(0);
    dropped_late_frames_.store(0);
    wait_events_.store(0);

    video_thread_ = std::thread([this] {
        runProtectedLoop([this] { videoDecoderLoop(); }, video_restart_count_);
    });
    audio_thread_ = std::thread([this] {
        runProtectedLoop([this] { audioDecoderLoop(); }, audio_restart_count_);
    });
    render_thread_ = std::thread(&Scheduler::renderLoop, this);
}

/// 标记调度器进入暂停态；解码线程和渲染线程会进入轻量等待。
void Scheduler::pause() {
    paused_.store(true);
}

/// 恢复调度器推进，使解码和渲染线程继续消费队列。
void Scheduler::resume() {
    paused_.store(false);
}

/// 停止全部调度线程并等待退出，避免后台线程继续访问外部对象。
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

/// 清空音视频帧队列，通常用于 seek 后丢弃旧时间线残留帧。
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

SchedulerStats Scheduler::getStats() const {
    SchedulerStats stats;
    stats.video_decoded_frames = video_decoded_frames_.load();
    stats.audio_decoded_frames = audio_decoded_frames_.load();
    stats.rendered_frames = rendered_frames_.load();
    stats.dropped_late_frames = dropped_late_frames_.load();
    stats.wait_events = wait_events_.load();
    return stats;
}

/// 视频解码线程主循环；按背压策略把解码结果送入视频帧队列。
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

        // 高低水位背压：队列超过 80% 暂停解码，回落到 50% 再继续，减少无效堆积。
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
        if (video_queue_->push(std::move(frame), std::chrono::milliseconds(20))) {
            video_decoded_frames_.fetch_add(1);
        }
    }
}

/// 音频解码线程主循环；按背压策略把解码结果送入音频帧队列。
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

        // 音频沿用同样的背压策略，保持 A/V 队列增长趋势一致。
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
        if (audio_queue_->push(std::move(frame), std::chrono::milliseconds(20))) {
            audio_decoded_frames_.fetch_add(1);
        }
    }
}

/// 执行一次时钟对齐后的渲染调度；可能等待、丢帧或提交渲染。
void Scheduler::pumpRenderOnce() {
    if (paused_.load()) {
        if (idle_callback_) {
            idle_callback_();
        }
        return;
    }
    if (!video_queue_ || !render_callback_) {
        if (idle_callback_) {
            idle_callback_();
        }
        return;
    }

    VideoFrame frame;
    if (!video_queue_->pop(frame, std::chrono::milliseconds(0))) {
        if (idle_callback_) {
            idle_callback_();
        }
        return;
    }
    if (!frame.valid) {
        return;
    }

    if (clock_) {
        const double master = clock_->getTime();
        const double diff = frame.pts - master;
        if (diff > 0.0) {
            wait_events_.fetch_add(1);
            // 仅做短等待，避免渲染线程长时间阻塞；剩余等待交给下一轮 pump。
            const double wait_s = std::min(diff, 0.005);
            std::this_thread::sleep_for(std::chrono::duration<double>(wait_s));
        } else if (diff < -0.25) {
            // 帧落后主时钟超过阈值直接丢弃，优先追赶实时播放。
            dropped_late_frames_.fetch_add(1);
            if (idle_callback_) {
                idle_callback_();
            }
            return;
        }
    }

    render_callback_(std::move(frame));
    rendered_frames_.fetch_add(1);
}

/// 渲染线程主循环；依据主时钟决定等待、渲染或丢弃视频帧。
void Scheduler::renderLoop() {
    while (running_.load()) {
        pumpRenderOnce();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

template <typename Func>
/// 在线程异常后执行有限次自动重启，避免单次崩溃直接中断调度。
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
        // 允许一次自动重启，第二次崩溃后停止调度，避免无限重启掩盖问题。
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
