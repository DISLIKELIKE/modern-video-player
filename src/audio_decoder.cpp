#include "audio_decoder.h"
#include <iostream>

namespace vp {

AudioFrame::AudioFrame()
    : frame_(nullptr)
    , pts_(0.0)
    , converted_data_(nullptr)
    , converted_size_(0) {
    frame_ = av_frame_alloc();
}

AudioFrame::~AudioFrame() {
    if (frame_) {
        av_frame_free(&frame_);
    }
    if (converted_data_) {
        av_free(converted_data_);
        converted_data_ = nullptr;
    }
}

AudioFrame::AudioFrame(AudioFrame&& other) noexcept
    : frame_(other.frame_)
    , pts_(other.pts_)
    , converted_data_(other.converted_data_)
    , converted_size_(other.converted_size_) {
    other.frame_ = nullptr;
    other.converted_data_ = nullptr;
    other.converted_size_ = 0;
}

AudioFrame& AudioFrame::operator=(AudioFrame&& other) noexcept {
    if (this != &other) {
        if (frame_) {
            av_frame_free(&frame_);
        }
        if (converted_data_) {
            av_free(converted_data_);
        }
        frame_ = other.frame_;
        pts_ = other.pts_;
        converted_data_ = other.converted_data_;
        converted_size_ = other.converted_size_;
        other.frame_ = nullptr;
        other.converted_data_ = nullptr;
        other.converted_size_ = 0;
    }
    return *this;
}

const uint8_t* AudioFrame::getData() const {
    return converted_data_;
}

int AudioFrame::getSize() const {
    return converted_size_;
}

AudioDecoder::AudioDecoder()
    : codec_ctx_(nullptr)
    , swr_ctx_(nullptr)
    , format_ctx_(nullptr)
    , stream_idx_(-1)
    , sample_rate_(0)
    , channels_(0)
    , sample_fmt_(AV_SAMPLE_FMT_NONE) {
}

AudioDecoder::~AudioDecoder() {
    close();
}

bool AudioDecoder::open(AVFormatContext* fmt_ctx, int stream_idx) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!fmt_ctx || stream_idx < 0) {
        return false;
    }
    
    format_ctx_ = fmt_ctx;
    stream_idx_ = stream_idx;
    AVStream* stream = fmt_ctx->streams[stream_idx];
    AVCodecParameters* codecpar = stream->codecpar;
    
    const AVCodec* codec = avcodec_find_decoder(codecpar->codec_id);
    if (!codec) {
        std::cerr << "Error: Unsupported audio codec" << std::endl;
        return false;
    }
    
    codec_ctx_ = avcodec_alloc_context3(codec);
    if (!codec_ctx_) {
        std::cerr << "Error: Could not allocate audio codec context" << std::endl;
        return false;
    }
    
    if (avcodec_parameters_to_context(codec_ctx_, codecpar) < 0) {
        std::cerr << "Error: Could not copy audio codec parameters" << std::endl;
        close();
        return false;
    }
    
    if (avcodec_open2(codec_ctx_, codec, nullptr) < 0) {
        std::cerr << "Error: Could not open audio codec" << std::endl;
        close();
        return false;
    }
    
    sample_rate_ = codec_ctx_->sample_rate;
    channels_ = codec_ctx_->ch_layout.nb_channels;
    sample_fmt_ = codec_ctx_->sample_fmt;
    time_base_ = stream->time_base;
    
    std::cout << "Audio decoder opened: " << sample_rate_ << "Hz, "
              << channels_ << " channels, " 
              << av_get_sample_fmt_name(sample_fmt_) << std::endl;
    
    return true;
}

void AudioDecoder::close() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (swr_ctx_) {
        swr_free(&swr_ctx_);
        swr_ctx_ = nullptr;
    }
    
    if (codec_ctx_) {
        avcodec_free_context(&codec_ctx_);
        codec_ctx_ = nullptr;
    }
    
    format_ctx_ = nullptr;
    sample_rate_ = 0;
    channels_ = 0;
    sample_fmt_ = AV_SAMPLE_FMT_NONE;
    stream_idx_ = -1;
}

bool AudioDecoder::decodePacket(AVPacket* packet, AudioFrame& frame) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!codec_ctx_) {
        return false;
    }

    if (!packet) {
        return false;
    }

    int ret = avcodec_send_packet(codec_ctx_, packet);
    if (ret < 0) {
        return false;
    }

    ret = avcodec_receive_frame(codec_ctx_, frame.get());
    if (ret < 0) {
        return false;
    }

    if (frame.get()->pts != AV_NOPTS_VALUE) {
        frame.setPts(frame.get()->pts * av_q2d(time_base_));
    } else if (frame.get()->pkt_dts != AV_NOPTS_VALUE) {
        frame.setPts(frame.get()->pkt_dts * av_q2d(time_base_));
    }

    return true;
}

bool AudioDecoder::decodeFrame(AudioFrame& frame) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!codec_ctx_ || !format_ctx_) {
        return false;
    }
    
    AVPacket* packet = av_packet_alloc();
    if (!packet) {
        return false;
    }
    
    int ret;
    
    while (true) {
        ret = av_read_frame(format_ctx_, packet);
        
        if (ret < 0) {
            av_packet_free(&packet);
            return false;
        }
        
        if (packet->stream_index != stream_idx_) {
            av_packet_unref(packet);
            continue;
        }
        
        break;
    }
    
    ret = avcodec_send_packet(codec_ctx_, packet);
    av_packet_unref(packet);
    av_packet_free(&packet);
    
    if (ret < 0) {
        return false;
    }
    
    ret = avcodec_receive_frame(codec_ctx_, frame.get());
    if (ret < 0) {
        return false;
    }
    
    if (frame.get()->pts != AV_NOPTS_VALUE) {
        frame.setPts(frame.get()->pts * av_q2d(time_base_));
    } else if (frame.get()->pkt_dts != AV_NOPTS_VALUE) {
        frame.setPts(frame.get()->pkt_dts * av_q2d(time_base_));
    }
    
    return true;
}

void AudioDecoder::flush() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (codec_ctx_) {
        avcodec_flush_buffers(codec_ctx_);
    }
    
    if (swr_ctx_) {
        swr_free(&swr_ctx_);
        swr_ctx_ = nullptr;
    }
}

bool AudioDecoder::convertToS16(AVFrame* src_frame, uint8_t** dst_data, int* dst_linesize) {
    if (!src_frame || !dst_data || !dst_linesize) {
        return false;
    }
    
    int dst_sample_fmt = AV_SAMPLE_FMT_S16;
    int dst_channels = src_frame->ch_layout.nb_channels;
    int dst_sample_rate = src_frame->sample_rate;
    
    if (!swr_ctx_) {
        AVChannelLayout dst_ch_layout;
        av_channel_layout_default(&dst_ch_layout, dst_channels);
        
        swr_alloc_set_opts2(&swr_ctx_,
                           &dst_ch_layout, (AVSampleFormat)dst_sample_fmt, dst_sample_rate,
                           &src_frame->ch_layout, (AVSampleFormat)src_frame->format, src_frame->sample_rate,
                           0, nullptr);
        
        if (!swr_ctx_ || swr_init(swr_ctx_) < 0) {
            std::cerr << "Error: Could not initialize SWR context" << std::endl;
            return false;
        }
    }
    
    int dst_nb_samples = av_rescale_rnd(swr_get_delay(swr_ctx_, src_frame->sample_rate) +
                                        src_frame->nb_samples,
                                        dst_sample_rate, src_frame->sample_rate, AV_ROUND_UP);
    
    av_samples_alloc(dst_data, dst_linesize, dst_channels, dst_nb_samples, 
                     (AVSampleFormat)dst_sample_fmt, 0);
    
    int converted_samples = swr_convert(swr_ctx_, dst_data, dst_nb_samples,
                                       (const uint8_t**)src_frame->data, src_frame->nb_samples);
    
    *dst_linesize = converted_samples * dst_channels * av_get_bytes_per_sample((AVSampleFormat)dst_sample_fmt);
    
    return true;
}

} // namespace vp
