#include "audio_decode_thread.h"
#include "logger.h"

namespace vp {

AudioDecodeThread::AudioDecodeThread()
    : output_queue_(nullptr)
    , running_(false)
    , paused_(false)
    , stop_requested_(false) {
}

AudioDecodeThread::~AudioDecodeThread() {
    stop();
}

bool AudioDecodeThread::start(AVFormatContext* fmt_ctx, int stream_idx, 
                             FrameQueue<AudioFrame>* output_queue) {
    if (running_.load()) {
        return false;
    }

    decoder_ = std::make_unique<AudioDecoder>();
    if (!decoder_->open(fmt_ctx, stream_idx)) {
        LOG_ERROR("Failed to open audio decoder in decode thread");
        return false;
    }

    output_queue_ = output_queue;
    stop_requested_.store(false);
    paused_.store(false);
    running_.store(true);

    thread_ = std::thread(&AudioDecodeThread::decodeLoop, this);
    
    LOG_INFO("Audio decode thread started");
    return true;
}

void AudioDecodeThread::stop() {
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
    
    LOG_INFO("Audio decode thread stopped");
}

void AudioDecodeThread::pause() {
    paused_.store(true);
}

void AudioDecodeThread::resume() {
    paused_.store(false);
    cv_.notify_all();
}

void AudioDecodeThread::flush() {
    if (decoder_) {
        decoder_->flush();
    }
    if (output_queue_) {
        output_queue_->clear();
    }
}

void AudioDecodeThread::decodeLoop() {
    AudioFrame frame;
    
    while (!stop_requested_.load()) {
        if (paused_.load()) {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait_for(lock, std::chrono::milliseconds(10), 
                        [this] { return !paused_.load() || stop_requested_.load(); });
            continue;
        }

        if (decoder_->decodeFrame(frame)) {
            if (frame.isValid()) {
                AVFrame* av_frame = frame.get();
                uint8_t* converted_data = nullptr;
                int converted_linesize = 0;
                
                if (decoder_->convertToS16(av_frame, &converted_data, &converted_linesize)) {
                    int converted_size = av_frame->nb_samples * av_frame->ch_layout.nb_channels * 2;
                    
                    frame.setConvertedData(converted_data, converted_size);
                    
                    if (!output_queue_->pushWithWait(std::move(frame), 50)) {
                        frame = AudioFrame();
                    } else {
                        frame = AudioFrame();
                    }
                }
            }
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}

}
