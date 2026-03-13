#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "core/frame.h"
#include "filters/audio_filter.h"
#include "filters/video_filter.h"

namespace vp::filters {

/// 综合滤镜管线；分别维护视频滤镜链和音频滤镜链。
class FilterPipeline {
public:
    /// 追加一个视频滤镜。
    void addVideoFilter(std::unique_ptr<IVideoFilter> filter);
    /// 追加一个音频滤镜。
    void addAudioFilter(std::unique_ptr<IAudioFilter> filter);
    /// 按名称移除视频滤镜。
    void removeVideoFilter(const std::string& name);
    /// 按名称移除音频滤镜。
    void removeAudioFilter(const std::string& name);

    /// 顺序执行全部已启用的视频滤镜。
    void processVideo(core::VideoFrame& frame);
    /// 顺序执行全部已启用的音频滤镜。
    void processAudio(uint8_t* samples, size_t sample_count, int channels);

    /// 返回指定名称的视频滤镜裸指针；不存在时返回空。
    IVideoFilter* getVideoFilter(const std::string& name);
    /// 返回指定名称的音频滤镜裸指针；不存在时返回空。
    IAudioFilter* getAudioFilter(const std::string& name);

private:
    std::vector<std::unique_ptr<IVideoFilter>> video_filters_;
    std::vector<std::unique_ptr<IAudioFilter>> audio_filters_;
    mutable std::mutex video_mutex_;
    mutable std::mutex audio_mutex_;
};

}  // namespace vp::filters