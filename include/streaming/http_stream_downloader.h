#pragma once

#include <string>
#include <vector>

extern "C" {
struct AVIOContext;
}

namespace vp::streaming {

class HttpStreamDownloader {
public:
    bool open(const std::string& url);
    void close();

    bool isOpen() const;
    const std::string& url() const;
    bool eof() const;
    size_t bufferedBytes() const;
    size_t totalBytesRead() const;
    const std::string& lastError() const;

    std::vector<uint8_t> readChunk(size_t max_bytes);
    bool prefetch(size_t target_bytes, size_t chunk_size = 64 * 1024);
    std::vector<uint8_t> consumeBuffered(size_t max_bytes);

private:
    std::string url_;
    bool open_{false};
    bool eof_{false};
    std::string last_error_;
    std::vector<uint8_t> buffer_;
    size_t total_bytes_read_{0};
    AVIOContext* io_ctx_{nullptr};
};

}  // namespace vp::streaming

