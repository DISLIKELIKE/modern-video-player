#include "audio_player.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>

#include "logger.h"

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <mmdeviceapi.h>
#endif

namespace vp {

namespace {

std::uint64_t elapsedMillisecondsSince(std::chrono::steady_clock::time_point start) {
    const auto elapsed = std::chrono::steady_clock::now() - start;
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());
}

#if defined(_WIN32)
struct AudioOutputProbeResult {
    bool skip_device_open{false};
    std::string detail;
};

std::string getEnvironmentValueCopy(const char* key) {
    char* buffer = nullptr;
    size_t buffer_size = 0;
    const errno_t err = _dupenv_s(&buffer, &buffer_size, key);
    if (err != 0 || buffer == nullptr) {
        return {};
    }
    std::string value(buffer);
    free(buffer);
    return value;
}

std::string toLowerAscii(std::string value) {
    for (char& ch : value) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return value;
}

std::string formatHresult(HRESULT hr) {
    std::ostringstream oss;
    oss << "0x" << std::hex << std::uppercase << static_cast<unsigned long>(hr);
    return oss.str();
}

bool shouldProbeWasapiDefaultEndpoint() {
    const std::string driver = getEnvironmentValueCopy("SDL_AUDIODRIVER");
    if (driver.empty()) {
        return true;
    }
    return toLowerAscii(driver) == "wasapi";
}

bool hasDefaultRenderEndpoint(IMMDeviceEnumerator* enumerator, ERole role) {
    IMMDevice* device = nullptr;
    const HRESULT hr = enumerator->GetDefaultAudioEndpoint(eRender, role, &device);
    if (device != nullptr) {
        device->Release();
    }
    return SUCCEEDED(hr);
}

AudioOutputProbeResult probeWindowsDefaultRenderEndpoint() {
    if (!shouldProbeWasapiDefaultEndpoint()) {
        return {};
    }

    const HRESULT init_hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    const bool should_uninitialize = SUCCEEDED(init_hr);
    if (FAILED(init_hr) && init_hr != RPC_E_CHANGED_MODE) {
        return {false, "default-endpoint probe unavailable: CoInitializeEx hr=" + formatHresult(init_hr)};
    }

    IMMDeviceEnumerator* enumerator = nullptr;
    const HRESULT create_hr = CoCreateInstance(__uuidof(MMDeviceEnumerator),
                                               nullptr,
                                               CLSCTX_INPROC_SERVER,
                                               IID_PPV_ARGS(&enumerator));
    if (FAILED(create_hr)) {
        if (should_uninitialize) {
            CoUninitialize();
        }
        return {false, "default-endpoint probe unavailable: CoCreateInstance hr=" + formatHresult(create_hr)};
    }

    const bool has_default_endpoint = hasDefaultRenderEndpoint(enumerator, eConsole) ||
                                      hasDefaultRenderEndpoint(enumerator, eMultimedia) ||
                                      hasDefaultRenderEndpoint(enumerator, eCommunications);

    UINT active_render_endpoints = 0;
    HRESULT enum_hr = E_FAIL;
    IMMDeviceCollection* collection = nullptr;
    if (!has_default_endpoint) {
        enum_hr = enumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &collection);
        if (SUCCEEDED(enum_hr) && collection != nullptr) {
            collection->GetCount(&active_render_endpoints);
        }
    }

    if (collection != nullptr) {
        collection->Release();
    }
    enumerator->Release();
    if (should_uninitialize) {
        CoUninitialize();
    }

    if (has_default_endpoint || active_render_endpoints > 0) {
        return {};
    }

    std::ostringstream detail;
    detail << "skipped SDL_OpenAudioDevice: no default or active render endpoint for WASAPI";
    if (FAILED(enum_hr)) {
        detail << " (EnumAudioEndpoints hr=" << formatHresult(enum_hr) << ")";
    }
    return {true, detail.str()};
}
#endif

}  // namespace

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

AudioInitReport AudioPlayer::init(int sample_rate, int channels) {
    if (initialized_) {
        close();
    }

    last_init_report_ = {};
    sample_rate_ = sample_rate;
    channels_ = std::max(1, channels);
    const auto init_start = std::chrono::steady_clock::now();

#if defined(_WIN32)
    const AudioOutputProbeResult probe_result = probeWindowsDefaultRenderEndpoint();
    if (probe_result.skip_device_open) {
        last_init_report_.strategy = "skip-no-default-render-endpoint";
        last_init_report_.detail = probe_result.detail;
        last_init_report_.elapsed_ms = elapsedMillisecondsSince(init_start);
        LOG_WARNING("Audio output preflight skipped SDL_OpenAudioDevice"
                    << " strategy=" << last_init_report_.strategy
                    << " elapsed_ms=" << last_init_report_.elapsed_ms
                    << " detail=" << last_init_report_.detail);
        std::cerr << "Warning: " << last_init_report_.detail << std::endl;
        return last_init_report_;
    }
    if (!probe_result.detail.empty()) {
        LOG_INFO("Audio output preflight note: " << probe_result.detail);
    }
#endif

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
    last_init_report_.device_open_attempted = true;
    audio_device_ = SDL_OpenAudioDevice(nullptr, 0, &desired_spec, &audio_spec_, allow_changes);
    if (audio_device_ == 0) {
        last_init_report_.strategy = "sdl-open-failed";
        last_init_report_.detail = SDL_GetError();
        last_init_report_.elapsed_ms = elapsedMillisecondsSince(init_start);
        LOG_WARNING("Audio output init failed"
                    << " strategy=" << last_init_report_.strategy
                    << " elapsed_ms=" << last_init_report_.elapsed_ms
                    << " detail=" << last_init_report_.detail);
        std::cerr << "Error: Could not open audio device: " << last_init_report_.detail << std::endl;
        return last_init_report_;
    }

    if (audio_spec_.format != AUDIO_S16SYS) {
        last_init_report_.strategy = "sdl-open-unsupported-format";
        last_init_report_.detail =
            "unsupported SDL output format=" + std::to_string(static_cast<int>(audio_spec_.format));
        last_init_report_.elapsed_ms = elapsedMillisecondsSince(init_start);
        LOG_WARNING("Audio output init rejected unexpected format"
                    << " strategy=" << last_init_report_.strategy
                    << " elapsed_ms=" << last_init_report_.elapsed_ms
                    << " detail=" << last_init_report_.detail);
        std::cerr << "Error: Unsupported SDL output format, expected AUDIO_S16SYS, got "
                  << audio_spec_.format << std::endl;
        SDL_CloseAudioDevice(audio_device_);
        audio_device_ = 0;
        return last_init_report_;
    }

    sample_rate_ = audio_spec_.freq;
    channels_ = audio_spec_.channels;
    initialized_ = true;
    last_init_report_.initialized = true;
    last_init_report_.strategy = "sdl-open-ok";
    last_init_report_.elapsed_ms = elapsedMillisecondsSince(init_start);
    {
        std::ostringstream detail;
        detail << "actual=" << sample_rate_ << "Hz/" << channels_ << "ch";
        last_init_report_.detail = detail.str();
    }
    LOG_INFO("Audio output initialized"
             << " strategy=" << last_init_report_.strategy
             << " elapsed_ms=" << last_init_report_.elapsed_ms
             << " detail=" << last_init_report_.detail);
    std::cout << "Audio player initialized: request " << sample_rate << "Hz/" << channels
              << "ch, actual " << sample_rate_ << "Hz/" << channels_ << "ch" << std::endl;

    return last_init_report_;
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
            SDL_MixAudioFormat(stream,
                               src,
                               player->audio_spec_.format,
                               copy_len,
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

}  // namespace vp
