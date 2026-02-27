#include "decoder_worker.h"
#include "thread_safe_queue.h"
#include "logger.h"

namespace vp {

DecoderWorker::DecoderWorker() = default;

DecoderWorker::~DecoderWorker() {
    stop();
}

bool DecoderWorker::init(AVCodecParameters* codecpar, AVRational time_base) {
    if (!codecpar) {
        return false;
    }

    const AVCodec* codec = avcodec_find_decoder(codecpar->codec_id);
    if (!codec) {
        LOG_ERROR("DecoderWorker: unsupported codec");
        return false;
    }

    codec_ctx_ = avcodec_alloc_context3(codec);
    if (!codec_ctx_) {
        LOG_ERROR("DecoderWorker: could not allocate codec context");
        return false;
    }

    if (avcodec_parameters_to_context(codec_ctx_, codecpar) < 0) {
        LOG_ERROR("DecoderWorker: could not copy codec parameters");
        avcodec_free_context(&codec_ctx_);
        return false;
    }

    if (avcodec_open2(codec_ctx_, codec, nullptr) < 0) {
        LOG_ERROR("DecoderWorker: could not open codec");
        avcodec_free_context(&codec_ctx_);
        return false;
    }

    time_base_ = time_base;
    return true;
}

void DecoderWorker::setOutputCallback(FrameCallback callback) {
    frame_callback_ = std::move(callback);
}

void DecoderWorker::start(PacketQueue* packet_queue) {
    if (running_.load()) {
        return;
    }

    packet_queue_ = packet_queue;
    stop_requested_.store(false);
    paused_.store(false);
    running_.store(true);

    thread_ = std::thread(&DecoderWorker::decodeLoop, this);
}

void DecoderWorker::stop() {
    if (!running_.load()) {
        return;
    }

    stop_requested_.store(true);
    cv_.notify_all();

    if (thread_.joinable()) {
        thread_.join();
    }

    running_.store(false);

    if (codec_ctx_) {
        avcodec_free_context(&codec_ctx_);
        codec_ctx_ = nullptr;
    }
}

void DecoderWorker::pause() {
    paused_.store(true);
}

void DecoderWorker::resume() {
    paused_.store(false);
    cv_.notify_all();
}

void DecoderWorker::flush() {
    if (codec_ctx_) {
        avcodec_flush_buffers(codec_ctx_);
    }
}

void DecoderWorker::decodeLoop() {
    while (!stop_requested_.load()) {
        if (paused_.load()) {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait_for(lock, std::chrono::milliseconds(10),
                        [this] { return !paused_.load() || stop_requested_.load(); });
            continue;
        }

        if (!packet_queue_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        AVPacket* packet = nullptr;
        if (!packet_queue_->pop(packet, 50)) {
            continue;
        }

        if (!packet) {
            continue;
        }

        decodePacket(packet);
        av_packet_free(&packet);
    }
}

bool DecoderWorker::decodePacket(AVPacket* packet) {
    if (!codec_ctx_ || !packet) {
        return false;
    }

    int ret = avcodec_send_packet(codec_ctx_, packet);
    if (ret < 0) {
        return false;
    }

    AVFrame* frame = av_frame_alloc();
    if (!frame) {
        return false;
    }

    while (true) {
        ret = avcodec_receive_frame(codec_ctx_, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        }
        if (ret < 0) {
            break;
        }

        if (frame->pts != AV_NOPTS_VALUE) {
            frame->pts = frame->pts;
        }

        if (frame_callback_) {
            frame_callback_(frame);
        }

        av_frame_unref(frame);
    }

    av_frame_free(&frame);
    return true;
}

}
