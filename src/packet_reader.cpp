#include "packet_reader.h"
#include "logger.h"

namespace vp {

PacketRef::PacketRef()
    : packet_(nullptr)
    , stream_index_(-1) {
    packet_ = av_packet_alloc();
}

PacketRef::~PacketRef() {
    if (packet_) {
        av_packet_free(&packet_);
    }
}

PacketRef::PacketRef(PacketRef&& other) noexcept
    : packet_(other.packet_)
    , stream_index_(other.stream_index_) {
    other.packet_ = nullptr;
    other.stream_index_ = -1;
}

PacketRef& PacketRef::operator=(PacketRef&& other) noexcept {
    if (this != &other) {
        if (packet_) {
            av_packet_free(&packet_);
        }
        packet_ = other.packet_;
        stream_index_ = other.stream_index_;
        other.packet_ = nullptr;
        other.stream_index_ = -1;
    }
    return *this;
}

void PacketRef::reset() {
    if (packet_) {
        av_packet_unref(packet_);
    }
    stream_index_ = -1;
}

PacketReaderThread::PacketReaderThread()
    : format_ctx_(nullptr)
    , video_stream_idx_(-1)
    , audio_stream_idx_(-1)
    , video_queue_(nullptr)
    , audio_queue_(nullptr)
    , running_(false)
    , paused_(false)
    , stop_requested_(false)
    , eof_reached_(false) {
}

PacketReaderThread::~PacketReaderThread() {
    stop();
}

bool PacketReaderThread::start(AVFormatContext* fmt_ctx, int video_stream_idx, int audio_stream_idx,
                               PacketQueue<PacketRef>* video_queue, PacketQueue<PacketRef>* audio_queue) {
    if (running_.load()) {
        return false;
    }

    if (!fmt_ctx) {
        LOG_ERROR("PacketReaderThread: format_ctx is null");
        return false;
    }

    if (video_stream_idx < 0 && audio_stream_idx < 0) {
        LOG_ERROR("PacketReaderThread: no valid stream index");
        return false;
    }

    format_ctx_ = fmt_ctx;
    video_stream_idx_ = video_stream_idx;
    audio_stream_idx_ = audio_stream_idx;
    video_queue_ = video_queue;
    audio_queue_ = audio_queue;

    av_seek_frame(format_ctx_, -1, 0, AVSEEK_FLAG_BACKWARD);

    stop_requested_.store(false);
    paused_.store(false);
    eof_reached_.store(false);
    running_.store(true);

    thread_ = std::thread(&PacketReaderThread::readLoop, this);

    LOG_INFO("Packet reader thread started");
    return true;
}

void PacketReaderThread::stop() {
    if (!running_.load()) {
        return;
    }

    stop_requested_.store(true);
    cv_.notify_all();

    if (video_queue_) {
        video_queue_->stop();
    }
    if (audio_queue_) {
        audio_queue_->stop();
    }

    if (thread_.joinable()) {
        thread_.join();
    }

    running_.store(false);

    LOG_INFO("Packet reader thread stopped");
}

void PacketReaderThread::pause() {
    paused_.store(true);
}

void PacketReaderThread::resume() {
    paused_.store(false);
    cv_.notify_all();
}

void PacketReaderThread::flush() {
    if (video_queue_) {
        video_queue_->clear();
    }
    if (audio_queue_) {
        audio_queue_->clear();
    }
    eof_reached_.store(false);
}

void PacketReaderThread::readLoop() {
    int packet_count = 0;
    
    while (!stop_requested_.load()) {
        if (paused_.load()) {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait_for(lock, std::chrono::milliseconds(10),
                        [this] { return !paused_.load() || stop_requested_.load(); });
            continue;
        }

        bool video_queue_full = video_queue_ && video_queue_->full();
        bool audio_queue_full = audio_queue_ && audio_queue_->full();

        if (video_queue_full && audio_queue_full) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        AVPacket* packet = av_packet_alloc();
        if (!packet) {
            LOG_ERROR("PacketReaderThread: failed to allocate packet");
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        int ret = av_read_frame(format_ctx_, packet);

        if (ret < 0) {
            LOG_ERROR("PacketReaderThread: av_read_frame failed, ret={}, packet_count={}", ret, packet_count);
            av_packet_free(&packet);
            eof_reached_.store(true);

            if (video_queue_) {
                video_queue_->setEof(true);
            }
            if (audio_queue_) {
                audio_queue_->setEof(true);
            }

            LOG_INFO("PacketReaderThread: EOF reached");
            break;
        }

        packet_count++;
        
        if (packet_count <= 5) {
            LOG_INFO("PacketReaderThread: read packet {}, stream_index={}, size={}", 
                     packet_count, packet->stream_index, packet->size);
        }

        PacketRef packet_ref;
        ret = av_packet_move_ref(packet_ref.get(), packet);
        if (ret < 0) {
            LOG_ERROR("PacketReaderThread: av_packet_move_ref failed, ret={}", ret);
        }
        av_packet_free(&packet);
        packet_ref.setStreamIndex(packet_ref.get()->stream_index);

        if (packet_ref.get()->stream_index == video_stream_idx_) {
            if (video_queue_ && !video_queue_full) {
                if (!video_queue_->pushWithWait(std::move(packet_ref), 10)) {
                    LOG_TRACE_VIDEO("PacketReaderThread: video queue push failed");
                }
            }
        } else if (packet_ref.get()->stream_index == audio_stream_idx_) {
            if (audio_queue_ && !audio_queue_full) {
                if (!audio_queue_->pushWithWait(std::move(packet_ref), 10)) {
                    LOG_TRACE_VIDEO("PacketReaderThread: audio queue push failed");
                }
            }
        }
    }
}

}
