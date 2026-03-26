#include "demuxer.h"
#include "logger.h"
#include "media/ffmpeg_channel_layout_compat.h"

#include <algorithm>
#include <string>

namespace vp {

Demuxer::Demuxer() = default;

Demuxer::~Demuxer() {
    close();
}

/// 打开媒体输入并探测流、章节、时长等元数据。
bool Demuxer::open(const std::string& filename) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (format_ctx_) {
        avformat_close_input(&format_ctx_);
        format_ctx_ = nullptr;
        media_info_ = MediaInfo();
        eof_reached_.store(false);
    }

    AVDictionary* options = nullptr;
    av_dict_set(&options, "probesize", "104857600", 0);      // 100 MB
    av_dict_set(&options, "analyzeduration", "10000000", 0); // 10 s
    av_dict_set(&options, "fflags", "+genpts", 0);

    if (avformat_open_input(&format_ctx_, filename.c_str(), nullptr, &options) != 0) {
        av_dict_free(&options);
        LOG_ERROR("Could not open input file: " << filename);
        return false;
    }
    av_dict_free(&options);

    if (avformat_find_stream_info(format_ctx_, nullptr) < 0) {
        LOG_ERROR("Could not find stream information");
        avformat_close_input(&format_ctx_);
        return false;
    }

    detectStreams();
    eof_reached_.store(false);

    LOG_INFO("Demuxer opened file: " << filename
             << ", duration=" << media_info_.duration << "s"
             << ", video_stream=" << media_info_.video_stream_idx
             << ", audio_stream=" << media_info_.audio_stream_idx);
    if (media_info_.video_stream_idx >= 0) {
        LOG_INFO("Video info: " << media_info_.width << "x" << media_info_.height
                 << " @" << media_info_.fps << "fps");
    }
    if (media_info_.audio_stream_idx >= 0) {
        LOG_INFO("Audio info: " << media_info_.sample_rate << "Hz, "
                 << media_info_.channels << "ch");
    }

    return true;
}

/// 关闭当前输入并重置媒体信息；后续读包会立即失败。
void Demuxer::close() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (format_ctx_) {
        avformat_close_input(&format_ctx_);
        format_ctx_ = nullptr;
    }

    media_info_ = MediaInfo();
    eof_reached_.store(false);
}

/// 读取一个压缩包；返回 false 表示失败或已读到 EOF。
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

/// 以秒为单位执行 seek，并在成功后清除 EOF 状态。
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

/// 从 `AVFormatContext` 提取音视频流、章节和基础媒体信息。
void Demuxer::detectStreams() {
    if (!format_ctx_) {
        return;
    }

    media_info_ = MediaInfo();

    media_info_.video_stream_idx = av_find_best_stream(
        format_ctx_, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    media_info_.audio_stream_idx = av_find_best_stream(
        format_ctx_, AVMEDIA_TYPE_AUDIO, -1, media_info_.video_stream_idx, nullptr, 0);

    for (unsigned int i = 0; i < format_ctx_->nb_streams; ++i) {
        AVStream* stream = format_ctx_->streams[i];
        if (!stream || !stream->codecpar) {
            continue;
        }

        if (media_info_.video_stream_idx < 0 && stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            media_info_.video_stream_idx = static_cast<int>(i);
        }
        if (media_info_.audio_stream_idx < 0 && stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            media_info_.audio_stream_idx = static_cast<int>(i);
        }
    }

    if (media_info_.video_stream_idx >= 0 &&
        media_info_.video_stream_idx < static_cast<int>(format_ctx_->nb_streams)) {
        AVStream* stream = format_ctx_->streams[media_info_.video_stream_idx];
        media_info_.width = stream->codecpar->width;
        media_info_.height = stream->codecpar->height;
        media_info_.video_time_base = stream->time_base;
        const AVRational fps_r = stream->avg_frame_rate.num > 0 ? stream->avg_frame_rate : stream->r_frame_rate;
        if (fps_r.num > 0 && fps_r.den > 0) {
            media_info_.fps = av_q2d(fps_r);
        }
    }

    if (media_info_.audio_stream_idx >= 0 &&
        media_info_.audio_stream_idx < static_cast<int>(format_ctx_->nb_streams)) {
        AVStream* stream = format_ctx_->streams[media_info_.audio_stream_idx];
        media_info_.sample_rate = stream->codecpar->sample_rate;
        media_info_.channels = std::max(1, media::ffmpeg_compat::codecParametersChannels(stream->codecpar, 2));
        media_info_.audio_time_base = stream->time_base;
    }

    media_info_.chapters.clear();
    media_info_.chapters.reserve(format_ctx_->nb_chapters);
    for (unsigned int i = 0; i < format_ctx_->nb_chapters; ++i) {
        AVChapter* chapter = format_ctx_->chapters[i];
        if (!chapter || chapter->time_base.num == 0 || chapter->time_base.den == 0) {
            continue;
        }

        ChapterInfo info{};
        info.start = chapter->start * av_q2d(chapter->time_base);
        info.end = chapter->end * av_q2d(chapter->time_base);
        if (AVDictionaryEntry* title = av_dict_get(chapter->metadata, "title", nullptr, 0)) {
            if (title->value) {
                info.title = title->value;
            }
        }

        if (info.end < info.start) {
            std::swap(info.start, info.end);
        }
        media_info_.chapters.push_back(std::move(info));
    }
    std::sort(media_info_.chapters.begin(), media_info_.chapters.end(), [](const ChapterInfo& lhs, const ChapterInfo& rhs) {
        if (lhs.start == rhs.start) {
            return lhs.end < rhs.end;
        }
        return lhs.start < rhs.start;
    });

    media_info_.duration = format_ctx_->duration / (double)AV_TIME_BASE;
}

}
