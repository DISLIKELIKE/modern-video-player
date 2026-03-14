#include "streaming/http_stream_downloader.h"

#include <algorithm>
#include <limits>

extern "C" {
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavutil/error.h>
}

namespace vp::streaming {

namespace {

/// 将 FFmpeg IO 错误码转换为可读文本，供上层诊断输出。
std::string avErrorToString(int error_code) {
    char buffer[AV_ERROR_MAX_STRING_SIZE] = {};
    if (av_strerror(error_code, buffer, sizeof(buffer)) == 0) {
        return std::string(buffer);
    }
    return "unknown ffmpeg io error";
}

}  // namespace

/// 打开远端 HTTP 流，并初始化 FFmpeg 网络 IO 上下文。
bool HttpStreamDownloader::open(const std::string& url) {
    close();

    avformat_network_init();
    url_ = url;
    if (url_.empty()) {
        last_error_ = "url is empty";
        open_ = false;
        return false;
    }

    AVDictionary* options = nullptr;
    av_dict_set(&options, "user_agent", "modern-video-player/1.0", 0);
    const int open_result = avio_open2(&io_ctx_, url_.c_str(), AVIO_FLAG_READ, nullptr, &options);
    av_dict_free(&options);
    if (open_result < 0) {
        last_error_ = avErrorToString(open_result);
        io_ctx_ = nullptr;
        open_ = false;
        return false;
    }

    eof_ = false;
    buffer_.clear();
    total_bytes_read_ = 0;
    last_error_.clear();
    open_ = true;
    return open_;
}

/// 关闭远端 IO 上下文并重置 URL、缓冲和错误状态。
void HttpStreamDownloader::close() {
    if (io_ctx_) {
        avio_closep(&io_ctx_);
    }
    open_ = false;
    eof_ = false;
    url_.clear();
    buffer_.clear();
    total_bytes_read_ = 0;
    last_error_.clear();
}

bool HttpStreamDownloader::isOpen() const {
    return open_;
}

const std::string& HttpStreamDownloader::url() const {
    return url_;
}

bool HttpStreamDownloader::eof() const {
    return eof_;
}

size_t HttpStreamDownloader::bufferedBytes() const {
    return buffer_.size();
}

size_t HttpStreamDownloader::totalBytesRead() const {
    return total_bytes_read_;
}

const std::string& HttpStreamDownloader::lastError() const {
    return last_error_;
}

/// 读取一段数据；必要时先做预取，再从内部缓冲消费。
std::vector<uint8_t> HttpStreamDownloader::readChunk(size_t max_bytes) {
    if (max_bytes == 0) {
        return {};
    }
    if (!prefetch(max_bytes, max_bytes) && !last_error_.empty()) {
        return {};
    }
    return consumeBuffered(max_bytes);
}

/// 持续从网络读取数据，直到达到目标缓冲量、EOF 或错误。
bool HttpStreamDownloader::prefetch(size_t target_bytes, size_t chunk_size) {
    if (!open_ || !io_ctx_) {
        last_error_ = "stream is not open";
        return false;
    }
    if (target_bytes == 0) {
        return true;
    }

    const size_t effective_chunk = std::max<size_t>(1, chunk_size);
    while (buffer_.size() < target_bytes && !eof_) {
        const size_t request_size = std::min(effective_chunk, static_cast<size_t>(std::numeric_limits<int>::max()));
        std::vector<uint8_t> chunk(request_size);
        const int read_result = avio_read(io_ctx_, chunk.data(), static_cast<int>(request_size));
        if (read_result == AVERROR_EOF || read_result == 0) {
            eof_ = true;
            break;
        }
        if (read_result < 0) {
            last_error_ = avErrorToString(read_result);
            return false;
        }
        chunk.resize(static_cast<size_t>(read_result));
        buffer_.insert(buffer_.end(), chunk.begin(), chunk.end());
        total_bytes_read_ += static_cast<size_t>(read_result);
    }

    return true;
}

/// 仅从内部缓冲提取数据，不会发起新的网络读取。
std::vector<uint8_t> HttpStreamDownloader::consumeBuffered(size_t max_bytes) {
    if (max_bytes == 0 || buffer_.empty()) {
        return {};
    }

    const size_t count = std::min(max_bytes, buffer_.size());
    std::vector<uint8_t> output(buffer_.begin(), buffer_.begin() + static_cast<std::ptrdiff_t>(count));
    buffer_.erase(buffer_.begin(), buffer_.begin() + static_cast<std::ptrdiff_t>(count));
    return output;
}

}  // namespace vp::streaming

