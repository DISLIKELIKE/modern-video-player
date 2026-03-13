#pragma once

#if __has_include(<SDL2/SDL.h>)
#include <SDL2/SDL.h>
#elif __has_include(<SDL.h>)
#include <SDL.h>
#else
#error "SDL2 headers not found"
#endif
#include <atomic>
#include <string>
#include <vector>
#include <queue>
#include <mutex>
#include <memory>

namespace vp {

/// SDL 音频设备封装；负责缓存 PCM 数据并在回调线程中推进音频时钟。
class AudioPlayer {
public:
    /// 构造未初始化的音频播放器。
    AudioPlayer();
    /// 关闭音频设备并释放播放缓冲。
    ~AudioPlayer();
    
    /// 初始化 SDL 音频设备；输出参数可能被 SDL 调整为实际支持的值。
    /// @param sample_rate 期望输出采样率。
    /// @param channels 期望输出声道数。
    /// @return 设备成功打开且输出格式满足当前实现要求时返回 true。
    bool init(int sample_rate, int channels);
    /// 关闭音频设备并清空待播放队列。
    void close();
    
    /// 追加一段 PCM 数据到播放队列。
    /// @param data 待播放的 PCM 字节流，格式需与当前 SDL 输出格式一致。
    /// @param pts 该音频块起始时间戳；小于 0 时按队列尾部自动推算。
    /// @note 该调用只负责入队，真正消费发生在 SDL 音频回调线程。
    void play(const std::vector<uint8_t>& data, double pts = -1.0);
    /// 暂停音频设备回调。
    void pause();
    /// 恢复音频设备回调。
    void resume();
    /// 停止播放并清空缓冲数据。
    void stop();
    
    /// 设置音量，范围为 [0, 1]。
    void setVolume(float volume);
    /// 返回当前音量。
    float getVolume() const { return volume_.load(); }
    
    /// 设置静音状态。
    void setMuted(bool muted);
    /// 返回是否已静音。
    bool isMuted() const { return muted_.load(); }
    /// 返回音频设备实际播放到的时间戳，通常可作为主时钟。
    double getPlaybackPts() const;
    /// 返回当前仍排队等待播放的字节数。
    size_t getQueuedBytes() const;
    /// 返回当前音频缓冲时长估计值（秒）。
    double getBufferedSeconds() const;
    /// 返回 SDL 实际输出采样率。
    int outputSampleRate() const { return audio_spec_.freq; }
    /// 返回 SDL 实际输出声道数。
    int outputChannels() const { return audio_spec_.channels; }
    /// 返回 SDL 实际输出采样格式。
    SDL_AudioFormat outputFormat() const { return audio_spec_.format; }
    /// 返回单样本字节数。
    int outputBytesPerSample() const;
    /// 返回音频设备是否已初始化。
    bool isInitialized() const { return initialized_; }

private:
    struct AudioChunk {
        std::vector<uint8_t> data;
        size_t offset{0};
        double pts{-1.0};
    };

    /// SDL 音频回调线程入口；从队列拉取数据、混音并更新播放时间戳。
    /// @param userdata `AudioPlayer` 实例指针。
    /// @param stream SDL 提供的输出缓冲区。
    /// @param len 本次要求填充的字节数。
    /// @note 回调线程必须保持低延迟，不能做阻塞或重计算操作。
    static void audioCallback(void* userdata, uint8_t* stream, int len);
    
    SDL_AudioDeviceID audio_device_;
    SDL_AudioSpec audio_spec_;
    
    int sample_rate_;
    int channels_;
    std::atomic<float> volume_{1.0f};
    std::atomic<bool> muted_{false};
    std::atomic<bool> paused_{false};
    
    std::queue<AudioChunk> audio_queue_;
    std::mutex queue_mutex_;
    std::atomic<double> playback_pts_{0.0};
    std::atomic<size_t> queued_bytes_{0};
    
    bool initialized_;
};

} // namespace vp
