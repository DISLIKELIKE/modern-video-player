#include "video_decoder.h"
#include "logger.h"
#include <iostream>

namespace vp {

VideoFrame::VideoFrame()
    : frame_(nullptr)
    , pts_(0.0) {
    frame_ = av_frame_alloc();
}

VideoFrame::~VideoFrame() {
    if (frame_) {
        av_frame_free(&frame_);
    }
}

void VideoFrame::setData(int width, int height, AVPixelFormat pix_fmt) {
    if (frame_) {
        frame_->width = width;
        frame_->height = height;
        frame_->format = pix_fmt;
        av_frame_get_buffer(frame_, 0);
    }
}

VideoDecoder::VideoDecoder()
    : codec_ctx_(nullptr)
    , sws_ctx_(nullptr)
    , format_ctx_(nullptr)
    , stream_idx_(-1)
    , width_(0)
    , height_(0)
    , pix_fmt_(AV_PIX_FMT_NONE) {
}

VideoDecoder::~VideoDecoder() {
    close();
}

bool VideoDecoder::open(AVFormatContext* fmt_ctx, int stream_idx) {
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
        std::cerr << "Error: Unsupported codec" << std::endl;
        return false;
    }
    
    codec_ctx_ = avcodec_alloc_context3(codec);
    if (!codec_ctx_) {
        std::cerr << "Error: Could not allocate codec context" << std::endl;
        return false;
    }
    
    if (avcodec_parameters_to_context(codec_ctx_, codecpar) < 0) {
        std::cerr << "Error: Could not copy codec parameters" << std::endl;
        close();
        return false;
    }
    
    if (avcodec_open2(codec_ctx_, codec, nullptr) < 0) {
        std::cerr << "Error: Could not open codec" << std::endl;
        close();
        return false;
    }
    
    width_ = codec_ctx_->width;
    height_ = codec_ctx_->height;
    pix_fmt_ = codec_ctx_->pix_fmt;
    time_base_ = stream->time_base;
    
    std::cout << "Video decoder opened: " << width_ << "x" << height_ 
              << ", format: " << av_get_pix_fmt_name(pix_fmt_) << std::endl;
    
    return true;
}

void VideoDecoder::close() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (sws_ctx_) {
        sws_freeContext(sws_ctx_);
        sws_ctx_ = nullptr;
    }
    
    if (codec_ctx_) {
        avcodec_free_context(&codec_ctx_);
        codec_ctx_ = nullptr;
    }
    
    format_ctx_ = nullptr;
    width_ = 0;
    height_ = 0;
    pix_fmt_ = AV_PIX_FMT_NONE;
    stream_idx_ = -1;
}

bool VideoDecoder::decodeFrame(VideoFrame& frame) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!codec_ctx_ || !format_ctx_) {
        LOG_TRACE_VIDEO("decodeFrame: codec_ctx_ or format_ctx_ is null");
        return false;
    }
    
    AVPacket* packet = av_packet_alloc();
    if (!packet) {
        LOG_TRACE_VIDEO("decodeFrame: failed to allocate packet");
        return false;
    }
    
    int ret;
    
    while (true) {
        ret = av_read_frame(format_ctx_, packet);
        
        if (ret < 0) {
            LOG_TRACE_VIDEO("decodeFrame: av_read_frame failed, ret={} (EOF or error)", ret);
            av_packet_free(&packet);
            return false;
        }
        
        LOG_TRACE_VIDEO("decodeFrame: read packet, stream_index={}, expected={}", packet->stream_index, stream_idx_);
        
        if (packet->stream_index != stream_idx_) {
            LOG_TRACE_VIDEO("decodeFrame: packet stream mismatch, skipping");
            av_packet_unref(packet);
            continue;
        }
        
        break;
    }
    
    ret = avcodec_send_packet(codec_ctx_, packet);
    av_packet_unref(packet);
    av_packet_free(&packet);
    
    if (ret < 0) {
        LOG_TRACE_VIDEO("decodeFrame: avcodec_send_packet failed, ret={}", ret);
        return false;
    }
    
    ret = avcodec_receive_frame(codec_ctx_, frame.get());
    if (ret < 0) {
        LOG_TRACE_VIDEO("decodeFrame: avcodec_receive_frame failed, ret={}", ret);
        return false;
    }
    
    if (frame.get()->pts != AV_NOPTS_VALUE) {
        frame.setPts(frame.get()->pts * av_q2d(time_base_));
    } else if (frame.get()->pkt_dts != AV_NOPTS_VALUE) {
        frame.setPts(frame.get()->pkt_dts * av_q2d(time_base_));
    }
    
    LOG_TRACE_VIDEO("decodeFrame: success, pts={}", frame.pts());
    return true;
}

void VideoDecoder::flush() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (codec_ctx_) {
        avcodec_flush_buffers(codec_ctx_);
    }
    
    if (sws_ctx_) {
        sws_freeContext(sws_ctx_);
        sws_ctx_ = nullptr;
    }
}

bool VideoDecoder::convertFrame(AVFrame* src_frame, AVFrame* dst_frame, 
                                AVPixelFormat dst_pix_fmt) {
    if (!src_frame || !dst_frame) {
        return false;
    }
    
    if (!sws_ctx_) {
        sws_ctx_ = sws_getContext(src_frame->width, src_frame->height, 
                                  (AVPixelFormat)src_frame->format,
                                  dst_frame->width, dst_frame->height, 
                                  dst_pix_fmt,
                                  SWS_BILINEAR, nullptr, nullptr, nullptr);
        if (!sws_ctx_) {
            std::cerr << "Error: Could not initialize SWS context" << std::endl;
            return false;
        }
    }
    
    sws_scale(sws_ctx_, 
              src_frame->data, src_frame->linesize, 0, src_frame->height,
              dst_frame->data, dst_frame->linesize);
    
    return true;
}

} // namespace vp
