#include "video_player.h"
#include "video_decoder.h"
#include "audio_decoder.h"
#include "display.h"
#include "audio_player.h"
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
    , playback_speed_(1.0) {
}

VideoPlayer::~VideoPlayer() {
    close();
}

bool VideoPlayer::open(const std::string& filename) {
    AVFormatContext* format_ctx = nullptr;
    
    if (avformat_open_input(&format_ctx, filename.c_str(), nullptr, nullptr) != 0) {
        std::cerr << "Error: Could not open input file: " << filename << std::endl;
        return false;
    }
    
    if (avformat_find_stream_info(format_ctx, nullptr) < 0) {
        std::cerr << "Error: Could not find stream information" << std::endl;
        avformat_close_input(&format_ctx);
        return false;
    }
    
    int video_stream_idx = -1;
    int audio_stream_idx = -1;
    
    for (unsigned int i = 0; i < format_ctx->nb_streams; i++) {
        if (format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && 
            video_stream_idx < 0) {
            video_stream_idx = i;
        } else if (format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO && 
                   audio_stream_idx < 0) {
            audio_stream_idx = i;
        }
    }
    
    if (video_stream_idx < 0 && audio_stream_idx < 0) {
        std::cerr << "Error: Could not find video or audio stream" << std::endl;
        avformat_close_input(&format_ctx);
        return false;
    }
    
    video_decoder_ = std::make_unique<VideoDecoder>();
    audio_decoder_ = std::make_unique<AudioDecoder>();
    display_ = std::make_unique<Display>();
    audio_player_ = std::make_unique<AudioPlayer>();
    
    if (video_stream_idx >= 0) {
        if (!video_decoder_->open(format_ctx, video_stream_idx)) {
            std::cerr << "Error: Failed to open video decoder" << std::endl;
            avformat_close_input(&format_ctx);
            return false;
        }
        
        if (!display_->init(video_decoder_->getWidth(), 
                           video_decoder_->getHeight(), "Video Player")) {
            std::cerr << "Error: Failed to initialize display" << std::endl;
            avformat_close_input(&format_ctx);
            return false;
        }
    }
    
    if (audio_stream_idx >= 0) {
        if (!audio_decoder_->open(format_ctx, audio_stream_idx)) {
            std::cerr << "Warning: Failed to open audio decoder" << std::endl;
            audio_stream_idx = -1;
        } else {
            if (!audio_player_->init(audio_decoder_->getSampleRate(), 
                                     audio_decoder_->getChannels())) {
                std::cerr << "Warning: Failed to initialize audio player" << std::endl;
                audio_stream_idx = -1;
            }
        }
    }
    
    duration_ = format_ctx->duration / (double)AV_TIME_BASE;
    
    std::cout << "Opened file: " << filename << std::endl;
    std::cout << "Duration: " << duration_ << " seconds" << std::endl;
    
    if (video_decoder_) {
        std::cout << "Video: " << video_decoder_->getWidth() << "x" 
                  << video_decoder_->getHeight() << std::endl;
    }
    
    if (audio_decoder_) {
        std::cout << "Audio: " << audio_decoder_->getSampleRate() << "Hz, "
                  << audio_decoder_->getChannels() << " channels" << std::endl;
    }
    
    return true;
}

void VideoPlayer::close() {
    stop();
    
    if (play_thread_.joinable()) {
        play_thread_.join();
    }
    
    video_decoder_.reset();
    audio_decoder_.reset();
    display_.reset();
    audio_player_.reset();
    
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
        }
        return;
    }
    
    playing_.store(true);
    paused_.store(false);
    stopped_.store(false);
    
    if (play_thread_.joinable()) {
        play_thread_.join();
    }
    
    play_thread_ = std::thread(&VideoPlayer::playLoop, this);
}

void VideoPlayer::pause() {
    if (playing_.load()) {
        paused_.store(!paused_.load());
    }
}

void VideoPlayer::stop() {
    stopped_.store(true);
    playing_.store(false);
    paused_.store(false);
    cv_.notify_all();
    
    if (play_thread_.joinable()) {
        play_thread_.join();
    }
    
    if (display_) {
        display_->close();
        if (video_decoder_) {
            display_->init(video_decoder_->getWidth(), 
                         video_decoder_->getHeight(), "Video Player");
        }
    }
    
    current_time_ = 0.0;
}

void VideoPlayer::seek(double timestamp) {
    if (!video_decoder_ && !audio_decoder_) return;
    
    seeking_.store(true);
    
    timestamp = std::max(0.0, std::min(timestamp, duration_));
    
    std::cout << "Seeking to " << timestamp << " seconds" << std::endl;
    
    if (video_decoder_) video_decoder_->flush();
    if (audio_decoder_) audio_decoder_->flush();
    
    current_time_ = timestamp;
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

void VideoPlayer::playLoop() {
    VideoFrame video_frame;
    AudioFrame audio_frame;
    
    auto start_time = Clock::now();
    double video_pts = 0.0;
    
    LOG_TRACE_LOOP("playLoop started, stopped=" << stopped_.load() 
              << ", display=" << (display_ ? "valid" : "null")
              << ", shouldQuit=" << (display_ ? display_->shouldQuit() : false));
    
    int loop_count = 0;
    
    while (!stopped_.load() && display_ && !display_->shouldQuit()) {
        LOG_TRACE_LOOP("Loop iteration " << loop_count++ << ", shouldQuit=" << display_->shouldQuit());
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
        
        if (video_decoder_ && !seeking_.load()) {
            LOG_TRACE_VIDEO("Calling decodeFrame...");
            if (video_decoder_->decodeFrame(video_frame) && video_frame.isValid()) {
                LOG_TRACE_VIDEO("decodeFrame success, pts=" << video_frame.pts());
                video_pts = video_frame.pts();
                display_->handleEvents();
                
                AVFrame* frame = video_frame.get();
                display_->renderFrame(frame->data[0], frame->width, frame->height);
                display_->present();
                
                double delay = (video_pts - current_time_) / playback_speed_;
                if (delay > 0.01) {
                    std::this_thread::sleep_for(std::chrono::duration<double>(delay));
                }
            } else {
                LOG_TRACE_VIDEO("decodeFrame failed or frame invalid");
            }
        } else if (!video_decoder_) {
            LOG_TRACE_VIDEO("video_decoder_ is null");
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    LOG_TRACE_LOOP("Loop exited, stopped=" << stopped_.load() 
              << ", display=" << (display_ ? "valid" : "null")
              << ", shouldQuit=" << (display_ ? display_->shouldQuit() : false));
    playing_.store(false);
}

double VideoPlayer::getMasterClock() {
    return current_time_.load();
}

void VideoPlayer::updateClock() {
}

} // namespace vp
