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

void AudioPlayer::pause() {
    if (initialized_) {
        paused_.store(true);
        SDL_PauseAudioDevice(audio_device_, 1);
    }
}

void AudioPlayer::resume() {
    if (initialized_) {
        paused_.store(false);
        SDL_PauseAudioDevice(audio_device_, 0);
    }
}

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

void AudioPlayer::setVolume(float volume) {
    volume_ = std::max(0.0f, std::min(1.0f, volume));
}

void AudioPlayer::setMuted(bool muted) {
    muted_ = muted;
}

double AudioPlayer::getPlaybackPts() const {
    return playback_pts_.load();
}

size_t AudioPlayer::getQueuedBytes() const {
    return queued_bytes_.load();
}

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
