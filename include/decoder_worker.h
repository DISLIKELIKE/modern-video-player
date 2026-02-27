#pragma once

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#include <memory>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>

namespace vp {

template<typename T>
class ThreadSafeQueue;

class DecoderWorker {
public:
    using PacketQueue = ThreadSafeQueue<AVPacket*>;
    using FrameCallback = std::function<void(AVFrame*)>;

    DecoderWorker();
    ~DecoderWorker();

    bool init(AVCodecParameters* codecpar, AVRational time_base);
    void setOutputCallback(FrameCallback callback);

    void start(PacketQueue* packet_queue);
    void stop();
    void pause();
    void resume();
    void flush();

    bool isRunning() const { return running_.load(); }
    AVCodecContext* getCodecContext() const { return codec_ctx_; }

private:
    void decodeLoop();
    bool decodePacket(AVPacket* packet);

    AVCodecContext* codec_ctx_ = nullptr;
    AVRational time_base_;
    PacketQueue* packet_queue_ = nullptr;
    FrameCallback frame_callback_;

    std::thread thread_;
    std::mutex mutex_;
    std::condition_variable cv_;

    std::atomic<bool> running_{false};
    std::atomic<bool> paused_{false};
    std::atomic<bool> stop_requested_{false};
};

}
