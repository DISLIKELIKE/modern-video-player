#include "streaming/http_stream_downloader.h"

namespace vp::streaming {

bool HttpStreamDownloader::open(const std::string& url) {
    url_ = url;
    open_ = !url_.empty();
    return open_;
}

void HttpStreamDownloader::close() {
    open_ = false;
    url_.clear();
}

bool HttpStreamDownloader::isOpen() const {
    return open_;
}

const std::string& HttpStreamDownloader::url() const {
    return url_;
}

std::vector<uint8_t> HttpStreamDownloader::readChunk(size_t max_bytes) {
    (void)max_bytes;
    return {};
}

}  // namespace vp::streaming

