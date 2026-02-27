#include "video_player.h"
#include "display.h"
#include "audio_player.h"
#include "logger.h"

#include <iostream>
#include <algorithm>

extern "C" {
#include <libswresample/swresample.h>
}

namespace vp {

VideoPlayer::VideoPlayer() = default;

VideoPlayer::~VideoPlayer() {
    close();
}

bool VideoPlayer::open(const std::string& filename) {
    demuxer_ = std::make_unique<Demuxer>();
    if (!demuxer_->open(filename)) {
        LOG_ERROR("Could not open file");
        return false;
    }

    const MediaInfo& info = demuxer_->getMediaInfo();

    if (!initComponents()) {
        LOG_ERROR("Failed to initialize components");
        return false;
    }

    LOG_INFO("Opened file");
    return true;
}

void VideoPlayer::close() {
    stop();

    if (render_thread_ && render_thread_->joinable()) {
        render_thread_->join();
    }

    stopThreads();

    video_decoder_.reset();
    audio_decoder_.reset();
    display_.reset();
    audio_player_.reset();
    demuxer_.reset();

    current_time_.store(0.0);
}

bool VideoPlayer::initComponents() {
    if (!demuxer_) {
        return false;
    }

    const MediaInfo& info = demuxer_->getMediaInfo();
    AVFormatContext* fmt_ctx = demuxer_->getFormatContext();

    if (info.video_stream_idx >= 0) {
        display_ = std::make_unique<Display>();
        if (!display_->init(info.width, info.height, "Video Player")) {
            LOG_ERROR("Failed to initialize display");
            return false;
        }

        video_packet_queue_ = std::make_unique<PacketQueue>(100);

        video_decoder_ = std::make_unique<DecoderWorker>();
        AVStream* video_stream = fmt_ctx->streams[info.video_stream_idx];
        if (!video_decoder_->init(video_stream->codecpar, video_stream->time_base)) {
            LOG_ERROR("Failed to initialize video decoder");
            return false;
        }

        video_decoder_->setOutputCallback([this](AVFrame* frame) {
            handleVideoFrame(frame);
        });
    }

    if (info.audio_stream_idx >= 0) {
        audio_player_ = std::make_unique<AudioPlayer>();
        if (!audio_player_->init(info.sample_rate, info.channels)) {
            LOG_WARNING("Failed to initialize audio player");
        }

        audio_packet_queue_ = std::make_unique<PacketQueue>(100);

        audio_decoder_ = std::make_unique<DecoderWorker>();
        AVStream* audio_stream = fmt_ctx->streams[info.audio_stream_idx];
        if (!audio_decoder_->init(audio_stream->codecpar, audio_stream->time_base)) {
            LOG_WARNING("Failed to initialize audio decoder");
        } else {
            audio_decoder_->setOutputCallback([this](AVFrame* frame) {
                handleAudioFrame(frame);
            });
        }
    }

    return true;
}

void VideoPlayer::startThreads() {
    if (video_decoder_ && video_packet_queue_) {
        video_decoder_->start(video_packet_queue_.get());
    }

    if (audio_decoder_ && audio_packet_queue_) {
        audio_decoder_->start(audio_packet_queue_.get());
    }

    demuxer_thread_ = std::make_unique<std::thread>([this]() {
        while (!stopped_.load() && demuxer_ && !demuxer_->isEof()) {
            if (paused_.load()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }

            bool video_full = video_packet_queue_ && video_packet_queue_->full();
            bool audio_full = audio_packet_queue_ && audio_packet_queue_->full();

            if (video_full && audio_full) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }

            AVPacket* packet = av_packet_alloc();
            if (!packet) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }

            if (!demuxer_->readPacket(packet)) {
                av_packet_free(&packet);
                if (video_packet_queue_) video_packet_queue_->setEof(true);
                if (audio_packet_queue_) audio_packet_queue_->setEof(true);
                break;
            }

            const MediaInfo& info = demuxer_->getMediaInfo();
            if (packet->stream_index == info.video_stream_idx && video_packet_queue_ && !video_full) {
                video_packet_queue_->push(packet, 10);
            } else if (packet->stream_index == info.audio_stream_idx && audio_packet_queue_ && !audio_full) {
                audio_packet_queue_->push(packet, 10);
            } else {
                av_packet_free(&packet);
            }
        }
    });

    render_thread_ = std::make_unique<std::thread>(&VideoPlayer::renderLoop, this);
}

void VideoPlayer::stopThreads() {
    if (video_packet_queue_) {
        video_packet_queue_->stop();
    }
    if (audio_packet_queue_) {
        audio_packet_queue_->stop();
    }

    if (video_decoder_) {
        video_decoder_->stop();
    }
    if (audio_decoder_) {
        audio_decoder_->stop();
    }

    if (demuxer_thread_ && demuxer_thread_->joinable()) {
        demuxer_thread_->join();
    }
    demuxer_thread_.reset();
}

void VideoPlayer::play() {
    if (playing_.load()) {
        if (paused_.load()) {
            paused_.store(false);
            if (video_decoder_) video_decoder_->resume();
            if (audio_decoder_) audio_decoder_->resume();
        }
        return;
    }

    playing_.store(true);
    paused_.store(false);
    stopped_.store(false);

    demuxer_->seek(0);
    clock_.reset();
    clock_.setMasterClock(0);

    startThreads();

    LOG_INFO("Playback started");
}

void VideoPlayer::pause() {
    if (playing_.load()) {
        paused_.store(!paused_.load());
        if (video_decoder_) {
            if (paused_.load()) video_decoder_->pause();
            else video_decoder_->resume();
        }
        if (audio_decoder_) {
            if (paused_.load()) audio_decoder_->pause();
            else audio_decoder_->resume();
        }
    }
}

void VideoPlayer::stop() {
    stopped_.store(true);
    playing_.store(false);
    paused_.store(false);
    cv_.notify_all();

    stopThreads();

    if (render_thread_ && render_thread_->joinable()) {
        render_thread_->join();
    }
    render_thread_.reset();

    if (display_) {
        display_->close();
        const MediaInfo& info = demuxer_->getMediaInfo();
        if (info.video_stream_idx >= 0) {
            display_->init(info.width, info.height, "Video Player");
        }
    }

    current_time_.store(0.0);
    clock_.reset();

    LOG_INFO("Playback stopped");
}

void VideoPlayer::seek(double timestamp) {
    if (!demuxer_) return;

    seeking_.store(true);
    timestamp = std::max(0.0, std::min(timestamp, getDuration()));

    stopThreads();

    demuxer_->seek(timestamp);

    if (video_packet_queue_) {
        video_packet_queue_->clear();
        video_packet_queue_->start();
    }
    if (audio_packet_queue_) {
        audio_packet_queue_->clear();
        audio_packet_queue_->start();
    }
    if (video_decoder_) video_decoder_->flush();
    if (audio_decoder_) audio_decoder_->flush();

    current_time_.store(timestamp);
    clock_.setMasterClock(timestamp);

    if (playing_.load() && !stopped_.load()) {
        startThreads();
    }

    seeking_.store(false);
    LOG_INFO("Seek completed");
}

double VideoPlayer::getDuration() const {
    return demuxer_ ? demuxer_->getMediaInfo().duration : 0.0;
}

void VideoPlayer::setVolume(float volume) {
    volume_ = std::max(0.0f, std::min(1.0f, volume));
    if (audio_player_) {
        audio_player_->setVolume(volume_);
    }
}

void VideoPlayer::setPlaybackSpeed(double speed) {
    playback_speed_ = std::max(0.5, std::min(2.0, speed));
    clock_.setPlaybackSpeed(playback_speed_);
}

void VideoPlayer::setSyncMode(SyncMode mode) {
    clock_.setMode(mode);
}

void VideoPlayer::handleVideoFrame(AVFrame* frame) {
    if (!frame) return;

    if (pending_video_frame_) {
        av_frame_free(&pending_video_frame_);
    }

    pending_video_frame_ = av_frame_clone(frame);

    if (frame->pts != AV_NOPTS_VALUE) {
        const MediaInfo& info = demuxer_->getMediaInfo();
        AVRational tb = info.video_time_base;
        pending_video_pts_ = frame->pts * av_q2d(tb);
    }
}

void VideoPlayer::handleAudioFrame(AVFrame* frame) {
    if (!frame || !audio_player_) return;

    SwrContext* swr_ctx = nullptr;
    AVChannelLayout dst_ch_layout;
    av_channel_layout_default(&dst_ch_layout, frame->ch_layout.nb_channels);

    swr_alloc_set_opts2(&swr_ctx,
                        &dst_ch_layout, AV_SAMPLE_FMT_S16, frame->sample_rate,
                        &frame->ch_layout, (AVSampleFormat)frame->format, frame->sample_rate,
                        0, nullptr);

    if (!swr_ctx || swr_init(swr_ctx) < 0) {
        if (swr_ctx) swr_free(&swr_ctx);
        return;
    }

    int dst_nb_samples = av_rescale_rnd(swr_get_delay(swr_ctx, frame->sample_rate) + frame->nb_samples,
                                        frame->sample_rate, frame->sample_rate, AV_ROUND_UP);

    uint8_t* dst_data = nullptr;
    int dst_linesize = 0;
    av_samples_alloc(&dst_data, &dst_linesize, frame->ch_layout.nb_channels, dst_nb_samples, AV_SAMPLE_FMT_S16, 0);

    int converted = swr_convert(swr_ctx, &dst_data, dst_nb_samples,
                                (const uint8_t**)frame->data, frame->nb_samples);

    int size = converted * frame->ch_layout.nb_channels * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);

    std::vector<uint8_t> audio_data(dst_data, dst_data + size);
    audio_player_->play(audio_data);

    av_free(dst_data);
    swr_free(&swr_ctx);
}

void VideoPlayer::renderLoop() {
    while (!stopped_.load() && display_ && !display_->shouldQuit()) {
        if (paused_.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        if (seeking_.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        display_->handleEvents();
        if (display_->shouldQuit()) {
            break;
        }

        if (pending_video_frame_) {
            double delay = clock_.calculateDelay(pending_video_pts_);

            if (delay > 0.01) {
                std::this_thread::sleep_for(std::chrono::duration<double>(delay));
            }

            clock_.updateVideoPts(pending_video_pts_);
            current_time_.store(pending_video_pts_);

            display_->renderFrame((const uint8_t*)pending_video_frame_,
                                  pending_video_frame_->width, pending_video_frame_->height);
            display_->present();

            av_frame_free(&pending_video_frame_);
            pending_video_frame_ = nullptr;
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    playing_.store(false);
    LOG_INFO("Render loop exited");
}

}
