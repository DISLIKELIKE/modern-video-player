#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <thread>
#include <utility>

#include "core/clock.h"
#include "core/frame.h"
#include "core/frame_queue.h"

namespace vp::core {

struct SchedulerStats {
    // 解码线程成功入队的帧数。
    uint64_t video_decoded_frames{0};
    uint64_t audio_decoded_frames{0};
    // 渲染线程成功提交的帧数。
    uint64_t rendered_frames{0};
    // 因过晚而被丢弃的视频帧数。
    uint64_t dropped_late_frames{0};
    // 因等待主时钟而发生的短等待次数。
    uint64_t wait_events{0};
};

class Scheduler {
public:
    Scheduler();
    ~Scheduler();

    void setVideoDecoder(std::function<bool(VideoFrame&)> decoder);
    void setAudioDecoder(std::function<bool(AudioFrame&)> decoder);
    void setVideoQueue(FrameQueue<VideoFrame>* queue);
    void setAudioQueue(FrameQueue<AudioFrame>* queue);
    void setRenderCallback(std::function<void(VideoFrame&&)> callback);
    void setIdleCallback(std::function<void()> callback);
    void setClock(Clock* clock);

    // 启动解码与渲染线程；重复 start 为幂等。
    void start();
    void pause();
    void resume();
    // 停止线程并等待退出。
    void stop();
    // 清空 A/V 帧队列，不影响解码器上下文。
    void flush();
    // 主动执行一次渲染泵，通常由内部渲染线程循环调用。
    void pumpRenderOnce();

    size_t getVideoQueueSize() const;
    size_t getAudioQueueSize() const;
    SchedulerStats getStats() const;

private:
    void videoDecoderLoop();
    void audioDecoderLoop();
    void renderLoop();

    template <typename Func>
    void runProtectedLoop(Func&& fn, std::atomic<int>& restart_counter);

    std::atomic<bool> running_{false};
    std::atomic<bool> paused_{false};

    std::thread video_thread_;
    std::thread audio_thread_;
    std::thread render_thread_;

    std::function<bool(VideoFrame&)> video_decoder_;
    std::function<bool(AudioFrame&)> audio_decoder_;
    std::function<void(VideoFrame&&)> render_callback_;
    std::function<void()> idle_callback_;

    FrameQueue<VideoFrame>* video_queue_{nullptr};
    FrameQueue<AudioFrame>* audio_queue_{nullptr};

    Clock* clock_{nullptr};
    std::atomic<int> video_restart_count_{0};
    std::atomic<int> audio_restart_count_{0};
    std::atomic<uint64_t> video_decoded_frames_{0};
    std::atomic<uint64_t> audio_decoded_frames_{0};
    std::atomic<uint64_t> rendered_frames_{0};
    std::atomic<uint64_t> dropped_late_frames_{0};
    std::atomic<uint64_t> wait_events_{0};
};

}  // namespace vp::core
