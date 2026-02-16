#include "audio_player.h"
#include <iostream>
#include <cstring>

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

void AudioPlayer::play(const std::vector<uint8_t>& data) {
    if (!initialized_ || data.empty()) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(queue_mutex_);
    audio_queue_.push(data);
    
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
    }
}

void AudioPlayer::setVolume(float volume) {
    volume_ = std::max(0.0f, std::min(1.0f, volume));
}

void AudioPlayer::setMuted(bool muted) {
    muted_ = muted;
}

void AudioPlayer::audioCallback(void* userdata, uint8_t* stream, int len) {
    AudioPlayer* player = static_cast<AudioPlayer*>(userdata);
    std::lock_guard<std::mutex> lock(player->queue_mutex_);
    
    SDL_memset(stream, 0, len);
    
    while (len > 0 && !player->audio_queue_.empty()) {
        const auto& data = player->audio_queue_.front();
        
        int copy_len = std::min(len, static_cast<int>(data.size()));
        
        if (player->muted_) {
            SDL_memset(stream, 0, copy_len);
        } else {
            SDL_MixAudioFormat(stream, data.data(), AUDIO_S16SYS, copy_len, 
                             static_cast<int>(player->volume_ * SDL_MIX_MAXVOLUME));
        }
        
        stream += copy_len;
        len -= copy_len;
        
        if (copy_len == static_cast<int>(data.size())) {
            player->audio_queue_.pop();
        } else {
            std::vector<uint8_t> remaining(data.begin() + copy_len, data.end());
            player->audio_queue_.pop();
            player->audio_queue_.push(remaining);
        }
    }
}

} // namespace vp
