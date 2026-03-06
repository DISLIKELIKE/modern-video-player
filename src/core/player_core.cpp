#include "core/player_core.h"

#include <algorithm>
#include <chrono>
#include <limits>
#include <vector>

extern "C" {
#include <libswresample/swresample.h>
}

#include "audio_player.h"
#include "display.h"
#include "filters/builtin_filters.h"
#include "logger.h"

namespace vp::core {

namespace {
constexpr int kPacketQueueSize = 256;
}

PlayerCore::PlayerCore() {
    filters::builtin::registerBuiltinFilters();
    scheduler_.setVideoQueue(&video_queue_);
    scheduler_.setAudioQueue(&audio_queue_);
    scheduler_.setClock(&clock_);
    scheduler_.setVideoDecoder([this](VideoFrame& frame) { return decodeVideoFrame(frame); });
    scheduler_.setAudioDecoder([this](AudioFrame& frame) { return decodeAudioFrame(frame); });
    scheduler_.setRenderCallback([this](VideoFrame&& frame) { renderFrame(std::move(frame)); });
    last_position_emit_tp_ = std::chrono::steady_clock::now();
}

PlayerCore::~PlayerCore() {
    close();
}

bool PlayerCore::open(const std::string& filename) {
    close();

    demuxer_ = std::make_unique<Demuxer>();
    if (!demuxer_->open(filename)) {
        emitError(ErrorCode::FileNotFound, "failed to open media file");
        return false;
    }

    const MediaInfo& info = demuxer_->getMediaInfo();
    display_ = std::make_unique<Display>();
    if (info.video_stream_idx >= 0 && !display_->init(info.width, info.height, "Video Player")) {
        emitError(ErrorCode::DisplayInitFailed, "failed to initialize display");
        return false;
    }

    audio_player_ = std::make_unique<AudioPlayer>();
    if (info.audio_stream_idx >= 0 && !audio_player_->init(info.sample_rate, info.channels)) {
        emitError(ErrorCode::AudioInitFailed, "failed to initialize audio");
    }

    if (!initDecoders()) {
        emitError(ErrorCode::DecoderInitFailed, "failed to initialize decoders");
        return false;
    }

    video_packet_queue_ = std::make_unique<PacketQueue>(kPacketQueueSize);
    audio_packet_queue_ = std::make_unique<PacketQueue>(kPacketQueueSize);
    opened_.store(true);
    state_.store(PlaybackState::Stopped);
    position_.store(0.0);
    clock_.reset();
    clock_.setSource(info.audio_stream_idx >= 0 ? ClockSource::Audio : ClockSource::System);
    return true;
}

void PlayerCore::close() {
    stop();
    releaseDecoders();
    if (audio_player_) {
        audio_player_->close();
    }
    if (display_) {
        display_->close();
    }
    audio_player_.reset();
    display_.reset();
    if (demuxer_) {
        demuxer_->close();
    }
    demuxer_.reset();
    video_packet_queue_.reset();
    audio_packet_queue_.reset();
    opened_.store(false);
}

void PlayerCore::play() {
    if (!opened_.load()) {
        return;
    }
    if (state_.load() == PlaybackState::Paused) {
        state_.store(PlaybackState::Playing);
        scheduler_.resume();
        clock_.resume();
        if (audio_player_) {
            audio_player_->resume();
        }
        emitStateChanged(PlaybackState::Playing);
        return;
    }
    if (state_.load() == PlaybackState::Playing) {
        return;
    }

    startDemuxThread();
    startAudioConsumer();
    scheduler_.start();
    state_.store(PlaybackState::Playing);
    emitStateChanged(PlaybackState::Playing);
}

void PlayerCore::pause() {
    if (state_.load() != PlaybackState::Playing) {
        return;
    }
    scheduler_.pause();
    clock_.pause();
    if (audio_player_) {
        audio_player_->pause();
    }
    state_.store(PlaybackState::Paused);
    emitStateChanged(PlaybackState::Paused);
}

void PlayerCore::stop() {
    if (state_.load() == PlaybackState::Stopped && !demux_running_.load()) {
        return;
    }
    stopDemuxThread();
    scheduler_.stop();
    stopAudioConsumer();
    flushPipelines();
    if (audio_player_) {
        audio_player_->stop();
    }
    if (demuxer_ && demuxer_->isOpen()) {
        demuxer_->seek(0.0);
    }
    position_.store(0.0);
    clock_.reset();
    state_.store(PlaybackState::Stopped);
    emitStateChanged(PlaybackState::Stopped);
}

void PlayerCore::seek(double timestamp) {
    if (!opened_.load() || !demuxer_) {
        return;
    }
    const bool was_playing = state_.load() == PlaybackState::Playing;
    if (was_playing) {
        scheduler_.pause();
    }

    flushPipelines();
    if (!demuxer_->seek(timestamp)) {
        emitError(ErrorCode::SeekFailed, "seek failed");
    }
    if (video_codec_ctx_) {
        avcodec_flush_buffers(video_codec_ctx_);
    }
    if (audio_codec_ctx_) {
        avcodec_flush_buffers(audio_codec_ctx_);
    }
    position_.store(timestamp);
    clock_.setTime(timestamp);
    clock_.setAudioClock(timestamp);
    clock_.setVideoClock(timestamp);
    emitPositionChanged(timestamp);

    if (was_playing) {
        scheduler_.resume();
    }
}

PlaybackState PlayerCore::getState() const {
    return state_.load();
}

PlaybackInfo PlayerCore::getInfo() const {
    PlaybackInfo info;
    if (demuxer_) {
        const MediaInfo& mi = demuxer_->getMediaInfo();
        info.duration = mi.duration;
        info.position = position_.load();
        info.video_width = mi.width;
        info.video_height = mi.height;
        info.audio_sample_rate = mi.sample_rate;
        info.audio_channels = mi.channels;
    }
    return info;
}

void PlayerCore::setVolume(float volume) {
    const float clamped = std::max(0.0f, std::min(1.0f, volume));
    volume_.store(clamped);
    if (audio_player_) {
        audio_player_->setVolume(clamped);
    }
}

float PlayerCore::getVolume() const {
    return volume_.load();
}

void PlayerCore::setPlaybackSpeed(double speed) {
    const double clamped = std::max(0.5, std::min(2.0, speed));
    speed_.store(clamped);
    clock_.setSpeed(clamped);
}

double PlayerCore::getPlaybackSpeed() const {
    return speed_.load();
}

void PlayerCore::onStateChanged(StateCallback callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    state_callbacks_.push_back(std::move(callback));
}

void PlayerCore::onPositionChanged(PositionCallback callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    position_callbacks_.push_back(std::move(callback));
}

void PlayerCore::onError(ErrorCallback callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    error_callbacks_.push_back(std::move(callback));
}

void PlayerCore::onFrameRendered(FrameCallback callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    frame_callbacks_.push_back(std::move(callback));
}

bool PlayerCore::initDecoders() {
    if (!demuxer_) {
        return false;
    }
    AVFormatContext* fmt_ctx = demuxer_->getFormatContext();
    const MediaInfo& info = demuxer_->getMediaInfo();

    if (info.video_stream_idx >= 0) {
        AVStream* vs = fmt_ctx->streams[info.video_stream_idx];
        const AVCodec* codec = avcodec_find_decoder(vs->codecpar->codec_id);
        if (!codec) {
            return false;
        }
        video_codec_ctx_ = avcodec_alloc_context3(codec);
        if (!video_codec_ctx_) {
            return false;
        }
        if (avcodec_parameters_to_context(video_codec_ctx_, vs->codecpar) < 0 ||
            avcodec_open2(video_codec_ctx_, codec, nullptr) < 0) {
            return false;
        }
        video_time_base_ = vs->time_base;
    }

    if (info.audio_stream_idx >= 0) {
        AVStream* as = fmt_ctx->streams[info.audio_stream_idx];
        const AVCodec* codec = avcodec_find_decoder(as->codecpar->codec_id);
        if (!codec) {
            return false;
        }
        audio_codec_ctx_ = avcodec_alloc_context3(codec);
        if (!audio_codec_ctx_) {
            return false;
        }
        if (avcodec_parameters_to_context(audio_codec_ctx_, as->codecpar) < 0 ||
            avcodec_open2(audio_codec_ctx_, codec, nullptr) < 0) {
            return false;
        }
        audio_time_base_ = as->time_base;
    }

    return true;
}

void PlayerCore::releaseDecoders() {
    if (video_codec_ctx_) {
        avcodec_free_context(&video_codec_ctx_);
        video_codec_ctx_ = nullptr;
    }
    if (audio_codec_ctx_) {
        avcodec_free_context(&audio_codec_ctx_);
        audio_codec_ctx_ = nullptr;
    }
}

void PlayerCore::startDemuxThread() {
    if (demux_running_.exchange(true)) {
        return;
    }
    demux_thread_ = std::thread([this] {
        while (demux_running_.load() && demuxer_ && !demuxer_->isEof()) {
            if (!video_packet_queue_ && !audio_packet_queue_) {
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
                continue;
            }

            AVPacket* packet = av_packet_alloc();
            if (!packet) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }

            if (!demuxer_->readPacket(packet)) {
                av_packet_free(&packet);
                if (video_packet_queue_) {
                    video_packet_queue_->setEof(true);
                }
                if (audio_packet_queue_) {
                    audio_packet_queue_->setEof(true);
                }
                break;
            }

            const MediaInfo& info = demuxer_->getMediaInfo();
            bool queued = false;
            if (packet->stream_index == info.video_stream_idx && video_packet_queue_) {
                while (demux_running_.load() && !(queued = video_packet_queue_->push(packet, 20))) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
            } else if (packet->stream_index == info.audio_stream_idx && audio_packet_queue_) {
                while (demux_running_.load() && !(queued = audio_packet_queue_->push(packet, 20))) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
            }

            if (!queued) {
                av_packet_free(&packet);
            }
        }
    });
}

void PlayerCore::stopDemuxThread() {
    demux_running_.store(false);
    if (video_packet_queue_) {
        video_packet_queue_->stop();
    }
    if (audio_packet_queue_) {
        audio_packet_queue_->stop();
    }
    if (demux_thread_.joinable()) {
        demux_thread_.join();
    }
    if (video_packet_queue_) {
        video_packet_queue_->start();
    }
    if (audio_packet_queue_) {
        audio_packet_queue_->start();
    }
}

void PlayerCore::startAudioConsumer() {
    if (!audio_player_ || audio_consumer_running_.exchange(true)) {
        return;
    }
    audio_consumer_thread_ = std::thread([this] {
        while (audio_consumer_running_.load()) {
            AudioFrame frame;
            const double played_pts = audio_player_->getPlaybackPts();
            if (played_pts > 0.0) {
                clock_.setAudioClock(played_pts);
                position_.store(played_pts);
                emitPositionChanged(played_pts);
            }

            if (!audio_queue_.pop(frame, std::chrono::milliseconds(5))) {
                continue;
            }
            if (!frame.valid || frame.samples.empty()) {
                continue;
            }

            filter_pipeline_.processAudio(frame.samples.data(), frame.samples.size(), frame.channels);
            audio_player_->play(frame.samples, frame.pts);
        }
    });
}

void PlayerCore::stopAudioConsumer() {
    audio_consumer_running_.store(false);
    if (audio_consumer_thread_.joinable()) {
        audio_consumer_thread_.join();
    }
}

void PlayerCore::flushPipelines() {
    scheduler_.flush();
    if (video_packet_queue_) {
        video_packet_queue_->clear();
    }
    if (audio_packet_queue_) {
        audio_packet_queue_->clear();
    }
}

bool PlayerCore::decodeVideoFrame(VideoFrame& out) {
    if (!video_codec_ctx_ || !video_packet_queue_) {
        return false;
    }

    av_frame_unref(out.frame);
    int ret = avcodec_receive_frame(video_codec_ctx_, out.frame);
    if (ret >= 0) {
        out.valid = true;
        if (out.frame->pts != AV_NOPTS_VALUE) {
            out.pts = out.frame->pts * av_q2d(video_time_base_);
            out.duration = out.frame->duration * av_q2d(video_time_base_);
        }
        return true;
    }

    AVPacket* packet = nullptr;
    if (!video_packet_queue_->pop(packet, 20) || !packet) {
        return false;
    }

    ret = avcodec_send_packet(video_codec_ctx_, packet);
    av_packet_free(&packet);
    if (ret < 0) {
        return false;
    }

    ret = avcodec_receive_frame(video_codec_ctx_, out.frame);
    if (ret < 0) {
        return false;
    }
    out.valid = true;
    if (out.frame->pts != AV_NOPTS_VALUE) {
        out.pts = out.frame->pts * av_q2d(video_time_base_);
        out.duration = out.frame->duration * av_q2d(video_time_base_);
    }
    return true;
}

bool PlayerCore::decodeAudioFrame(AudioFrame& out) {
    if (!audio_codec_ctx_ || !audio_packet_queue_) {
        return false;
    }

    AVFrame* frame = av_frame_alloc();
    if (!frame) {
        return false;
    }

    int ret = avcodec_receive_frame(audio_codec_ctx_, frame);
    if (ret < 0) {
        AVPacket* packet = nullptr;
        if (!audio_packet_queue_->pop(packet, 20) || !packet) {
            av_frame_free(&frame);
            return false;
        }
        ret = avcodec_send_packet(audio_codec_ctx_, packet);
        av_packet_free(&packet);
        if (ret < 0) {
            av_frame_free(&frame);
            return false;
        }
        ret = avcodec_receive_frame(audio_codec_ctx_, frame);
        if (ret < 0) {
            av_frame_free(&frame);
            return false;
        }
    }

    SwrContext* swr_ctx = nullptr;
    AVChannelLayout dst_ch_layout;
    av_channel_layout_default(&dst_ch_layout, frame->ch_layout.nb_channels);

    swr_alloc_set_opts2(&swr_ctx,
                        &dst_ch_layout, AV_SAMPLE_FMT_S16, frame->sample_rate,
                        &frame->ch_layout, static_cast<AVSampleFormat>(frame->format), frame->sample_rate,
                        0, nullptr);
    if (!swr_ctx || swr_init(swr_ctx) < 0) {
        if (swr_ctx) {
            swr_free(&swr_ctx);
        }
        av_channel_layout_uninit(&dst_ch_layout);
        av_frame_free(&frame);
        return false;
    }

    const int channels = static_cast<int>(frame->ch_layout.nb_channels);
    const int64_t dst_nb_samples_i64 = av_rescale_rnd(
        swr_get_delay(swr_ctx, frame->sample_rate) + frame->nb_samples,
        frame->sample_rate, frame->sample_rate, AV_ROUND_UP);
    const int dst_nb_samples = static_cast<int>(std::min<int64_t>(
        dst_nb_samples_i64, std::numeric_limits<int>::max()));

    uint8_t* dst_data = nullptr;
    int dst_linesize = 0;
    av_samples_alloc(&dst_data, &dst_linesize, channels, dst_nb_samples, AV_SAMPLE_FMT_S16, 0);
    const int converted = swr_convert(swr_ctx, &dst_data, dst_nb_samples,
                                      (const uint8_t**)frame->data, frame->nb_samples);
    if (converted <= 0) {
        if (dst_data) {
            av_free(dst_data);
        }
        swr_free(&swr_ctx);
        av_channel_layout_uninit(&dst_ch_layout);
        av_frame_free(&frame);
        return false;
    }
    const int byte_count = converted * channels * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);

    out.samples.assign(dst_data, dst_data + std::max(0, byte_count));
    out.sample_rate = frame->sample_rate;
    out.channels = channels;
    out.valid = !out.samples.empty();
    if (frame->pts != AV_NOPTS_VALUE) {
        out.pts = frame->pts * av_q2d(audio_time_base_);
        out.duration = frame->duration * av_q2d(audio_time_base_);
    }

    av_free(dst_data);
    swr_free(&swr_ctx);
    av_channel_layout_uninit(&dst_ch_layout);
    av_frame_free(&frame);
    return out.valid;
}

void PlayerCore::renderFrame(VideoFrame&& frame) {
    if (!display_ || !frame.valid || !frame.frame) {
        return;
    }

    display_->handleEvents();
    if (display_->shouldQuit()) {
        state_.store(PlaybackState::Stopped);
        emitStateChanged(PlaybackState::Stopped);
        return;
    }

    filter_pipeline_.processVideo(frame);
    display_->renderFrame(reinterpret_cast<const uint8_t*>(frame.frame), frame.frame->width, frame.frame->height);
    display_->present();

    clock_.setVideoClock(frame.pts);
    if (clock_.getSource() != ClockSource::Audio) {
        position_.store(frame.pts);
        emitPositionChanged(frame.pts);
    }
    emitFrameRendered();
}

void PlayerCore::emitStateChanged(PlaybackState state) {
    std::vector<StateCallback> callbacks;
    {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        callbacks = state_callbacks_;
    }
    for (const auto& cb : callbacks) {
        cb(state);
    }
}

void PlayerCore::emitPositionChanged(double position) {
    const auto now = std::chrono::steady_clock::now();
    if (now - last_position_emit_tp_ < std::chrono::milliseconds(100)) {
        return;
    }
    last_position_emit_tp_ = now;

    std::vector<PositionCallback> callbacks;
    {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        callbacks = position_callbacks_;
    }
    for (const auto& cb : callbacks) {
        cb(position);
    }
}

void PlayerCore::emitFrameRendered() {
    std::vector<FrameCallback> callbacks;
    {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        callbacks = frame_callbacks_;
    }
    for (const auto& cb : callbacks) {
        cb();
    }
}

void PlayerCore::emitError(ErrorCode code, const std::string& message) {
    LOG_ERROR("PlayerCore error: " << message);
    std::vector<ErrorCallback> callbacks;
    {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        callbacks = error_callbacks_;
    }
    for (const auto& cb : callbacks) {
        cb(code, message);
    }
}

}  // namespace vp::core
