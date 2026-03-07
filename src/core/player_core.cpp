#include "core/player_core.h"

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <limits>
#include <vector>

extern "C" {
#include <libavutil/hwcontext.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}

#include "audio_player.h"
#include "decoder/decoder_factory.h"
#include "filters/builtin_filters.h"
#include "logger.h"
#include "render/renderer_factory.h"

namespace vp::core {

namespace {
constexpr int kPacketQueueSize = 256;

int64_t nowSteadyMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

AVSampleFormat toAvSampleFormat(SDL_AudioFormat fmt) {
    switch (fmt) {
    case AUDIO_U8:
        return AV_SAMPLE_FMT_U8;
    case AUDIO_S16LSB:
    case AUDIO_S16MSB:
        return AV_SAMPLE_FMT_S16;
    case AUDIO_S32LSB:
    case AUDIO_S32MSB:
        return AV_SAMPLE_FMT_S32;
    case AUDIO_F32LSB:
    case AUDIO_F32MSB:
        return AV_SAMPLE_FMT_FLT;
    default:
        return AV_SAMPLE_FMT_NONE;
    }
}

bool channelLayoutEquals(const AVChannelLayout& lhs, const AVChannelLayout& rhs) {
    return av_channel_layout_compare(&lhs, &rhs) == 0;
}

double framePtsSeconds(const AVFrame* frame, AVRational time_base) {
    if (!frame) {
        return 0.0;
    }
    const int64_t pts = frame->best_effort_timestamp != AV_NOPTS_VALUE
                            ? frame->best_effort_timestamp
                            : frame->pts;
    if (pts == AV_NOPTS_VALUE) {
        return 0.0;
    }
    return static_cast<double>(pts) * av_q2d(time_base);
}
}

PlayerCore::PlayerCore() {
    filters::builtin::registerBuiltinFilters();
    scheduler_.setVideoQueue(&video_queue_);
    scheduler_.setAudioQueue(&audio_queue_);
    scheduler_.setClock(&clock_);
    scheduler_.setVideoDecoder([this](VideoFrame& frame) { return decodeVideoFrame(frame); });
    scheduler_.setAudioDecoder([this](AudioFrame& frame) { return decodeAudioFrame(frame); });
    scheduler_.setRenderCallback([this](VideoFrame&& frame) { renderFrame(std::move(frame)); });
    scheduler_.setIdleCallback([this] { onRenderIdle(); });
    last_position_emit_tp_ = std::chrono::steady_clock::now();
    resetDiagnostics();
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
    if (info.video_stream_idx >= 0) {
        video_renderer_ = render::RendererFactory::create(render::VideoRendererType::Auto);
        if (!video_renderer_) {
            emitError(ErrorCode::DisplayInitFailed, "failed to create video renderer");
            return false;
        }
        render::VideoRendererConfig renderer_config{};
        renderer_config.width = info.width;
        renderer_config.height = info.height;
        renderer_config.title = "Video Player";
        if (!video_renderer_->init(renderer_config)) {
            emitError(ErrorCode::DisplayInitFailed, "failed to initialize video renderer");
            return false;
        }
    }

    if (info.audio_stream_idx >= 0) {
        audio_player_ = std::make_unique<AudioPlayer>();
        if (!audio_player_->init(info.sample_rate, info.channels)) {
            emitError(ErrorCode::AudioInitFailed, "failed to initialize audio");
            audio_player_.reset();
        }
    }

    if (!initDecoders()) {
        emitError(ErrorCode::DecoderInitFailed, "failed to initialize decoders");
        return false;
    }

    if (info.video_stream_idx >= 0) {
        video_packet_queue_ = std::make_unique<PacketQueue>(kPacketQueueSize);
    } else {
        video_packet_queue_.reset();
    }
    if (audio_codec_ctx_ && audio_player_ && audio_player_->isInitialized()) {
        audio_packet_queue_ = std::make_unique<PacketQueue>(kPacketQueueSize);
    } else {
        audio_packet_queue_.reset();
    }
    opened_.store(true);
    state_.store(PlaybackState::Stopped);
    position_.store(0.0);
    clock_.reset();
    const bool has_audio_clock = audio_codec_ctx_ && audio_player_ && audio_player_->isInitialized();
    clock_.setSource(has_audio_clock ? ClockSource::Audio : ClockSource::System);
    resetDiagnostics();
    return true;
}

void PlayerCore::close() {
    stop();
    releaseDecoders();
    if (audio_player_) {
        audio_player_->close();
    }
    if (video_renderer_) {
        video_renderer_->close();
    }
    audio_player_.reset();
    video_renderer_.reset();
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
    resetDiagnostics();
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
    releaseAudioResampler();
    position_.store(timestamp);
    clock_.setTime(timestamp);
    clock_.setAudioClock(timestamp);
    clock_.setVideoClock(timestamp);
    emitPositionChanged(timestamp);

    if (was_playing) {
        scheduler_.resume();
    }
}

void PlayerCore::pumpEvents() {
    if (!video_renderer_) {
        return;
    }

    if (video_renderer_->consumeTogglePauseRequest()) {
        if (state_.load() == PlaybackState::Playing) {
            pause();
        } else if (state_.load() == PlaybackState::Paused) {
            play();
        }
    }

    double seek_ratio = 0.0;
    if (video_renderer_->consumeSeekRequest(seek_ratio)) {
        const double duration = demuxer_ ? demuxer_->getMediaInfo().duration : 0.0;
        if (duration > 0.0) {
            seek(duration * std::max(0.0, std::min(1.0, seek_ratio)));
        }
    }

    float volume_request = 0.0f;
    if (video_renderer_->consumeVolumeChangeRequest(volume_request)) {
        setVolume(volume_request);
    }

    if (video_renderer_->shouldQuit()) {
        if (state_.exchange(PlaybackState::Stopped) != PlaybackState::Stopped) {
            emitStateChanged(PlaybackState::Stopped);
        }
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
        if (audio_player_ && audio_player_->isInitialized()) {
            info.audio_sample_rate = audio_player_->outputSampleRate();
            info.audio_channels = audio_player_->outputChannels();
        } else {
            info.audio_sample_rate = mi.sample_rate;
            info.audio_channels = mi.channels;
        }
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

        auto configure_video_codec_ctx = [&](AVCodecContext* ctx) -> bool {
            if (!ctx || avcodec_parameters_to_context(ctx, vs->codecpar) < 0) {
                return false;
            }
            int thread_type = FF_THREAD_FRAME;
            if (codec->capabilities & AV_CODEC_CAP_SLICE_THREADS) {
                thread_type |= FF_THREAD_SLICE;
            }
            if (codec->capabilities & AV_CODEC_CAP_FRAME_THREADS) {
                thread_type |= FF_THREAD_FRAME;
            }
            ctx->thread_count = 0;  // auto
            ctx->thread_type = thread_type;
            ctx->pkt_timebase = vs->time_base;
            ctx->flags2 |= AV_CODEC_FLAG2_FAST;
            return true;
        };

        if (!configure_video_codec_ctx(video_codec_ctx_)) {
            return false;
        }

        video_decoder_backend_ = decoder::DecoderBackend::Software;
        if (!tryConfigureD3D11HardwareDecode(codec, video_codec_ctx_)) {
            video_decoder_backend_ = decoder::DecoderBackend::Software;
            video_hw_pixel_fmt_ = AV_PIX_FMT_NONE;
            LOG_INFO("Video decoder backend: "
                     << decoder::DecoderFactory::backendName(video_decoder_backend_));
        }

        if (avcodec_open2(video_codec_ctx_, codec, nullptr) < 0) {
            if (video_decoder_backend_ == decoder::DecoderBackend::D3D11VA) {
                LOG_WARNING("D3D11VA open failed, retrying software decode backend");

                if (video_codec_ctx_->hw_device_ctx) {
                    av_buffer_unref(&video_codec_ctx_->hw_device_ctx);
                }
                video_codec_ctx_->get_format = nullptr;
                video_codec_ctx_->opaque = nullptr;
                if (video_hw_device_ctx_) {
                    av_buffer_unref(&video_hw_device_ctx_);
                }
                video_hw_pixel_fmt_ = AV_PIX_FMT_NONE;
                video_decoder_backend_ = decoder::DecoderBackend::Software;

                avcodec_free_context(&video_codec_ctx_);
                video_codec_ctx_ = avcodec_alloc_context3(codec);
                if (!video_codec_ctx_ || !configure_video_codec_ctx(video_codec_ctx_) ||
                    avcodec_open2(video_codec_ctx_, codec, nullptr) < 0) {
                    return false;
                }

                LOG_INFO("Video decoder backend: "
                         << decoder::DecoderFactory::backendName(video_decoder_backend_));
            } else {
                return false;
            }
        }
        video_time_base_ = vs->time_base;
    }

    const bool enable_audio = audio_player_ && audio_player_->isInitialized();
    if (info.audio_stream_idx >= 0 && enable_audio) {
        AVStream* as = fmt_ctx->streams[info.audio_stream_idx];
        const AVCodec* codec = avcodec_find_decoder(as->codecpar->codec_id);
        if (!codec) {
            return false;
        }
        audio_codec_ctx_ = avcodec_alloc_context3(codec);
        if (!audio_codec_ctx_) {
            return false;
        }
        if (avcodec_parameters_to_context(audio_codec_ctx_, as->codecpar) < 0) {
            return false;
        }
        audio_codec_ctx_->pkt_timebase = as->time_base;
        audio_codec_ctx_->thread_count = 0;
        audio_codec_ctx_->thread_type = FF_THREAD_FRAME;
        if (avcodec_open2(audio_codec_ctx_, codec, nullptr) < 0) {
            return false;
        }
        audio_time_base_ = as->time_base;
    }

    return true;
}

void PlayerCore::releaseDecoders() {
    releaseVideoScaler();
    releaseAudioResampler();
    if (video_hw_device_ctx_) {
        av_buffer_unref(&video_hw_device_ctx_);
        video_hw_device_ctx_ = nullptr;
    }
    video_hw_pixel_fmt_ = AV_PIX_FMT_NONE;
    video_decoder_backend_ = decoder::DecoderBackend::Software;
    if (video_codec_ctx_) {
        avcodec_free_context(&video_codec_ctx_);
        video_codec_ctx_ = nullptr;
    }
    if (audio_codec_ctx_) {
        avcodec_free_context(&audio_codec_ctx_);
        audio_codec_ctx_ = nullptr;
    }
}

bool PlayerCore::tryConfigureD3D11HardwareDecode(const AVCodec* codec, AVCodecContext* codec_ctx) {
#if defined(_WIN32)
    if (!codec || !codec_ctx) {
        return false;
    }

    const std::string codec_name = codec->name ? codec->name : "";
    const decoder::DecoderBackend preferred_backend =
        decoder::DecoderFactory::selectBestBackend(codec_name, true);
    if (preferred_backend != decoder::DecoderBackend::D3D11VA) {
        return false;
    }

    video_hw_pixel_fmt_ = AV_PIX_FMT_NONE;
    for (int i = 0;; ++i) {
        const AVCodecHWConfig* hw_config = avcodec_get_hw_config(codec, i);
        if (!hw_config) {
            break;
        }
        if ((hw_config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX) &&
            hw_config->device_type == AV_HWDEVICE_TYPE_D3D11VA) {
            video_hw_pixel_fmt_ = hw_config->pix_fmt;
            break;
        }
    }

    if (video_hw_pixel_fmt_ == AV_PIX_FMT_NONE) {
        LOG_WARNING("Codec " << codec_name << " has no D3D11VA HW config, fallback to software decode");
        return false;
    }

    if (av_hwdevice_ctx_create(&video_hw_device_ctx_, AV_HWDEVICE_TYPE_D3D11VA, nullptr, nullptr, 0) < 0 ||
        !video_hw_device_ctx_) {
        LOG_WARNING("Failed to create D3D11VA device context, fallback to software decode");
        return false;
    }

    codec_ctx->get_format = &PlayerCore::selectVideoPixelFormat;
    codec_ctx->opaque = this;
    codec_ctx->hw_device_ctx = av_buffer_ref(video_hw_device_ctx_);
    if (!codec_ctx->hw_device_ctx) {
        LOG_WARNING("Failed to attach D3D11VA device context, fallback to software decode");
        av_buffer_unref(&video_hw_device_ctx_);
        video_hw_pixel_fmt_ = AV_PIX_FMT_NONE;
        return false;
    }

    video_decoder_backend_ = decoder::DecoderBackend::D3D11VA;
    LOG_INFO("Video decoder backend: " << decoder::DecoderFactory::backendName(video_decoder_backend_));
    return true;
#else
    (void)codec;
    (void)codec_ctx;
    return false;
#endif
}

AVPixelFormat PlayerCore::selectVideoPixelFormat(AVCodecContext* ctx, const AVPixelFormat* pix_fmts) {
    if (!pix_fmts) {
        return AV_PIX_FMT_NONE;
    }

    if (ctx && ctx->opaque) {
        auto* self = static_cast<PlayerCore*>(ctx->opaque);
        if (self->video_hw_pixel_fmt_ != AV_PIX_FMT_NONE) {
            for (const AVPixelFormat* p = pix_fmts; *p != AV_PIX_FMT_NONE; ++p) {
                if (*p == self->video_hw_pixel_fmt_) {
                    return *p;
                }
            }
            LOG_WARNING("Requested D3D11VA pixel format not offered by decoder, fallback to software format");
        }
    }

    return pix_fmts[0];
}

bool PlayerCore::prepareVideoOutputFrame(AVFrame* decoded_frame, VideoFrame& out) {
    if (!decoded_frame || !out.frame) {
        return false;
    }

    AVFrame* software_frame = nullptr;
    AVFrame* src_frame = decoded_frame;

    if (video_hw_pixel_fmt_ != AV_PIX_FMT_NONE && decoded_frame->format == video_hw_pixel_fmt_) {
        software_frame = av_frame_alloc();
        if (!software_frame) {
            return false;
        }

        if (av_hwframe_transfer_data(software_frame, decoded_frame, 0) < 0 ||
            av_frame_copy_props(software_frame, decoded_frame) < 0) {
            av_frame_free(&software_frame);
            return false;
        }
        src_frame = software_frame;
    }

    bool ok = true;
    if (src_frame->format != AV_PIX_FMT_YUV420P) {
        AVFrame* conversion_source = src_frame;
        AVFrame* source_ref = nullptr;
        if (conversion_source == out.frame) {
            source_ref = av_frame_alloc();
            if (!source_ref || av_frame_ref(source_ref, out.frame) < 0) {
                if (source_ref) {
                    av_frame_free(&source_ref);
                }
                if (software_frame) {
                    av_frame_free(&software_frame);
                }
                return false;
            }
            conversion_source = source_ref;
        }

        ok = convertVideoFrameToYuv420(conversion_source, out.frame);
        if (source_ref) {
            av_frame_free(&source_ref);
        }
    } else if (src_frame != out.frame) {
        av_frame_unref(out.frame);
        ok = av_frame_ref(out.frame, src_frame) >= 0;
    }

    if (software_frame) {
        av_frame_free(&software_frame);
    }
    return ok;
}

bool PlayerCore::ensureVideoScaler(const AVFrame* src_frame) {
    if (!src_frame || src_frame->width <= 0 || src_frame->height <= 0 || src_frame->format == AV_PIX_FMT_NONE) {
        return false;
    }

    const AVPixelFormat src_fmt = static_cast<AVPixelFormat>(src_frame->format);
    if (video_sws_ctx_ &&
        video_sws_src_width_ == src_frame->width &&
        video_sws_src_height_ == src_frame->height &&
        video_sws_src_fmt_ == src_fmt) {
        return true;
    }

    releaseVideoScaler();
    video_sws_ctx_ = sws_getContext(src_frame->width, src_frame->height, src_fmt,
                                    src_frame->width, src_frame->height, AV_PIX_FMT_YUV420P,
                                    SWS_BILINEAR, nullptr, nullptr, nullptr);
    if (!video_sws_ctx_) {
        return false;
    }

    video_sws_src_width_ = src_frame->width;
    video_sws_src_height_ = src_frame->height;
    video_sws_src_fmt_ = src_fmt;
    return true;
}

bool PlayerCore::convertVideoFrameToYuv420(const AVFrame* src_frame, AVFrame* dst_frame) {
    if (!src_frame || !dst_frame || !ensureVideoScaler(src_frame)) {
        return false;
    }

    av_frame_unref(dst_frame);
    dst_frame->format = AV_PIX_FMT_YUV420P;
    dst_frame->width = src_frame->width;
    dst_frame->height = src_frame->height;

    if (av_frame_get_buffer(dst_frame, 32) < 0 ||
        av_frame_make_writable(dst_frame) < 0 ||
        av_frame_copy_props(dst_frame, src_frame) < 0) {
        av_frame_unref(dst_frame);
        return false;
    }

    const int scaled = sws_scale(video_sws_ctx_,
                                 src_frame->data,
                                 src_frame->linesize,
                                 0,
                                 src_frame->height,
                                 dst_frame->data,
                                 dst_frame->linesize);
    if (scaled <= 0) {
        av_frame_unref(dst_frame);
        return false;
    }

    return true;
}

void PlayerCore::releaseVideoScaler() {
    if (video_sws_ctx_) {
        sws_freeContext(video_sws_ctx_);
        video_sws_ctx_ = nullptr;
    }
    video_sws_src_width_ = 0;
    video_sws_src_height_ = 0;
    video_sws_src_fmt_ = AV_PIX_FMT_NONE;
}

bool PlayerCore::ensureAudioResampler(const AVFrame* frame) {
    if (!frame || !audio_player_ || !audio_player_->isInitialized()) {
        return false;
    }

    const AVSampleFormat in_sample_fmt = static_cast<AVSampleFormat>(frame->format);
    AVSampleFormat out_sample_fmt = toAvSampleFormat(audio_player_->outputFormat());
    if (out_sample_fmt == AV_SAMPLE_FMT_NONE) {
        out_sample_fmt = AV_SAMPLE_FMT_S16;
    }

    const int in_sample_rate = std::max(1, frame->sample_rate);
    const int out_sample_rate = std::max(1, audio_player_->outputSampleRate());
    const int out_channels = std::max(1, audio_player_->outputChannels());

    AVChannelLayout desired_in_layout{};
    if (frame->ch_layout.nb_channels > 0) {
        if (av_channel_layout_copy(&desired_in_layout, &frame->ch_layout) < 0) {
            return false;
        }
    } else {
        const int fallback_channels = (audio_codec_ctx_ && audio_codec_ctx_->ch_layout.nb_channels > 0)
                                          ? audio_codec_ctx_->ch_layout.nb_channels
                                          : 2;
        av_channel_layout_default(&desired_in_layout, std::max(1, fallback_channels));
    }

    AVChannelLayout desired_out_layout{};
    av_channel_layout_default(&desired_out_layout, out_channels);

    bool need_reinit = (audio_swr_ctx_ == nullptr) ||
                       (swr_in_sample_fmt_ != in_sample_fmt) ||
                       (swr_out_sample_fmt_ != out_sample_fmt) ||
                       (swr_in_sample_rate_ != in_sample_rate) ||
                       (swr_out_sample_rate_ != out_sample_rate) ||
                       !channelLayoutEquals(swr_in_layout_, desired_in_layout) ||
                       !channelLayoutEquals(swr_out_layout_, desired_out_layout);

    if (!need_reinit) {
        av_channel_layout_uninit(&desired_in_layout);
        av_channel_layout_uninit(&desired_out_layout);
        return true;
    }

    releaseAudioResampler();

    if (av_channel_layout_copy(&swr_in_layout_, &desired_in_layout) < 0 ||
        av_channel_layout_copy(&swr_out_layout_, &desired_out_layout) < 0) {
        av_channel_layout_uninit(&desired_in_layout);
        av_channel_layout_uninit(&desired_out_layout);
        releaseAudioResampler();
        return false;
    }
    av_channel_layout_uninit(&desired_in_layout);
    av_channel_layout_uninit(&desired_out_layout);

    swr_in_sample_fmt_ = in_sample_fmt;
    swr_out_sample_fmt_ = out_sample_fmt;
    swr_in_sample_rate_ = in_sample_rate;
    swr_out_sample_rate_ = out_sample_rate;

    if (swr_alloc_set_opts2(&audio_swr_ctx_,
                            &swr_out_layout_, swr_out_sample_fmt_, swr_out_sample_rate_,
                            &swr_in_layout_, swr_in_sample_fmt_, swr_in_sample_rate_,
                            0, nullptr) < 0 ||
        !audio_swr_ctx_ ||
        swr_init(audio_swr_ctx_) < 0) {
        releaseAudioResampler();
        return false;
    }

    return true;
}

void PlayerCore::releaseAudioResampler() {
    if (audio_swr_ctx_) {
        swr_free(&audio_swr_ctx_);
        audio_swr_ctx_ = nullptr;
    }
    av_channel_layout_uninit(&swr_in_layout_);
    av_channel_layout_uninit(&swr_out_layout_);
    swr_in_sample_fmt_ = AV_SAMPLE_FMT_NONE;
    swr_out_sample_fmt_ = AV_SAMPLE_FMT_NONE;
    swr_in_sample_rate_ = 0;
    swr_out_sample_rate_ = 0;
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
                    demux_push_retries_.fetch_add(1);
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
                if (queued) {
                    demux_video_packets_.fetch_add(1);
                }
            } else if (packet->stream_index == info.audio_stream_idx && audio_packet_queue_) {
                while (demux_running_.load() && !(queued = audio_packet_queue_->push(packet, 20))) {
                    demux_push_retries_.fetch_add(1);
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
                if (queued) {
                    demux_audio_packets_.fetch_add(1);
                }
            }

            if (!queued) {
                demux_dropped_packets_.fetch_add(1);
                av_packet_free(&packet);
            }
            maybeLogDiagnostics("demux");
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
    if (!audio_player_ || !audio_player_->isInitialized() || audio_consumer_running_.exchange(true)) {
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
                maybeLogDiagnostics("audio-idle");
                continue;
            }
            if (!frame.valid || frame.samples.empty()) {
                maybeLogDiagnostics("audio-invalid");
                continue;
            }

            if (audio_player_->outputBytesPerSample() == 2) {
                const size_t sample_count = frame.samples.size() / 2;
                filter_pipeline_.processAudio(frame.samples.data(), sample_count, frame.channels);
            }
            audio_player_->play(frame.samples, frame.pts);
            audio_submitted_frames_.fetch_add(1);
            maybeLogDiagnostics("audio-play");
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

void PlayerCore::resetDiagnostics() {
    demux_video_packets_.store(0);
    demux_audio_packets_.store(0);
    demux_push_retries_.store(0);
    demux_dropped_packets_.store(0);
    decode_video_ok_.store(0);
    decode_audio_ok_.store(0);
    audio_submitted_frames_.store(0);
    render_frames_.store(0);
    last_diag_log_ms_.store(nowSteadyMs());
}

void PlayerCore::maybeLogDiagnostics(const char* source_tag) {
    const int64_t now_ms = nowSteadyMs();
    int64_t last_ms = last_diag_log_ms_.load();
    while (now_ms - last_ms >= 1000) {
        if (last_diag_log_ms_.compare_exchange_weak(last_ms, now_ms)) {
            const SchedulerStats scheduler_stats = scheduler_.getStats();
            const size_t video_pkt_q = video_packet_queue_ ? video_packet_queue_->size() : 0;
            const size_t audio_pkt_q = audio_packet_queue_ ? audio_packet_queue_->size() : 0;
            LOG_INFO("[diag:" << source_tag << "]"
                     << " demux(v=" << demux_video_packets_.load()
                     << ",a=" << demux_audio_packets_.load()
                     << ",retry=" << demux_push_retries_.load()
                     << ",drop=" << demux_dropped_packets_.load() << ")"
                     << " pkt_q(v=" << video_pkt_q << ",a=" << audio_pkt_q << ")"
                     << " dec(core v=" << decode_video_ok_.load()
                     << ",a=" << decode_audio_ok_.load() << ")"
                     << " dec(sched v=" << scheduler_stats.video_decoded_frames
                     << ",a=" << scheduler_stats.audio_decoded_frames << ")"
                     << " frame_q(v=" << video_queue_.size() << ",a=" << audio_queue_.size() << ")"
                     << " render(out=" << scheduler_stats.rendered_frames
                     << ",late_drop=" << scheduler_stats.dropped_late_frames
                     << ",wait=" << scheduler_stats.wait_events
                     << ",cb=" << render_frames_.load() << ")"
                     << " audio(submit=" << audio_submitted_frames_.load()
                     << ",play_pts=" << (audio_player_ ? audio_player_->getPlaybackPts() : 0.0) << ")"
                     << " clock(a=" << clock_.getAudioClock()
                     << ",v=" << clock_.getVideoClock()
                     << ",m=" << clock_.getTime() << ")");
            return;
        }
    }
}

bool PlayerCore::decodeVideoFrame(VideoFrame& out) {
    if (!video_codec_ctx_ || !video_packet_queue_) {
        return false;
    }

    av_frame_unref(out.frame);
    int ret = avcodec_receive_frame(video_codec_ctx_, out.frame);
    if (ret >= 0) {
        if (!prepareVideoOutputFrame(out.frame, out)) {
            return false;
        }
        out.valid = true;
        out.pts = framePtsSeconds(out.frame, video_time_base_);
        if (out.frame->duration > 0) {
            out.duration = out.frame->duration * av_q2d(video_time_base_);
        } else if (video_codec_ctx_->framerate.num > 0 && video_codec_ctx_->framerate.den > 0) {
            out.duration = 1.0 / av_q2d(video_codec_ctx_->framerate);
        } else {
            out.duration = 0.0;
        }
        decode_video_ok_.fetch_add(1);
        maybeLogDiagnostics("vdec-recv");
        return true;
    }
    if (ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
        return false;
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
    if (!prepareVideoOutputFrame(out.frame, out)) {
        return false;
    }
    out.valid = true;
    out.pts = framePtsSeconds(out.frame, video_time_base_);
    if (out.frame->duration > 0) {
        out.duration = out.frame->duration * av_q2d(video_time_base_);
    } else if (video_codec_ctx_->framerate.num > 0 && video_codec_ctx_->framerate.den > 0) {
        out.duration = 1.0 / av_q2d(video_codec_ctx_->framerate);
    } else {
        out.duration = 0.0;
    }
    decode_video_ok_.fetch_add(1);
    maybeLogDiagnostics("vdec-send");
    return true;
}

bool PlayerCore::decodeAudioFrame(AudioFrame& out) {
    if (!audio_codec_ctx_ || !audio_packet_queue_ || !audio_player_ || !audio_player_->isInitialized()) {
        return false;
    }

    AVFrame* frame = av_frame_alloc();
    if (!frame) {
        return false;
    }

    int ret = avcodec_receive_frame(audio_codec_ctx_, frame);
    if (ret == AVERROR(EAGAIN)) {
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
    } else if (ret < 0) {
        av_frame_free(&frame);
        return false;
    }

    if (!ensureAudioResampler(frame)) {
        av_frame_free(&frame);
        return false;
    }

    const int channels = std::max(1, swr_out_layout_.nb_channels);
    const int64_t dst_nb_samples_i64 = av_rescale_rnd(
        swr_get_delay(audio_swr_ctx_, frame->sample_rate) + frame->nb_samples,
        swr_out_sample_rate_, frame->sample_rate, AV_ROUND_UP);
    const int dst_nb_samples = static_cast<int>(std::min<int64_t>(
        dst_nb_samples_i64, std::numeric_limits<int>::max()));

    uint8_t* dst_data = nullptr;
    int dst_linesize = 0;
    if (av_samples_alloc(&dst_data, &dst_linesize, channels, dst_nb_samples, swr_out_sample_fmt_, 0) < 0) {
        av_frame_free(&frame);
        return false;
    }

    const int converted = swr_convert(audio_swr_ctx_, &dst_data, dst_nb_samples,
                                      (const uint8_t**)frame->extended_data, frame->nb_samples);
    if (converted <= 0) {
        av_freep(&dst_data);
        av_frame_free(&frame);
        return false;
    }

    const int byte_count = av_samples_get_buffer_size(
        nullptr, channels, converted, swr_out_sample_fmt_, 1);
    if (byte_count <= 0) {
        av_freep(&dst_data);
        av_frame_free(&frame);
        return false;
    }

    out.samples.assign(dst_data, dst_data + std::max(0, byte_count));
    out.sample_rate = swr_out_sample_rate_;
    out.channels = channels;
    out.valid = !out.samples.empty();
    if (frame->pts != AV_NOPTS_VALUE) {
        out.pts = frame->pts * av_q2d(audio_time_base_);
        out.duration = frame->duration * av_q2d(audio_time_base_);
    } else {
        out.pts = 0.0;
        out.duration = 0.0;
    }
    if (out.duration <= 0.0 && out.sample_rate > 0) {
        const int bytes_per_sample = av_get_bytes_per_sample(swr_out_sample_fmt_);
        if (bytes_per_sample > 0 && out.channels > 0) {
            out.duration = static_cast<double>(out.samples.size()) /
                           static_cast<double>(out.sample_rate * out.channels * bytes_per_sample);
        }
    }
    if (out.valid) {
        decode_audio_ok_.fetch_add(1);
        maybeLogDiagnostics("adec");
    }

    av_freep(&dst_data);
    av_frame_free(&frame);
    return out.valid;
}

void PlayerCore::renderFrame(VideoFrame&& frame) {
    if (!video_renderer_ || !frame.valid || !frame.frame) {
        return;
    }

    const double duration = demuxer_ ? demuxer_->getMediaInfo().duration : 0.0;
    video_renderer_->setOverlayState(position_.load(), duration, volume_.load(), state_.load() == PlaybackState::Paused);

    filter_pipeline_.processVideo(frame);
    video_renderer_->renderFrame(frame);
    video_renderer_->present();

    clock_.setVideoClock(frame.pts);
    render_frames_.fetch_add(1);
    if (clock_.getSource() != ClockSource::Audio) {
        position_.store(frame.pts);
        emitPositionChanged(frame.pts);
    }
    maybeLogDiagnostics("render");
    emitFrameRendered();
}

void PlayerCore::onRenderIdle() {
    if (video_renderer_) {
        video_renderer_->handleEvents();
        const double duration = demuxer_ ? demuxer_->getMediaInfo().duration : 0.0;
        video_renderer_->setOverlayState(position_.load(), duration, volume_.load(), state_.load() == PlaybackState::Paused);
    }

    if (state_.load() != PlaybackState::Playing || !demuxer_ || !demuxer_->isEof()) {
        return;
    }

    const bool video_done = (!video_packet_queue_ || video_packet_queue_->empty()) && video_queue_.empty();
    const bool audio_done = (!audio_packet_queue_ || audio_packet_queue_->empty()) && audio_queue_.empty();
    if (video_done && audio_done) {
        if (state_.exchange(PlaybackState::Stopped) != PlaybackState::Stopped) {
            LOG_INFO("Playback reached EOF, auto-stopping");
            emitStateChanged(PlaybackState::Stopped);
        }
    }
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
