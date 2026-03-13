#pragma once

#include <string>
#include <vector>

extern "C" {
struct AVIOContext;
}

namespace vp::streaming {

/// 基于 FFmpeg `AVIOContext` 的 HTTP 流读取器。
class HttpStreamDownloader {
public:
    /// 打开远端 URL 并初始化网络读取上下文。
    /// @param url 远端 HTTP/HTTPS 资源地址。
    /// @return 连接建立且 `AVIOContext` 创建成功时返回 true。
    bool open(const std::string& url);
    /// 关闭远端流并清空内部缓冲。
    void close();

    /// 返回流是否已打开。
    bool isOpen() const;
    /// 返回当前打开的 URL。
    const std::string& url() const;
    /// 返回是否已经读取到 EOF。
    bool eof() const;
    /// 返回当前内部缓冲字节数。
    size_t bufferedBytes() const;
    /// 返回累计从远端读取的字节数。
    size_t totalBytesRead() const;
    /// 返回最近一次错误描述。
    const std::string& lastError() const;

    /// 读取最多 `max_bytes` 字节；必要时会先触发预取。
    /// @param max_bytes 期望读取的最大字节数。
    /// @return 本次实际读到的数据块；失败或无数据时可能为空。
    std::vector<uint8_t> readChunk(size_t max_bytes);
    /// 预取数据直到内部缓冲达到目标字节数或 EOF。
    /// @param target_bytes 目标缓冲字节数。
    /// @param chunk_size 单次底层读取块大小。
    /// @return 读取成功或已到 EOF 时返回 true；网络/IO 错误返回 false。
    bool prefetch(size_t target_bytes, size_t chunk_size = 64 * 1024);
    /// 从内部缓冲消费数据，不会继续访问网络。
    /// @param max_bytes 最多提取的字节数。
    /// @return 从内部缓冲区剪出的数据块。
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
