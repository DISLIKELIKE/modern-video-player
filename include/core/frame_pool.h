#pragma once

extern "C" {
#include <libavutil/frame.h>
}

#include <cstddef>
#include <memory>
#include <mutex>
#include <vector>

namespace vp::core {

/// `AVFrame` 对象池；减少高频解码路径中的重复分配与释放。
class FramePool {
public:
    /// 创建固定容量的空闲帧池。
    explicit FramePool(size_t capacity = 32);
    /// 析构时释放池中仍持有的全部 `AVFrame`。
    ~FramePool();

    /// 获取一个可复用的 `AVFrame`；池空时会临时新分配。
    AVFrame* acquire();
    /// 归还一个 `AVFrame`；池已满时直接释放。
    void release(AVFrame* frame);
    /// 清空对象池并释放所有空闲帧。
    void clear();

    /// 返回池容量上限。
    size_t capacity() const;
    /// 返回当前空闲帧数量。
    size_t available() const;

private:
    size_t capacity_;
    mutable std::mutex mutex_;
    std::vector<AVFrame*> idle_frames_;
};

using FramePoolPtr = std::shared_ptr<FramePool>;

}  // namespace vp::core