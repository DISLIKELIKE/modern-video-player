#include "video_decode_thread.h"
#include "logger.h"

namespace vp {

VideoDecodeThread::VideoDecodeThread()
    : packet_queue_(nullptr)
    , output_queue_(nullptr)
    , running_(false)
    , paused_(false)
    , stop_requested_(false) {
}

VideoDecodeThread::~VideoDecodeThread() {
    stop();
}

bool VideoDecodeThread::start(AVFormatContext* fmt_ctx, int stream_idx,
                              PacketQueue<PacketRef>* packet_queue,
                              FrameQueue<VideoFrame>* output_queue) {
    if (running_.load()) {
        return false;
    }

    decoder_ = std::make_unique<VideoDecoder>();
    if (!decoder_->open(fmt_ctx, stream_idx)) {
        LOG_ERROR("Failed to open video decoder in decode thread");
        return false;
    }

    packet_queue_ = packet_queue;
    output_queue_ = output_queue;
    stop_requested_.store(false);
    paused_.store(false);
    running_.store(true);

    thread_ = std::thread(&VideoDecodeThread::decodeLoop, this);

    LOG_INFO("Video decode thread started");
    return true;
}

void VideoDecodeThread::stop() {
    if (!running_.load()) {
        return;
    }

    stop_requested_.store(true);
    cv_.notify_all();

    if (thread_.joinable()) {
        thread_.join();
    }

    running_.store(false);
    decoder_.reset();

    LOG_INFO("Video decode thread stopped");
}

void VideoDecodeThread::pause() {
    paused_.store(true);
}

void VideoDecodeThread::resume() {
    paused_.store(false);
    cv_.notify_all();
}

void VideoDecodeThread::flush() {
    if (decoder_) {
        decoder_->flush();
    }
    if (output_queue_) {
        output_queue_->clear();
    }
}

void VideoDecodeThread::decodeLoop() {
    VideoFrame frame;
    PacketRef packet_ref;

    while (!stop_requested_.load()) {
        if (paused_.load()) {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait_for(lock, std::chrono::milliseconds(10),
                        [this] { return !paused_.load() || stop_requested_.load(); });
            continue;
        }

        if (!packet_queue_ || !packet_queue_->pop(packet_ref, 50)) {
            continue;
        }

        if (!packet_ref.isValid()) {
            continue;
        }

        AVPacket* packet = packet_ref.get();
        if (decoder_->decodePacket(packet, frame)) {
            if (frame.isValid()) {
                if (!output_queue_->pushWithWait(std::move(frame), 50)) {
                    frame = VideoFrame();
                } else {
                    frame = VideoFrame();
                }
            }
        }
    }
}

}
