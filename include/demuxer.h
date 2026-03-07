#pragma once

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

#include <string>
#include <memory>
#include <atomic>
#include <mutex>

namespace vp {

struct MediaInfo {
    int video_stream_idx = -1;
    int audio_stream_idx = -1;
    int width = 0;
    int height = 0;
    double fps = 0.0;
    int sample_rate = 0;
    int channels = 0;
    double duration = 0.0;
    AVRational video_time_base = {0, 1};
    AVRational audio_time_base = {0, 1};
};

class Demuxer {
public:
    Demuxer();
    ~Demuxer();

    bool open(const std::string& filename);
    void close();

    bool readPacket(AVPacket* packet);
    bool seek(double timestamp);

    const MediaInfo& getMediaInfo() const { return media_info_; }
    AVFormatContext* getFormatContext() const { return format_ctx_; }
    bool isEof() const { return eof_reached_; }
    bool isOpen() const { return format_ctx_ != nullptr; }

private:
    void detectStreams();

    AVFormatContext* format_ctx_ = nullptr;
    MediaInfo media_info_;
    std::atomic<bool> eof_reached_{false};
    std::mutex mutex_;
};

}
