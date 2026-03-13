#pragma once

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

#include <string>
#include <memory>
#include <atomic>
#include <mutex>
#include <vector>

namespace vp {

/// 媒体章节信息快照。
struct ChapterInfo {
    /// 章节起始时间（秒）。
    double start{0.0};
    /// 章节结束时间（秒）。
    double end{0.0};
    /// 章节标题；不存在时可能为空。
    std::string title;
};

/// 解复用探测结果快照；在 `open()` 成功后填充。
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
    std::vector<ChapterInfo> chapters;
};

/// FFmpeg 解复用器封装；负责打开媒体、读包、seek 与流信息探测。
class Demuxer {
public:
    /// 构造未打开的解复用器。
    Demuxer();
    /// 关闭输入并释放 FFmpeg 格式上下文。
    ~Demuxer();

    /// 打开媒体文件并探测音视频流、章节与时基信息。
    /// @param filename 媒体文件路径或 FFmpeg 可访问的输入 URL。
    /// @return 成功创建并探测 `AVFormatContext` 时返回 true。
    bool open(const std::string& filename);
    /// 关闭当前输入并重置探测结果。
    void close();

    /// 读取一个压缩包到 `packet`；到达 EOF 或失败时返回 false。
    /// @param packet 由调用方分配的 `AVPacket*` 输出对象。
    /// @return 成功读到一个压缩包时返回 true。
    bool readPacket(AVPacket* packet);
    /// 按秒 seek 到目标时间；成功后会清除 EOF 状态。
    /// @param timestamp 目标时间点，单位秒。
    /// @return FFmpeg seek 成功时返回 true。
    bool seek(double timestamp);

    /// 返回最近一次探测得到的媒体信息快照。
    const MediaInfo& getMediaInfo() const { return media_info_; }
    /// 返回 FFmpeg `AVFormatContext` 借用指针，不转移所有权。
    AVFormatContext* getFormatContext() const { return format_ctx_; }
    /// 返回最近一次读包是否已触达 EOF。
    bool isEof() const { return eof_reached_; }
    /// 返回是否已成功打开输入。
    bool isOpen() const { return format_ctx_ != nullptr; }

private:
    void detectStreams();

    AVFormatContext* format_ctx_ = nullptr;
    MediaInfo media_info_;
    std::atomic<bool> eof_reached_{false};
    std::mutex mutex_;
};

}
