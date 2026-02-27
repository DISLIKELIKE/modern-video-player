#include "demuxer.h"
#include "logger.h"
#include <iostream>

namespace vp {

Demuxer::Demuxer() = default;

Demuxer::~Demuxer() {
    close();
}

bool Demuxer::open(const std::string& filename) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (format_ctx_) {
        close();
    }

    if (avformat_open_input(&format_ctx_, filename.c_str(), nullptr, nullptr) != 0) {
        LOG_ERROR("Could not open input file");
        return false;
    }

    if (avformat_find_stream_info(format_ctx_, nullptr) < 0) {
        LOG_ERROR("Could not find stream information");
        avformat_close_input(&format_ctx_);
        return false;
    }

    detectStreams();
    eof_reached_.store(false);

    LOG_INFO("Demuxer opened file");
    LOG_INFO("Duration:");
    LOG_INFO("Video stream index:");
    LOG_INFO("Audio stream index:");

    return true;
}

void Demuxer::close() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (format_ctx_) {
        avformat_close_input(&format_ctx_);
        format_ctx_ = nullptr;
    }

    media_info_ = MediaInfo();
    eof_reached_.store(false);
}

bool Demuxer::readPacket(AVPacket* packet) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!format_ctx_ || !packet) {
        return false;
    }

    int ret = av_read_frame(format_ctx_, packet);
    if (ret < 0) {
        eof_reached_.store(true);
        return false;
    }

    return true;
}

bool Demuxer::seek(double timestamp) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!format_ctx_) {
        return false;
    }

    int64_t seek_target = static_cast<int64_t>(timestamp * AV_TIME_BASE);
    int ret = av_seek_frame(format_ctx_, -1, seek_target, AVSEEK_FLAG_BACKWARD);

    if (ret < 0) {
        LOG_ERROR("Seek failed");
        return false;
    }

    eof_reached_.store(false);
    return true;
}

void Demuxer::detectStreams() {
    if (!format_ctx_) {
        return;
    }

    for (unsigned int i = 0; i < format_ctx_->nb_streams; i++) {
        AVStream* stream = format_ctx_->streams[i];
        AVMediaType type = stream->codecpar->codec_type;

        if (type == AVMEDIA_TYPE_VIDEO && media_info_.video_stream_idx < 0) {
            media_info_.video_stream_idx = static_cast<int>(i);
            media_info_.width = stream->codecpar->width;
            media_info_.height = stream->codecpar->height;
            media_info_.video_time_base = stream->time_base;
        } else if (type == AVMEDIA_TYPE_AUDIO && media_info_.audio_stream_idx < 0) {
            media_info_.audio_stream_idx = static_cast<int>(i);
            media_info_.sample_rate = stream->codecpar->sample_rate;
            media_info_.channels = stream->codecpar->ch_layout.nb_channels;
            media_info_.audio_time_base = stream->time_base;
        }
    }

    media_info_.duration = format_ctx_->duration / (double)AV_TIME_BASE;
}

}
