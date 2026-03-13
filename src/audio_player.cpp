#include "audio_player.h"
#include <iostream>
#include <cstring>
#include <algorithm>

namespace vp {

AudioPlayer::AudioPlayer()
    : audio_device_(0)
    , sample_rate_(0)
    , channels_(0)
    , initialized_(false) {
    SDL_zero(audio_spec_);
}

AudioPlayer::~AudioPlayer() {
    close();
}

/// 初始化 SDL 音频设备，并记录实际生效的输出参数。
bool AudioPlayer::init(int sample_rate, int channels) {
    if (initialized_) {
        close();
    }
    
    sample_rate_ = sample_rate;
    channels_ = std::max(1, channels);
    
    SDL_AudioSpec desired_spec;
    SDL_zero(desired_spec);
    desired_spec.freq = sample_rate;
    desired_spec.format = AUDIO_S16SYS;
    desired_spec.channels = static_cast<Uint8>(std::clamp(channels_, 1, 8));
    desired_spec.silence = 0;
    desired_spec.samples = 1024;
    desired_spec.callback = audioCallback;
    desired_spec.userdata = this;
    
    const int allow_changes = SDL_AUDIO_ALLOW_FREQUENCY_CHANGE | SDL_AUDIO_ALLOW_CHANNELS_CHANGE;
    audio_device_ = SDL_OpenAudioDevice(nullptr, 0, &desired_spec, &audio_spec_, allow_changes);
    if (audio_device_ == 0) {
        std::cerr << "Error: Could not open audio device: " << SDL_GetError() << std::endl;
        return false;
    }

    if (audio_spec_.format != AUDIO_S16SYS) {
        std::cerr << "Error: Unsupported SDL output format, expected AUDIO_S16SYS, got "
                  << audio_spec_.format << std::endl;
        SDL_CloseAudioDevice(audio_device_);
        audio_device_ = 0;
        return false;
    }
    
    sample_rate_ = audio_spec_.freq;
    channels_ = audio_spec_.channels;
    initialized_ = true;
    
    std::cout << "Audio player initialized: request " << sample_rate << "Hz/" << channels
              << "ch, actual " << sample_rate_ << "Hz/" << channels_ << "ch" << std::endl;
    
    return true;
}

/// 关闭音频设备并清空播放队列；供重开设备和析构路径复用。
void AudioPlayer::close() {
    stop();
    
    if (audio_device_ != 0) {
        SDL_CloseAudioDevice(audio_device_);
        audio_device_ = 0;
    }
    
    std::lock_guard<std::mutex> lock(queue_mutex_);
    while (!audio_queue_.empty()) {
        audio_queue_.pop();
    }
    
    initialized_ = false;
    SDL_zero(audio_spec_);
    sample_rate_ = 0;
    channels_ = 0;
}

/// 追加一段 PCM 到播放队列；若未提供 PTS，则按队列尾部自动推算。
void AudioPlayer::play(const std::vector<uint8_t>& data, double pts) {
    if (!initialized_ || data.empty()) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        AudioChunk chunk;
        chunk.data = data;
        chunk.offset = 0;
        chunk.pts = pts;

        if (chunk.pts < 0.0) {
            const double bytes_per_second = static_cast<double>(audio_spec_.freq) *
                                            static_cast<double>(audio_spec_.channels) *
                                            static_cast<double>(std::max(1, outputBytesPerSample()));
            if (bytes_per_second > 0.0) {
                chunk.pts = playback_pts_.load() + static_cast<double>(queued_bytes_.load()) / bytes_per_second;
            } else {
                chunk.pts = playback_pts_.load();
            }
        }

        audio_queue_.push(std::move(chunk));
        queued_bytes_.fetch_add(data.size());
    }

    // Avoid lock inversion with SDL audio callback thread (device lock vs queue lock).
    SDL_PauseAudioDevice(audio_device_, 0);
}

/// 暂停 SDL 音频设备回调，但保留当前缓冲数据。
void AudioPlayer::pause() {
    if (initialized_) {
        paused_.store(true);
        SDL_PauseAudioDevice(audio_device_, 1);
    }
}

/// 恢复 SDL 音频设备回调，继续消费已排队的 PCM 数据。
void AudioPlayer::resume() {
    if (initialized_) {
        paused_.store(false);
        SDL_PauseAudioDevice(audio_device_, 0);
    }
}

/// 停止音频播放并丢弃当前缓冲，使下次播放从干净状态开始。
void AudioPlayer::stop() {
    if (initialized_) {
        SDL_PauseAudioDevice(audio_device_, 1);
        
        std::lock_guard<std::mutex> lock(queue_mutex_);
        while (!audio_queue_.empty()) {
            audio_queue_.pop();
        }
        queued_bytes_.store(0);
        playback_pts_.store(0.0);
    }
}

/// 更新音量原子值；真正的混音缩放发生在 SDL 回调线程。
void AudioPlayer::setVolume(float volume) {
    volume_ = std::max(0.0f, std::min(1.0f, volume));
}

/// 更新静音标记；不会清空已经排队的音频块。
void AudioPlayer::setMuted(bool muted) {
    muted_ = muted;
}

double AudioPlayer::getPlaybackPts() const {
    return playback_pts_.load();
}

size_t AudioPlayer::getQueuedBytes() const {
    return queued_bytes_.load();
}

/// 根据当前排队字节数估算音频缓冲时长。
double AudioPlayer::getBufferedSeconds() const {
    const double bytes_per_second = static_cast<double>(audio_spec_.freq) *
                                    static_cast<double>(audio_spec_.channels) *
                                    static_cast<double>(std::max(1, outputBytesPerSample()));
    if (bytes_per_second <= 0.0) {
        return 0.0;
    }
    return static_cast<double>(queued_bytes_.load()) / bytes_per_second;
}

int AudioPlayer::outputBytesPerSample() const {
    if (audio_spec_.format == 0) {
        return 0;
    }
    return static_cast<int>(SDL_AUDIO_BITSIZE(audio_spec_.format) / 8);
}

/// SDL 音频回调线程入口；从队列拉取样本、混音并推进 `playback_pts_`。
void AudioPlayer::audioCallback(void* userdata, uint8_t* stream, int len) {
    AudioPlayer* player = static_cast<AudioPlayer*>(userdata);
    std::lock_guard<std::mutex> lock(player->queue_mutex_);
    
    SDL_memset(stream, 0, len);
    
    while (len > 0 && !player->audio_queue_.empty()) {
        AudioChunk& chunk = player->audio_queue_.front();
        const size_t remaining = chunk.data.size() - chunk.offset;
        int copy_len = std::min(len, static_cast<int>(remaining));
        const uint8_t* src = chunk.data.data() + chunk.offset;
        
        if (player->muted_.load()) {
            SDL_memset(stream, 0, copy_len);
        } else {
            SDL_MixAudioFormat(stream, src, player->audio_spec_.format, copy_len,
                               static_cast<int>(player->volume_.load() * SDL_MIX_MAXVOLUME));
        }

        chunk.offset += static_cast<size_t>(copy_len);
        const size_t dec = static_cast<size_t>(copy_len);
        const size_t prev_queued = player->queued_bytes_.fetch_sub(dec);
        if (prev_queued < dec) {
            player->queued_bytes_.store(0);
        }

        const double bytes_per_second = static_cast<double>(player->audio_spec_.freq) *
                                        static_cast<double>(player->audio_spec_.channels) *
                                        static_cast<double>(std::max(1, player->outputBytesPerSample()));
        if (chunk.pts >= 0.0 && bytes_per_second > 0.0) {
            player->playback_pts_.store(chunk.pts + static_cast<double>(chunk.offset) / bytes_per_second);
        }
        
        stream += copy_len;
        len -= copy_len;
        
        if (chunk.offset >= chunk.data.size()) {
            player->audio_queue_.pop();
        }
    }
}

} // namespace vp
