#pragma once

#include "video_decoder.h"
#include "frame_queue.h"
#include "packet_reader.h"
#include <memory>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <mutex>

extern "C" {
#include <libavformat/avformat.h>
}

namespace vp {

class VideoDecodeThread {
public:
    VideoDecodeThread();
    ~VideoDecodeThread();

    bool start(AVFormatContext* fmt_ctx, int stream_idx,
               PacketQueue<PacketRef>* packet_queue,
               FrameQueue<VideoFrame>* output_queue);
    void stop();
    void pause();
    void resume();
    void flush();

    bool isRunning() const { return running_.load(); }
    bool isPaused() const { return paused_.load(); }

private:
    void decodeLoop();

    std::unique_ptr<VideoDecoder> decoder_;
    std::thread thread_;
    PacketQueue<PacketRef>* packet_queue_;
    FrameQueue<VideoFrame>* output_queue_;

    std::atomic<bool> running_;
    std::atomic<bool> paused_;
    std::atomic<bool> stop_requested_;

    std::mutex mutex_;
    std::condition_variable cv_;
};

}
