#include "video_player.h"
#include "video_decoder.h"
#include "audio_decoder.h"
#include "display.h"
#include "audio_player.h"
#include "logger.h"
#include <iostream>
#include <algorithm>

namespace vp {

VideoPlayer::VideoPlayer()
    : playing_(false)
    , paused_(false)
    , stopped_(true)
    , seeking_(false)
    , duration_(0.0)
    , current_time_(0.0)
    , volume_(1.0f)
    , playback_speed_(1.0)
    , format_ctx_(nullptr)
    , video_stream_idx_(-1)
    , audio_stream_idx_(-1) {
    video_frame_queue_ = std::make_unique<FrameQueue<VideoFrame>>(10);
    audio_frame_queue_ = std::make_unique<FrameQueue<AudioFrame>>(10);
}

VideoPlayer::~VideoPlayer() {
    close();
}

bool VideoPlayer::open(const std::string& filename) {
    AVFormatContext* format_ctx = nullptr;
    
    if (avformat_open_input(&format_ctx, filename.c_str(), nullptr, nullptr) != 0) {
        LOG_ERROR("Could not open input file: " + filename);
        return false;
    }
    
    if (avformat_find_stream_info(format_ctx, nullptr) < 0) {
        LOG_ERROR("Could not find stream information");
        avformat_close_input(&format_ctx);
        return false;
    }
    
    video_stream_idx_ = -1;
    audio_stream_idx_ = -1;
    
    for (unsigned int i = 0; i < format_ctx->nb_streams; i++) {
        if (format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && 
            video_stream_idx_ < 0) {
            video_stream_idx_ = i;
        } else if (format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO && 
                   audio_stream_idx_ < 0) {
            audio_stream_idx_ = i;
        }
    }
    
    if (video_stream_idx_ < 0 && audio_stream_idx_ < 0) {
        LOG_ERROR("Could not find video or audio stream");
        avformat_close_input(&format_ctx);
        return false;
    }
    
    video_decoder_ = std::make_unique<VideoDecoder>();
    audio_decoder_ = std::make_unique<AudioDecoder>();
    display_ = std::make_unique<Display>();
    audio_player_ = std::make_unique<AudioPlayer>();
    
    if (video_stream_idx_ >= 0) {
        if (!video_decoder_->open(format_ctx, video_stream_idx_)) {
            LOG_ERROR("Failed to open video decoder");
            avformat_close_input(&format_ctx);
            return false;
        }
        
        if (!display_->init(video_decoder_->getWidth(), 
                           video_decoder_->getHeight(), "Video Player")) {
            LOG_ERROR("Failed to initialize display");
            avformat_close_input(&format_ctx);
            return false;
        }
    }
    
    if (audio_stream_idx_ >= 0) {
        if (!audio_decoder_->open(format_ctx, audio_stream_idx_)) {
            LOG_WARNING("Failed to open audio decoder");
            audio_stream_idx_ = -1;
        } else {
            if (!audio_player_->init(audio_decoder_->getSampleRate(), 
                                     audio_decoder_->getChannels())) {
                LOG_WARNING("Failed to initialize audio player");
                audio_stream_idx_ = -1;
            }
        }
    }
    
    duration_ = format_ctx->duration / (double)AV_TIME_BASE;
    format_ctx_ = format_ctx;
    
    LOG_INFO("Opened file: " + filename);
    LOG_INFO("Duration: " + std::to_string(duration_) + " seconds");
    
    if (video_decoder_) {
        LOG_INFO("Video: " + std::to_string(video_decoder_->getWidth()) + "x" 
                  + std::to_string(video_decoder_->getHeight()));
    }
    
    if (audio_decoder_) {
        LOG_INFO("Audio: " + std::to_string(audio_decoder_->getSampleRate()) + "Hz, "
                  + std::to_string(audio_decoder_->getChannels()) + " channels");
    }
    
    return true;
}

bool VideoPlayer::initDecodeThreads(AVFormatContext* fmt_ctx, int video_stream_idx, int audio_stream_idx) {
    if (video_stream_idx >= 0) {
        video_decode_thread_ = std::make_unique<VideoDecodeThread>();
        if (!video_decode_thread_->start(fmt_ctx, video_stream_idx, video_frame_queue_.get())) {
            LOG_ERROR("Failed to start video decode thread");
            return false;
        }
    }
    
    if (audio_stream_idx >= 0) {
        audio_decode_thread_ = std::make_unique<AudioDecodeThread>();
        if (!audio_decode_thread_->start(fmt_ctx, audio_stream_idx, audio_frame_queue_.get())) {
            LOG_ERROR("Failed to start audio decode thread");
            if (video_decode_thread_) {
                video_decode_thread_->stop();
            }
            return false;
        }
    }
    
    return true;
}

void VideoPlayer::stopDecodeThreads() {
    if (video_decode_thread_) {
        video_decode_thread_->stop();
        video_decode_thread_.reset();
    }
    
    if (audio_decode_thread_) {
        audio_decode_thread_->stop();
        audio_decode_thread_.reset();
    }
    
    if (video_frame_queue_) {
        video_frame_queue_->clear();
    }
    
    if (audio_frame_queue_) {
        audio_frame_queue_->clear();
    }
}

void VideoPlayer::close() {
    stop();
    
    if (render_thread_.joinable()) {
        render_thread_.join();
    }
    
    stopDecodeThreads();
    
    video_decoder_.reset();
    audio_decoder_.reset();
    display_.reset();
    audio_player_.reset();
    
    if (format_ctx_) {
        avformat_close_input(&format_ctx_);
        format_ctx_ = nullptr;
    }
    
    duration_ = 0.0;
    current_time_ = 0.0;
}

void VideoPlayer::play() {
    if (!video_decoder_ && !audio_decoder_) {
        return;
    }
    
    if (playing_.load()) {
        if (paused_.load()) {
            paused_.store(false);
            if (video_decode_thread_) {
                video_decode_thread_->resume();
            }
            if (audio_decode_thread_) {
                audio_decode_thread_->resume();
            }
        }
        return;
    }
    
    playing_.store(true);
    paused_.store(false);
    stopped_.store(false);
    
    if (!initDecodeThreads(format_ctx_, video_stream_idx_, audio_stream_idx_)) {
        LOG_ERROR("Failed to initialize decode threads");
        playing_.store(false);
        return;
    }
    
    if (render_thread_.joinable()) {
        render_thread_.join();
    }
    
    render_thread_ = std::thread(&VideoPlayer::renderLoop, this);
}

void VideoPlayer::pause() {
    if (playing_.load()) {
        paused_.store(!paused_.load());
        
        if (paused_.load()) {
            if (video_decode_thread_) {
                video_decode_thread_->pause();
            }
            if (audio_decode_thread_) {
                audio_decode_thread_->pause();
            }
        } else {
            if (video_decode_thread_) {
                video_decode_thread_->resume();
            }
            if (audio_decode_thread_) {
                audio_decode_thread_->resume();
            }
        }
    }
}

void VideoPlayer::stop() {
    stopped_.store(true);
    playing_.store(false);
    paused_.store(false);
    cv_.notify_all();
    
    stopDecodeThreads();
    
    if (render_thread_.joinable()) {
        render_thread_.join();
    }
    
    if (display_) {
        display_->close();
        if (video_decoder_) {
            display_->init(video_decoder_->getWidth(), 
                         video_decoder_->getHeight(), "Video Player");
        }
    }
    
    current_time_ = 0.0;
    sync_manager_.reset();
}

void VideoPlayer::seek(double timestamp) {
    if (!video_decoder_ && !audio_decoder_) return;
    
    seeking_.store(true);
    
    timestamp = std::max(0.0, std::min(timestamp, duration_));
    
    LOG_INFO("Seeking to " + std::to_string(timestamp) + " seconds");
    
    stopDecodeThreads();
    
    if (format_ctx_) {
        int64_t seek_target = static_cast<int64_t>(timestamp * AV_TIME_BASE);
        av_seek_frame(format_ctx_, -1, seek_target, AVSEEK_FLAG_BACKWARD);
    }
    
    current_time_ = timestamp;
    sync_manager_.reset();
    sync_manager_.setMasterClock(timestamp);
    
    if (playing_.load() && !stopped_.load()) {
        initDecodeThreads(format_ctx_, video_stream_idx_, audio_stream_idx_);
    }
    
    seeking_.store(false);
}

void VideoPlayer::setVolume(float volume) {
    volume_ = std::max(0.0f, std::min(1.0f, volume));
    if (audio_player_) {
        audio_player_->setVolume(volume_);
    }
}

void VideoPlayer::setPlaybackSpeed(double speed) {
    playback_speed_ = std::max(0.5, std::min(2.0, speed));
}

void VideoPlayer::setSyncMode(SyncMode mode) {
    sync_manager_.setMode(mode);
}

void VideoPlayer::renderLoop() {
    VideoFrame video_frame;
    AudioFrame audio_frame;
    
    auto start_time = Clock::now();
    double video_pts = 0.0;
    
    LOG_TRACE_LOOP("Render loop started");
    
    int loop_count = 0;
    
    while (!stopped_.load() && display_ && !display_->shouldQuit()) {
        LOG_TRACE_LOOP("========== Loop iteration {} ==========", loop_count++);
        
        if (paused_.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            start_time = Clock::now() - 
                         std::chrono::duration_cast<std::chrono::nanoseconds>(
                             std::chrono::duration<double>(current_time_.load() / playback_speed_));
            continue;
        }
        
        if (seeking_.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        
        auto current = Clock::now();
        double elapsed = std::chrono::duration<double>(current - start_time).count();
        current_time_ = elapsed * playback_speed_;
        
        sync_manager_.setMasterClock(current_time_.load());
        
        if (video_frame_queue_ && !video_frame_queue_->empty()) {
            LOG_TRACE_VIDEO("Trying to get video frame from queue");
            
            if (video_frame_queue_->pop(video_frame, 50)) {
                if (video_frame.isValid()) {
                    LOG_TRACE_VIDEO("Got video frame, pts={}", video_frame.pts());
                    video_pts = video_frame.pts();
                    sync_manager_.updateVideoPts(video_pts);
                    
                    display_->handleEvents();
                    
                    if (display_->shouldQuit()) {
                        LOG_TRACE_LOOP("shouldQuit is true, will exit loop");
                        break;
                    }
                    
                    AVFrame* frame = video_frame.get();
                    display_->renderFrame((const uint8_t*)frame, frame->width, frame->height);
                    display_->present();
                    
                    double delay = sync_manager_.calculateDelay(video_pts);
                    if (delay > 0.01) {
                        std::this_thread::sleep_for(std::chrono::duration<double>(delay));
                    }
                }
            }
        } else {
            LOG_TRACE_VIDEO("Video frame queue is empty or null");
        }
        
        if (audio_frame_queue_ && !audio_frame_queue_->empty()) {
            if (audio_frame_queue_->tryPop(audio_frame)) {
                if (audio_frame.isValid()) {
                    double audio_pts = audio_frame.pts();
                    sync_manager_.updateAudioPts(audio_pts);
                    
                    if (audio_player_) {
                        const uint8_t* data = audio_frame.getData();
                        int size = audio_frame.getSize();
                        if (data && size > 0) {
                            std::vector<uint8_t> audio_data(data, data + size);
                            audio_player_->play(audio_data);
                        }
                    }
                }
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    LOG_TRACE_LOOP("Render loop exited");
    playing_.store(false);
}

double VideoPlayer::getMasterClock() {
    return current_time_.load();
}

void VideoPlayer::updateClock() {
}

}
