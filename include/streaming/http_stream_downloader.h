#pragma once

#include <string>
#include <vector>

namespace vp::streaming {

class HttpStreamDownloader {
public:
    bool open(const std::string& url);
    void close();

    bool isOpen() const;
    const std::string& url() const;

    std::vector<uint8_t> readChunk(size_t max_bytes);

private:
    std::string url_;
    bool open_{false};
};

}  // namespace vp::streaming

