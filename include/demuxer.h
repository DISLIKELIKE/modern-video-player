#pragma once

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace vp {

struct ChapterInfo {
    double start{0.0};
    double end{0.0};
    std::string title;
};

struct MediaInfo {
    int video_stream_idx{-1};
    int audio_stream_idx{-1};
    int width{0};
    int height{0};
    double fps{0.0};
    int sample_rate{0};
    int channels{0};
    double duration{0.0};
    AVRational video_time_base{0, 1};
    AVRational audio_time_base{0, 1};
    std::vector<ChapterInfo> chapters;
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

    AVFormatContext* format_ctx_{nullptr};
    MediaInfo media_info_{};
    std::atomic<bool> eof_reached_{false};
    std::mutex mutex_;
};

}  // namespace vp
