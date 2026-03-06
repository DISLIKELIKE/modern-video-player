#include "audio_player.h"
#include <iostream>
#include <cstring>
#include <algorithm>

namespace vp {

AudioPlayer::AudioPlayer()
    : audio_device_(0)
    , sample_rate_(0)
    , channels_(0)
    , volume_(1.0f)
    , muted_(false)
    , paused_(false)
    , initialized_(false) {
}

AudioPlayer::~AudioPlayer() {
    close();
}

bool AudioPlayer::init(int sample_rate, int channels) {
    if (initialized_) {
        close();
    }
    
    sample_rate_ = sample_rate;
    channels_ = channels;
    
    SDL_AudioSpec desired_spec;
    SDL_zero(desired_spec);
    desired_spec.freq = sample_rate;
    desired_spec.format = AUDIO_S16SYS;
    desired_spec.channels = channels;
    desired_spec.silence = 0;
    desired_spec.samples = 1024;
    desired_spec.callback = audioCallback;
    desired_spec.userdata = this;
    
    audio_device_ = SDL_OpenAudioDevice(nullptr, 0, &desired_spec, &audio_spec_, 0);
    if (audio_device_ == 0) {
        std::cerr << "Error: Could not open audio device: " << SDL_GetError() << std::endl;
        return false;
    }
    
    initialized_ = true;
    
    std::cout << "Audio player initialized: " << sample_rate << "Hz, " 
              << channels << " channels" << std::endl;
    
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
}

void AudioPlayer::play(const std::vector<uint8_t>& data, double pts) {
    if (!initialized_ || data.empty()) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(queue_mutex_);
    AudioChunk chunk;
    chunk.data = data;
    chunk.offset = 0;
    chunk.pts = pts;

    if (chunk.pts < 0.0) {
        const double bytes_per_second = static_cast<double>(audio_spec_.freq) *
                                        static_cast<double>(audio_spec_.channels) * 2.0;
        if (bytes_per_second > 0.0) {
            chunk.pts = playback_pts_.load() + static_cast<double>(queued_bytes_.load()) / bytes_per_second;
        } else {
            chunk.pts = playback_pts_.load();
        }
    }

    audio_queue_.push(std::move(chunk));
    queued_bytes_.fetch_add(data.size());
    
    SDL_PauseAudioDevice(audio_device_, 0);
}

void AudioPlayer::pause() {
    if (initialized_) {
        paused_ = true;
        SDL_PauseAudioDevice(audio_device_, 1);
    }
}

void AudioPlayer::resume() {
    if (initialized_) {
        paused_ = false;
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

void AudioPlayer::audioCallback(void* userdata, uint8_t* stream, int len) {
    AudioPlayer* player = static_cast<AudioPlayer*>(userdata);
    std::lock_guard<std::mutex> lock(player->queue_mutex_);
    
    SDL_memset(stream, 0, len);
    
    while (len > 0 && !player->audio_queue_.empty()) {
        AudioChunk& chunk = player->audio_queue_.front();
        const size_t remaining = chunk.data.size() - chunk.offset;
        int copy_len = std::min(len, static_cast<int>(remaining));
        const uint8_t* src = chunk.data.data() + chunk.offset;
        
        if (player->muted_) {
            SDL_memset(stream, 0, copy_len);
        } else {
            SDL_MixAudioFormat(stream, src, AUDIO_S16SYS, copy_len, 
                             static_cast<int>(player->volume_ * SDL_MIX_MAXVOLUME));
        }

        chunk.offset += static_cast<size_t>(copy_len);
        size_t prev_queued = player->queued_bytes_.load();
        size_t dec = static_cast<size_t>(copy_len);
        player->queued_bytes_.store(prev_queued > dec ? prev_queued - dec : 0);

        const double bytes_per_second = static_cast<double>(player->audio_spec_.freq) *
                                        static_cast<double>(player->audio_spec_.channels) * 2.0;
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
