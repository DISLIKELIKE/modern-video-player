#include "core/player_core.h"

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <limits>
#include <sstream>
#include <vector>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

extern "C" {
#include <libavutil/hwcontext.h>
#include <libavutil/pixdesc.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}

#if defined(_WIN32)
#include <d3d11.h>
#include <libavutil/hwcontext_d3d11va.h>
#endif

#include "audio_player.h"
#include "decoder/decoder_factory.h"
#include "filters/builtin_filters.h"
#include "logger.h"
#include "render/renderer_factory.h"
#include "subtitle/subtitle_timeline.h"

namespace vp::core {

namespace {
constexpr int kPacketQueueSize = 256;
constexpr double kMaxAudioBufferedSeconds = 0.35;
constexpr double kMaxMediaDelaySeconds = 5.0;

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

const char* rendererTypeName(render::VideoRendererType type) {
    switch (type) {
    case render::VideoRendererType::Auto:
        return "Auto";
    case render::VideoRendererType::SoftwareSDL:
        return "SoftwareSDL";
    case render::VideoRendererType::D3D11:
        return "D3D11";
    case render::VideoRendererType::OpenGL:
        return "OpenGL";
    default:
        return "Unknown";
    }
}

bool isHardwarePixelFormat(AVPixelFormat format) {
    const AVPixFmtDescriptor* desc = av_pix_fmt_desc_get(format);
    return desc && (desc->flags & AV_PIX_FMT_FLAG_HWACCEL) != 0;
}
std::string backendOrderToString(const std::vector<decoder::DecoderBackend>& order) {
    if (order.empty()) {
        return "none";
    }

    std::string result;
    for (size_t i = 0; i < order.size(); ++i) {
        if (i > 0) {
            result += " -> ";
        }
        result += decoder::DecoderFactory::backendName(order[i]);
    }
    return result;
}
}

/// 鏋勯€犳挱鏀炬牳蹇冨苟缁戝畾璋冨害鍣ㄣ€侀槦鍒椼€佹护闀滀笌榛樿鍥炶皟銆?
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

/// 鎵撳紑濯掍綋骞朵覆璧锋覆鏌撳櫒銆侀煶棰戣澶囥€佽В鐮佸櫒鍜屽唴閮ㄩ槦鍒楄祫婧愩€?
bool PlayerCore::open(const std::string& filename) {
    close();

    demuxer_ = std::make_unique<Demuxer>();
    if (!demuxer_->open(filename)) {
        emitError(ErrorCode::FileNotFound, "failed to open media file");
        return false;
    }

    const MediaInfo& info = demuxer_->getMediaInfo();
    rebuildChapterPoints();
    if (info.video_stream_idx >= 0) {
        const render::VideoRendererType selected_type = render::RendererFactory::detectBestRendererType();
        render::VideoRendererConfig renderer_config{};
        renderer_config.width = info.width;
        renderer_config.height = info.height;
        renderer_config.title = "Video Player";

        auto init_renderer = [&](render::VideoRendererType type) -> bool {
            video_renderer_ = render::RendererFactory::create(type);
            if (!video_renderer_) {
                return false;
            }
            if (!video_renderer_->init(renderer_config)) {
                video_renderer_.reset();
                return false;
            }
            LOG_INFO("Video renderer initialized: " << rendererTypeName(type));
            return true;
        };

        if (!init_renderer(selected_type)) {
            LOG_WARNING("Video renderer init failed: " << rendererTypeName(selected_type));
            if (selected_type != render::VideoRendererType::SoftwareSDL) {
                LOG_WARNING("Falling back to SoftwareSDL renderer");
                if (!init_renderer(render::VideoRendererType::SoftwareSDL)) {
                    emitError(ErrorCode::DisplayInitFailed, "failed to initialize video renderer");
                    return false;
                }
            } else {
                emitError(ErrorCode::DisplayInitFailed, "failed to initialize video renderer");
                return false;
            }
        }

        video_renderer_->setHotkeyManager(hotkey_manager_);
    }

    // Audio output is optional; if initialization fails we allow video-only playback.
    if (info.audio_stream_idx >= 0) {
        audio_player_ = std::make_unique<AudioPlayer>();
        if (!audio_player_->init(info.sample_rate, info.channels)) {
            emitError(ErrorCode::AudioInitFailed, "failed to initialize audio");
            audio_player_.reset();
        }
    }

    // Decoder init is required for playback; fail fast if it cannot be set up.
    if (!initDecoders()) {
        emitError(ErrorCode::DecoderInitFailed, "failed to initialize decoders");
        return false;
    }

    // Create packet queues only for streams we can actually decode/output.
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
    quit_requested_.store(false);
    next_item_requested_.store(false);
    previous_item_requested_.store(false);
    clearABRepeat();
    screenshot_requested_.store(false);
    last_video_frame_duration_.store(0.0);
    {
        std::lock_guard<std::mutex> lock(screenshot_mutex_);
        last_screenshot_path_.clear();
        screenshot_path_pending_ = false;
    }
    clearLastRenderedFrame();
    clock_.reset();
    const bool has_audio_clock = audio_codec_ctx_ && audio_player_ && audio_player_->isInitialized();
    clock_.setSource(has_audio_clock ? ClockSource::Audio : ClockSource::System);
    resetDiagnostics();
    return true;
}

/// 鍋滄鍏ㄩ摼璺嚎绋嬪苟閲婃斁瑙ｇ爜銆佹覆鏌撱€佸瓧骞曞拰鎴浘缂撳瓨璧勬簮銆?
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
    clearExternalSubtitles();
    chapter_points_.clear();
    clearABRepeat();
    screenshot_requested_.store(false);
    last_video_frame_duration_.store(0.0);
    {
        std::lock_guard<std::mutex> lock(screenshot_mutex_);
        last_screenshot_path_.clear();
        screenshot_path_pending_ = false;
    }
    clearLastRenderedFrame();
    opened_.store(false);
}

/// 浠庡仠姝㈡€佸惎鍔ㄧ嚎绋嬶紝鎴栦粠鏆傚仠鎬佹仮澶嶈皟搴︺€佹椂閽熷拰闊抽璁惧銆?
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

    // Transition from Stopped -> Playing: start the pipeline threads.
    startDemuxThread();
    startAudioConsumer();
    scheduler_.start();
    resetDiagnostics();
    state_.store(PlaybackState::Playing);
    emitStateChanged(PlaybackState::Playing);
}

/// 鏆傚仠璋冨害鍣ㄤ笌涓绘椂閽燂紝浣嗕繚鐣欏綋鍓嶈В鐮佸櫒鍜岀紦鍐查槦鍒楃姸鎬併€?
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

/// 鍋滄鐢熶骇/娑堣垂绾跨▼骞舵竻绌虹绾匡紝鍚屾椂鎶婂獟浣撲綅缃浣嶅埌寮€澶淬€?
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

/// 鎵ц涓€娆″甫 flush 鐨勬椂闂寸嚎鍒囨崲锛岀‘淇濇棫鍖呫€佹棫甯у拰鏃ч煶棰戠紦鍐插叏閮ㄥけ鏁堛€?
void PlayerCore::seek(double timestamp) {
    if (!opened_.load() || !demuxer_) {
        return;
    }
    const double duration = demuxer_->getMediaInfo().duration;
    if (duration > 0.0) {
        timestamp = std::max(0.0, std::min(duration, timestamp));
    } else {
        timestamp = std::max(0.0, timestamp);
    }
    LOG_INFO("Seek request: target=" << timestamp << "s");
    const bool was_playing = state_.load() == PlaybackState::Playing;
    const bool demux_was_running = demux_running_.load();
    // Pause scheduling before flushing stale data.
    if (was_playing) {
        scheduler_.pause();
    }

    if (demux_was_running) {
        stopDemuxThread();
    }

    flushPipelines();
    const bool seek_ok = demuxer_->seek(timestamp);
    {
        // Flush decoder state under both codec locks.
        std::scoped_lock codec_lock(video_codec_mutex_, audio_codec_mutex_);
        if (video_codec_ctx_) {
            avcodec_flush_buffers(video_codec_ctx_);
        }
        if (audio_codec_ctx_) {
            avcodec_flush_buffers(audio_codec_ctx_);
        }
        releaseAudioResampler();
    }
    if (!seek_ok) {
        emitError(ErrorCode::SeekFailed, "seek failed");
    }
    if (audio_player_) {
        // Drop stale buffered audio so seek takes effect immediately.
        audio_player_->stop();
    }
    flushPipelines();
    position_.store(timestamp);
    clock_.setTime(timestamp);
    clock_.setAudioClock(timestamp);
    clock_.setVideoClock(timestamp);
    updateSubtitleOverlay(timestamp);
    emitPositionChanged(timestamp);

    if (demux_was_running) {
        startDemuxThread();
    }
    if (was_playing) {
        if (audio_player_) {
            audio_player_->resume();
        }
        scheduler_.resume();
    }
}

/// 鍦ㄦ殏鍋滄€佹寜浼扮畻甯ч棿闅旀墽琛屽崟甯ф杩涳紝骞跺皾璇曞懡涓洰鏍囨椂闂撮檮杩戠殑瑙嗛甯с€?
bool PlayerCore::stepFrame(int direction) {
    if (!opened_.load() || !demuxer_ || !video_renderer_ || state_.load() != PlaybackState::Paused) {
        return false;
    }
    if (direction == 0) {
        return false;
    }

    const double step_seconds = std::max(1.0 / 240.0, estimateFrameStepSeconds());
    const double current = position_.load();
    const double duration = demuxer_->getMediaInfo().duration;
    double target = current + (direction < 0 ? -step_seconds : step_seconds);
    if (duration > 0.0) {
        target = std::max(0.0, std::min(duration, target));
    } else {
        target = std::max(0.0, target);
    }

    LOG_INFO("Frame step " << (direction < 0 ? "backward" : "forward")
             << ": current=" << current << "s target=" << target << "s step=" << step_seconds << "s");

    seek(target);
    return renderPausedFrameAtOrAfter(target);
}

/// 鎸夋渶杩戞覆鏌撳抚鏃堕暱銆佺紪鐮佸櫒甯х巼鍜屽獟浣撳抚鐜囦及绠楀崟甯ф椂闀裤€?
double PlayerCore::estimateFrameStepSeconds() const {
    const double cached_duration = last_video_frame_duration_.load();
    if (cached_duration > 0.0) {
        return cached_duration;
    }
    if (video_codec_ctx_ && video_codec_ctx_->framerate.num > 0 && video_codec_ctx_->framerate.den > 0) {
        return 1.0 / av_q2d(video_codec_ctx_->framerate);
    }
    if (demuxer_) {
        const double fps = demuxer_->getMediaInfo().fps;
        if (fps > 0.0) {
            return 1.0 / fps;
        }
    }
    return 1.0 / 30.0;
}

/// 鍦ㄦ殏鍋滄€佹秷璐规垨涓诲姩瑙ｇ爜涓€甯э紝骞舵覆鏌撳埌涓嶆棭浜庣洰鏍囨椂闂寸殑浣嶇疆銆?
bool PlayerCore::renderPausedFrameAtOrAfter(double target_seconds) {
    if (!opened_.load() || !video_renderer_) {
        return false;
    }

    constexpr int kMaxAttempts = 120;
    constexpr double kToleranceSeconds = 0.001;
    // Prefer queued frames before forcing another decode while paused.
    for (int attempt = 0; attempt < kMaxAttempts; ++attempt) {
        VideoFrame frame;
        bool have_frame = video_queue_.pop(frame, std::chrono::milliseconds(0));
        if (!have_frame || !frame.valid) {
            have_frame = decodeVideoFrame(frame);
        }
        if (!have_frame || !frame.valid) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }
        // Skip stale frames until we reach or pass the requested position.
        if (frame.pts + kToleranceSeconds < target_seconds) {
            continue;
        }

        const double frame_pts = frame.pts;
        if (frame.duration > 0.0) {
            last_video_frame_duration_.store(frame.duration);
        }
        position_.store(frame_pts);
        clock_.setTime(frame_pts);
        clock_.setAudioClock(frame_pts);
        clock_.setVideoClock(frame_pts);
        renderFrame(std::move(frame));
        position_.store(frame_pts);
        clock_.setTime(frame_pts);
        clock_.setAudioClock(frame_pts);
        clock_.setVideoClock(frame_pts);
        last_position_emit_tp_ = std::chrono::steady_clock::time_point{};
        emitPositionChanged(frame_pts);
        return true;
    }

    return false;
}

bool PlayerCore::seekToNextChapter() {
    if (chapter_points_.empty()) {
        return false;
    }

    constexpr double kEpsilonSeconds = 0.2;
    const double current = position_.load();
    const auto next_it = std::upper_bound(chapter_points_.begin(), chapter_points_.end(), current + kEpsilonSeconds);
    if (next_it == chapter_points_.end()) {
        return false;
    }

    const double target = *next_it;
    LOG_INFO("Chapter next: " << current << "s -> " << target << "s");
    seek(target);
    return true;
}

bool PlayerCore::seekToPreviousChapter() {
    if (chapter_points_.empty()) {
        return false;
    }

    constexpr double kEpsilonSeconds = 0.2;
    const double current = position_.load();
    const auto first_not_before = std::lower_bound(chapter_points_.begin(), chapter_points_.end(), current - kEpsilonSeconds);
    if (first_not_before == chapter_points_.begin()) {
        if (current > kEpsilonSeconds) {
            LOG_INFO("Chapter previous: " << current << "s -> 0s");
            seek(0.0);
            return true;
        }
        return false;
    }

    const auto previous_it = std::prev(first_not_before);
    const double target = *previous_it;
    LOG_INFO("Chapter previous: " << current << "s -> " << target << "s");
    seek(target);
    return true;
}

size_t PlayerCore::chapterCount() const {
    return chapter_points_.size();
}

bool PlayerCore::setABRepeatStart() {
    if (!opened_.load()) {
        return false;
    }

    double start = position_.load();
    const double duration = demuxer_ ? demuxer_->getMediaInfo().duration : 0.0;
    if (duration > 0.0) {
        start = std::max(0.0, std::min(duration, start));
    } else {
        start = std::max(0.0, start);
    }

    ab_repeat_start_.store(start);
    ab_repeat_end_.store(-1.0);
    ab_repeat_enabled_.store(false);
    ab_repeat_last_loop_ms_.store(0);
    LOG_INFO("A-B repeat start set: A=" << start << "s");
    return true;
}

bool PlayerCore::setABRepeatEnd() {
    if (!opened_.load()) {
        return false;
    }

    constexpr double kMinSegmentSeconds = 0.2;
    const double start = ab_repeat_start_.load();
    if (start < 0.0) {
        LOG_WARNING("A-B repeat end ignored: A point is not set");
        return false;
    }

    double end = position_.load();
    const double duration = demuxer_ ? demuxer_->getMediaInfo().duration : 0.0;
    if (duration > 0.0) {
        end = std::max(0.0, std::min(duration, end));
    } else {
        end = std::max(0.0, end);
    }

    if (end <= start + kMinSegmentSeconds) {
        LOG_WARNING("A-B repeat end ignored: B(" << end << "s) must be greater than A(" << start << "s)");
        return false;
    }

    ab_repeat_end_.store(end);
    ab_repeat_enabled_.store(true);
    ab_repeat_last_loop_ms_.store(0);
    LOG_INFO("A-B repeat enabled: [" << start << "s, " << end << "s]");
    return true;
}

void PlayerCore::clearABRepeat() {
    const bool had_repeat = ab_repeat_enabled_.load() ||
                            ab_repeat_start_.load() >= 0.0 ||
                            ab_repeat_end_.load() >= 0.0;
    ab_repeat_enabled_.store(false);
    ab_repeat_start_.store(-1.0);
    ab_repeat_end_.store(-1.0);
    ab_repeat_last_loop_ms_.store(0);
    if (had_repeat) {
        LOG_INFO("A-B repeat cleared");
    }
}

bool PlayerCore::isABRepeatEnabled() const {
    return ab_repeat_enabled_.load();
}

double PlayerCore::abRepeatStart() const {
    return ab_repeat_start_.load();
}

double PlayerCore::abRepeatEnd() const {
    return ab_repeat_end_.load();
}

bool PlayerCore::requestScreenshot() {
    if (!opened_.load() || !video_renderer_) {
        return false;
    }

    if (state_.load() != PlaybackState::Playing) {
        if (!captureScreenshotFromCachedFrame()) {
            LOG_WARNING("Screenshot request failed: no cached frame available");
            return false;
        }
        return true;
    }

    screenshot_requested_.store(true);
    return true;
}

bool PlayerCore::consumeLastScreenshotPath(std::string& path) {
    std::lock_guard<std::mutex> lock(screenshot_mutex_);
    if (!screenshot_path_pending_ || last_screenshot_path_.empty()) {
        return false;
    }
    path = last_screenshot_path_;
    screenshot_path_pending_ = false;
    return true;
}

bool PlayerCore::stepFrameBackward() {
    return stepFrame(-1);
}

bool PlayerCore::stepFrameForward() {
    return stepFrame(1);
}

/// 鎷夊彇鏄剧ず灞備竴娆℃€ц姹傦紝骞舵槧灏勪负 seek銆侀煶閲忋€佺珷鑺傚拰瀛楀箷绛夋挱鏀炬帶鍒躲€?
void PlayerCore::pumpEvents() {
    if (video_renderer_) {
        video_renderer_->handleEvents();

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

        double seek_delta_seconds = 0.0;
        if (video_renderer_->consumeSeekDeltaRequest(seek_delta_seconds) && seek_delta_seconds != 0.0) {
            const double duration = demuxer_ ? demuxer_->getMediaInfo().duration : 0.0;
            const double current = position_.load();
            if (duration > 0.0) {
                const double target = std::max(0.0, std::min(duration, current + seek_delta_seconds));
                seek(target);
            }
        }

        if (video_renderer_->consumeNextChapterRequest()) {
            seekToNextChapter();
        }

        if (video_renderer_->consumePreviousChapterRequest()) {
            seekToPreviousChapter();
        }

        float volume_request = 0.0f;
        if (video_renderer_->consumeVolumeChangeRequest(volume_request)) {
            setVolume(volume_request);
        }

        double speed_delta = 0.0;
        if (video_renderer_->consumeSpeedChangeRequest(speed_delta) && speed_delta != 0.0) {
            setPlaybackSpeed(getPlaybackSpeed() + speed_delta);
        }
        if (video_renderer_->consumeResetSpeedRequest()) {
            setPlaybackSpeed(1.0);
        }

        double subtitle_delay_delta = 0.0;
        if (video_renderer_->consumeSubtitleDelayChangeRequest(subtitle_delay_delta) && subtitle_delay_delta != 0.0) {
            setSubtitleDelay(getSubtitleDelay() + subtitle_delay_delta);
        }

        double audio_delay_delta = 0.0;
        if (video_renderer_->consumeAudioDelayChangeRequest(audio_delay_delta) && audio_delay_delta != 0.0) {
            setAudioDelay(getAudioDelay() + audio_delay_delta);
        }

        if (video_renderer_->consumeToggleSubtitleRequest()) {
            toggleSubtitleEnabled();
        }

        if (video_renderer_->consumeSetABRepeatStartRequest()) {
            setABRepeatStart();
        }

        if (video_renderer_->consumeSetABRepeatEndRequest()) {
            setABRepeatEnd();
        }

        if (video_renderer_->consumeClearABRepeatRequest()) {
            clearABRepeat();
        }

        if (video_renderer_->consumeScreenshotRequest() && !requestScreenshot()) {
            LOG_WARNING("Screenshot request failed");
        }

        if (video_renderer_->consumeStepFrameBackwardRequest() && !stepFrameBackward()) {
            LOG_WARNING("Frame step backward request failed");
        }

        if (video_renderer_->consumeStepFrameForwardRequest() && !stepFrameForward()) {
            LOG_WARNING("Frame step forward request failed");
        }

        if (video_renderer_->consumeNextItemRequest()) {
            next_item_requested_.store(true);
            if (state_.exchange(PlaybackState::Stopped) != PlaybackState::Stopped) {
                emitStateChanged(PlaybackState::Stopped);
            }
        }

        if (video_renderer_->consumePreviousItemRequest()) {
            previous_item_requested_.store(true);
            if (state_.exchange(PlaybackState::Stopped) != PlaybackState::Stopped) {
                emitStateChanged(PlaybackState::Stopped);
            }
        }

        if (video_renderer_->shouldQuit()) {
            quit_requested_.store(true);
            if (state_.exchange(PlaybackState::Stopped) != PlaybackState::Stopped) {
                emitStateChanged(PlaybackState::Stopped);
            }
        }
    }

    handleABRepeatLoop();
}

/// 浠庣珷鑺傚厓鏁版嵁閲嶅缓鏈夊簭璺宠浆鏃堕棿鐐癸紝渚涗笂涓€绔?涓嬩竴绔犻€昏緫澶嶇敤銆?
void PlayerCore::rebuildChapterPoints() {
    chapter_points_.clear();
    if (!demuxer_) {
        return;
    }

    const MediaInfo& info = demuxer_->getMediaInfo();
    chapter_points_.reserve(info.chapters.size());
    for (const auto& chapter : info.chapters) {
        if (chapter.start >= 0.0) {
            chapter_points_.push_back(chapter.start);
        }
    }

    std::sort(chapter_points_.begin(), chapter_points_.end());
    chapter_points_.erase(std::unique(chapter_points_.begin(), chapter_points_.end(), [](double lhs, double rhs) {
        return std::abs(lhs - rhs) < 0.001;
    }), chapter_points_.end());

    if (!chapter_points_.empty()) {
        LOG_INFO("Detected chapters: " << chapter_points_.size());
    }
}

/// 鍦ㄦ挱鏀句綅缃Е杈?B 鐐归檮杩戞椂鍥炶烦鍒?A 鐐癸紝褰㈡垚闂幆鎾斁銆?
void PlayerCore::handleABRepeatLoop() {
    if (state_.load() != PlaybackState::Playing || !ab_repeat_enabled_.load()) {
        return;
    }

    const double start = ab_repeat_start_.load();
    const double end = ab_repeat_end_.load();
    if (start < 0.0 || end <= start) {
        return;
    }

    constexpr double kLoopTriggerEpsilon = 0.03;
    constexpr int64_t kLoopMinIntervalMs = 120;
    const double current = position_.load();
    if (current + kLoopTriggerEpsilon < end) {
        return;
    }

    const int64_t now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    const int64_t last_ms = ab_repeat_last_loop_ms_.load();
    if (last_ms != 0 && (now_ms - last_ms) < kLoopMinIntervalMs) {
        return;
    }
    ab_repeat_last_loop_ms_.store(now_ms);

    LOG_INFO("A-B repeat loop: " << current << "s -> " << start << "s");
    seek(start);
}

bool PlayerCore::captureScreenshot(const VideoFrame& frame) {
    if (!frame.valid || !frame.frame) {
        return false;
    }

    return captureScreenshotFrame(frame.frame);
}

/// 缂撳瓨鏈€杩戜竴娆℃垚鍔熸覆鏌撶殑甯э紝渚涙殏鍋滄€佹埅鍥惧拰閫愬抚娴忚澶嶇敤銆?
void PlayerCore::updateLastRenderedFrame(const VideoFrame& frame) {
    if (!frame.valid || !frame.frame) {
        return;
    }

    std::lock_guard<std::mutex> lock(rendered_frame_mutex_);
    if (!last_rendered_frame_) {
        last_rendered_frame_ = av_frame_alloc();
        if (!last_rendered_frame_) {
            LOG_WARNING("Failed to allocate cached frame for screenshot support");
            return;
        }
    }

    av_frame_unref(last_rendered_frame_);
    if (av_frame_ref(last_rendered_frame_, frame.frame) < 0) {
        LOG_WARNING("Failed to cache last rendered frame for screenshot support");
        av_frame_unref(last_rendered_frame_);
    }
}

void PlayerCore::clearLastRenderedFrame() {
    std::lock_guard<std::mutex> lock(rendered_frame_mutex_);
    if (last_rendered_frame_) {
        av_frame_free(&last_rendered_frame_);
    }
}

/// 浠庣紦瀛樺抚澶嶅埗鍑轰竴涓揩鐓э紝骞跺鐢ㄧ粺涓€鐨勬埅鍥捐惤鐩橀€昏緫銆?
bool PlayerCore::captureScreenshotFromCachedFrame() {
    AVFrame* cached_frame = nullptr;
    {
        std::lock_guard<std::mutex> lock(rendered_frame_mutex_);
        if (!last_rendered_frame_) {
            return false;
        }

        cached_frame = av_frame_alloc();
        if (!cached_frame) {
            LOG_WARNING("Failed to allocate cached screenshot frame clone");
            return false;
        }
        if (av_frame_ref(cached_frame, last_rendered_frame_) < 0) {
            av_frame_free(&cached_frame);
            LOG_WARNING("Failed to clone cached frame for screenshot");
            return false;
        }
    }

    const bool ok = captureScreenshotFrame(cached_frame);
    av_frame_free(&cached_frame);
    return ok;
}

bool PlayerCore::captureScreenshotFrame(const AVFrame* src) {
    if (!src) {
        return false;
    }

    if (src->width <= 0 || src->height <= 0) {
        return false;
    }

    const AVFrame* screenshot_source = src;
    AVFrame* transferred_frame = nullptr;
    const AVPixelFormat src_format = static_cast<AVPixelFormat>(src->format);
    if (isHardwarePixelFormat(src_format)) {
        transferred_frame = av_frame_alloc();
        if (!transferred_frame) {
            LOG_WARNING("Failed to allocate software frame for screenshot transfer");
            return false;
        }
        if (av_hwframe_transfer_data(transferred_frame, src, 0) < 0 ||
            av_frame_copy_props(transferred_frame, src) < 0) {
            av_frame_free(&transferred_frame);
            LOG_WARNING("Failed to transfer hardware frame for screenshot");
            return false;
        }
        screenshot_source = transferred_frame;
    }

    SwsContext* screenshot_sws = sws_getContext(
        screenshot_source->width,
        screenshot_source->height,
        static_cast<AVPixelFormat>(screenshot_source->format),
        screenshot_source->width,
        screenshot_source->height,
        AV_PIX_FMT_RGB24,
        SWS_BILINEAR,
        nullptr,
        nullptr,
        nullptr);
    if (!screenshot_sws) {
        if (transferred_frame) {
            av_frame_free(&transferred_frame);
        }
        LOG_WARNING("Failed to initialize screenshot scaler");
        return false;
    }

    std::vector<uint8_t> rgb(static_cast<size_t>(screenshot_source->width) * static_cast<size_t>(screenshot_source->height) * 3U);
    uint8_t* dst_data[4]{rgb.data(), nullptr, nullptr, nullptr};
    int dst_linesize[4]{screenshot_source->width * 3, 0, 0, 0};
    const int converted_rows = sws_scale(
        screenshot_sws,
        screenshot_source->data,
        screenshot_source->linesize,
        0,
        screenshot_source->height,
        dst_data,
        dst_linesize);
    sws_freeContext(screenshot_sws);
    if (transferred_frame) {
        av_frame_free(&transferred_frame);
    }
    if (converted_rows <= 0) {
        LOG_WARNING("Failed to convert frame for screenshot");
        return false;
    }

    const std::filesystem::path screenshot_dir("screenshots");
    std::error_code ec;
    std::filesystem::create_directories(screenshot_dir, ec);
    if (ec) {
        LOG_WARNING("Failed to create screenshot directory: " << screenshot_dir.string());
        return false;
    }

    const auto now = std::chrono::system_clock::now();
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    const std::time_t now_tt = std::chrono::system_clock::to_time_t(now);
    std::tm local_tm{};
#if defined(_WIN32)
    localtime_s(&local_tm, &now_tt);
#else
    local_tm = *std::localtime(&now_tt);
#endif

    std::ostringstream filename;
    filename << "screenshot_"
             << std::put_time(&local_tm, "%Y%m%d_%H%M%S")
             << '_' << std::setw(3) << std::setfill('0') << ms.count()
             << ".ppm";
    const std::filesystem::path screenshot_path = screenshot_dir / filename.str();

    std::ofstream output(screenshot_path, std::ios::binary);
    if (!output) {
        LOG_WARNING("Failed to open screenshot file for writing: " << screenshot_path.string());
        return false;
    }

    output << "P6\n" << screenshot_source->width << ' ' << screenshot_source->height << "\n255\n";
    output.write(reinterpret_cast<const char*>(rgb.data()), static_cast<std::streamsize>(rgb.size()));
    if (!output.good()) {
        LOG_WARNING("Failed to write screenshot file: " << screenshot_path.string());
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(screenshot_mutex_);
        last_screenshot_path_ = screenshot_path.string();
        screenshot_path_pending_ = true;
    }
    LOG_INFO("Screenshot saved: " << screenshot_path.string());
    return true;
}

bool PlayerCore::consumeQuitRequest() {
    return quit_requested_.exchange(false);
}

bool PlayerCore::consumeNextItemRequest() {
    return next_item_requested_.exchange(false);
}

bool PlayerCore::consumePreviousItemRequest() {
    return previous_item_requested_.exchange(false);
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

DiagnosticsSnapshot PlayerCore::getDiagnosticsSnapshot() const {
    DiagnosticsSnapshot snapshot;
    snapshot.demux_video_packets = demux_video_packets_.load();
    snapshot.demux_audio_packets = demux_audio_packets_.load();
    snapshot.demux_push_retries = demux_push_retries_.load();
    snapshot.demux_dropped_packets = demux_dropped_packets_.load();
    snapshot.decode_video_ok = decode_video_ok_.load();
    snapshot.decode_audio_ok = decode_audio_ok_.load();
    snapshot.audio_submitted_frames = audio_submitted_frames_.load();
    snapshot.render_frames = render_frames_.load();

    const SchedulerStats scheduler_stats = scheduler_.getStats();
    snapshot.scheduler_video_decoded_frames = scheduler_stats.video_decoded_frames;
    snapshot.scheduler_audio_decoded_frames = scheduler_stats.audio_decoded_frames;
    snapshot.scheduler_late_drops = scheduler_stats.dropped_late_frames;
    snapshot.video_packet_queue_size = video_packet_queue_ ? video_packet_queue_->size() : 0;
    snapshot.audio_packet_queue_size = audio_packet_queue_ ? audio_packet_queue_->size() : 0;
    snapshot.video_frame_queue_size = video_queue_.size();
    snapshot.audio_frame_queue_size = audio_queue_.size();
    return snapshot;
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

void PlayerCore::setAudioDelay(double delay_seconds) {
    const double clamped = std::max(-kMaxMediaDelaySeconds, std::min(kMaxMediaDelaySeconds, delay_seconds));
    const double previous = audio_delay_seconds_.exchange(clamped);
    if (std::abs(previous - clamped) >= 1e-9) {
        LOG_INFO("Audio delay set to " << std::lround(clamped * 1000.0) << " ms");
    }
}

double PlayerCore::getAudioDelay() const {
    return audio_delay_seconds_.load();
}

void PlayerCore::setSubtitleDelay(double delay_seconds) {
    const double clamped = std::max(-kMaxMediaDelaySeconds, std::min(kMaxMediaDelaySeconds, delay_seconds));
    const double previous = subtitle_delay_seconds_.exchange(clamped);
    if (std::abs(previous - clamped) >= 1e-9) {
        updateSubtitleOverlay(position_.load());
        LOG_INFO("Subtitle delay set to " << std::lround(clamped * 1000.0) << " ms");
    }
}

double PlayerCore::getSubtitleDelay() const {
    return subtitle_delay_seconds_.load();
}

void PlayerCore::setPreferHardwareDecode(bool prefer_hardware_decode) {
    prefer_hardware_decode_.store(prefer_hardware_decode);
}

bool PlayerCore::preferHardwareDecode() const {
    return prefer_hardware_decode_.load();
}

decoder::DecoderBackend PlayerCore::videoDecoderBackend() const {
    return video_decoder_backend_;
}

std::string PlayerCore::videoRendererBackendName() const {
    if (!video_renderer_) {
        return "None";
    }
    const char* name = video_renderer_->rendererBackendName();
    if (!name || name[0] == '\0') {
        return "Unknown";
    }
    return name;
}

void PlayerCore::setExternalSubtitles(std::vector<subtitle::SubtitleItem> subtitles, const std::string& source_path) {
    std::sort(subtitles.begin(), subtitles.end(), [](const subtitle::SubtitleItem& lhs, const subtitle::SubtitleItem& rhs) {
        if (lhs.start_seconds != rhs.start_seconds) {
            return lhs.start_seconds < rhs.start_seconds;
        }
        if (lhs.layer != rhs.layer) {
            return lhs.layer < rhs.layer;
        }
        if (lhs.end_seconds != rhs.end_seconds) {
            return lhs.end_seconds < rhs.end_seconds;
        }
        return lhs.index < rhs.index;
    });

    size_t subtitle_count = 0;
    std::string subtitle_path;
    {
        std::lock_guard<std::mutex> lock(subtitle_mutex_);
        subtitle_items_ = std::move(subtitles);
        subtitle_source_path_ = source_path;
        subtitle_active_indices_.clear();
        subtitle_count = subtitle_items_.size();
        subtitle_path = subtitle_source_path_;
    }

    if (video_renderer_) {
        video_renderer_->setSubtitleItems({});
    }
    LOG_INFO("Subtitle track attached: path=" << subtitle_path << ", entries=" << subtitle_count);
}

void PlayerCore::clearExternalSubtitles() {
    {
        std::lock_guard<std::mutex> lock(subtitle_mutex_);
        subtitle_items_.clear();
        subtitle_source_path_.clear();
        subtitle_active_indices_.clear();
    }

    if (video_renderer_) {
        video_renderer_->setSubtitleItems({});
    }
}

bool PlayerCore::hasExternalSubtitles() const {
    std::lock_guard<std::mutex> lock(subtitle_mutex_);
    return !subtitle_items_.empty();
}

void PlayerCore::setSubtitleEnabled(bool enabled) {
    bool changed = false;
    bool has_subtitle_track = false;
    {
        std::lock_guard<std::mutex> lock(subtitle_mutex_);
        has_subtitle_track = !subtitle_items_.empty();
        if (subtitle_enabled_ == enabled) {
            return;
        }
        subtitle_enabled_ = enabled;
        subtitle_active_indices_.clear();
        changed = true;
    }

    if (!changed) {
        return;
    }

    if (video_renderer_) {
        if (!enabled) {
            video_renderer_->setSubtitleItems({});
        } else {
            updateSubtitleOverlay(position_.load());
        }
    }

    LOG_INFO("Subtitle display " << (enabled ? "enabled" : "disabled"));
    if (enabled && !has_subtitle_track) {
        LOG_WARNING("Subtitle display enabled, but no subtitle track is loaded");
    }
}

bool PlayerCore::isSubtitleEnabled() const {
    std::lock_guard<std::mutex> lock(subtitle_mutex_);
    return subtitle_enabled_;
}

bool PlayerCore::toggleSubtitleEnabled() {
    const bool next_enabled = !isSubtitleEnabled();
    setSubtitleEnabled(next_enabled);
    return next_enabled;
}

void PlayerCore::setHotkeyManager(const input::HotkeyManager& hotkey_manager) {
    hotkey_manager_ = hotkey_manager;
    if (video_renderer_) {
        video_renderer_->setHotkeyManager(hotkey_manager_);
    }
}

const input::HotkeyManager& PlayerCore::hotkeyManager() const {
    return hotkey_manager_;
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

/// 鍒濆鍖栬棰?闊抽瑙ｇ爜鍣紝骞舵牴鎹厤缃€夋嫨纭В鎴栬蒋瑙ｅ洖閫€椤哄簭銆?
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

        auto reset_video_hw_binding = [&]() {
            if (video_codec_ctx_) {
                if (video_codec_ctx_->hw_device_ctx) {
                    av_buffer_unref(&video_codec_ctx_->hw_device_ctx);
                }
                video_codec_ctx_->get_format = nullptr;
                video_codec_ctx_->opaque = nullptr;
            }
            if (video_hw_device_ctx_) {
                av_buffer_unref(&video_hw_device_ctx_);
            }
            video_hw_pixel_fmt_ = AV_PIX_FMT_NONE;
            video_decoder_backend_ = decoder::DecoderBackend::Software;
        };

        auto try_open_video_decoder_with_backend = [&](decoder::DecoderBackend backend) -> bool {
            if (video_codec_ctx_) {
                avcodec_free_context(&video_codec_ctx_);
                video_codec_ctx_ = nullptr;
            }

            video_codec_ctx_ = avcodec_alloc_context3(codec);
            if (!video_codec_ctx_ || !configure_video_codec_ctx(video_codec_ctx_)) {
                return false;
            }

            reset_video_hw_binding();

            bool backend_ready = false;
            switch (backend) {
            case decoder::DecoderBackend::Software:
                backend_ready = true;
                break;
            case decoder::DecoderBackend::D3D11VA:
                backend_ready = tryConfigureD3D11HardwareDecode(codec, video_codec_ctx_);
                break;
            default:
                LOG_WARNING("Unsupported decoder backend candidate: "
                            << decoder::DecoderFactory::backendName(backend));
                backend_ready = false;
                break;
            }

            if (!backend_ready) {
                reset_video_hw_binding();
                return false;
            }

            if (avcodec_open2(video_codec_ctx_, codec, nullptr) < 0) {
                LOG_WARNING("avcodec_open2 failed for backend "
                            << decoder::DecoderFactory::backendName(backend)
                            << ", retrying next candidate");
                reset_video_hw_binding();
                return false;
            }

            if (backend == decoder::DecoderBackend::D3D11VA &&
                video_decoder_backend_ != decoder::DecoderBackend::D3D11VA) {
                LOG_WARNING("D3D11VA decoder initialization downgraded to software backend during format negotiation");
            }

            LOG_INFO("Video decoder backend: " << decoder::DecoderFactory::backendName(video_decoder_backend_));
            return true;
        };

        const std::string codec_name = codec->name ? codec->name : "unknown";
        const std::vector<decoder::DecoderBackend> backend_order =
            decoder::DecoderFactory::selectBackendOrder(codec_name, preferHardwareDecode());
        LOG_INFO("Video decoder backend candidates for codec " << codec_name
                 << ": " << backendOrderToString(backend_order));

        bool opened_video_decoder = false;
        for (decoder::DecoderBackend backend : backend_order) {
            if (try_open_video_decoder_with_backend(backend)) {
                opened_video_decoder = true;
                break;
            }
        }
        if (!opened_video_decoder) {
            reset_video_hw_binding();
            if (video_codec_ctx_) {
                avcodec_free_context(&video_codec_ctx_);
                video_codec_ctx_ = nullptr;
            }
            return false;
        }

        if (video_decoder_backend_ == decoder::DecoderBackend::Software && !preferHardwareDecode()) {
            LOG_INFO("Hardware decode disabled by config, using software decode backend");
        }

        if (video_codec_ctx_ && video_decoder_backend_ != decoder::DecoderBackend::D3D11VA) {
            if (video_codec_ctx_->hw_device_ctx) {
                av_buffer_unref(&video_codec_ctx_->hw_device_ctx);
                }
            video_codec_ctx_->get_format = nullptr;
            video_codec_ctx_->opaque = nullptr;
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

/// 閲婃斁瑙ｇ爜鍣ㄣ€佺‖浠惰澶囦笂涓嬫枃浠ュ強闊宠棰戞牸寮忚浆鎹㈠櫒銆?
void PlayerCore::releaseDecoders() {
    std::scoped_lock codec_lock(video_codec_mutex_, audio_codec_mutex_);
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

/// 鍦?Windows 涓婁负瑙嗛瑙ｇ爜鍣ㄧ粦瀹?D3D11VA 璁惧锛涘け璐ユ椂鍥為€€杞В銆?
bool PlayerCore::tryConfigureD3D11HardwareDecode(const AVCodec* codec, AVCodecContext* codec_ctx) {
#if defined(_WIN32)
    if (!codec || !codec_ctx) {
        return false;
    }

    const std::string codec_name = codec->name ? codec->name : "";

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

    void* native_device_handle = video_renderer_ ? video_renderer_->nativeDeviceHandle() : nullptr;
    if (native_device_handle) {
        video_hw_device_ctx_ = av_hwdevice_ctx_alloc(AV_HWDEVICE_TYPE_D3D11VA);
        if (!video_hw_device_ctx_) {
            LOG_WARNING("Failed to allocate shared D3D11VA device context, fallback to software decode");
            return false;
        }

        auto* device_ctx = reinterpret_cast<AVHWDeviceContext*>(video_hw_device_ctx_->data);
        auto* d3d11_ctx = reinterpret_cast<AVD3D11VADeviceContext*>(device_ctx->hwctx);
        auto* d3d11_device = static_cast<ID3D11Device*>(native_device_handle);
        d3d11_device->AddRef();
        d3d11_ctx->device = d3d11_device;
        if (av_hwdevice_ctx_init(video_hw_device_ctx_) < 0) {
            LOG_WARNING("Failed to initialize shared D3D11VA device context, fallback to software decode");
            av_buffer_unref(&video_hw_device_ctx_);
            video_hw_pixel_fmt_ = AV_PIX_FMT_NONE;
            return false;
        }
        LOG_INFO("D3D11VA decoder bound to renderer-owned D3D11 device");
    } else if (av_hwdevice_ctx_create(&video_hw_device_ctx_, AV_HWDEVICE_TYPE_D3D11VA, nullptr, nullptr, 0) < 0 ||
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
            LOG_WARNING("Requested D3D11VA pixel format not offered by decoder, fallback to software decode backend");
            self->video_hw_pixel_fmt_ = AV_PIX_FMT_NONE;
            self->video_decoder_backend_ = decoder::DecoderBackend::Software;
        }
    }

    return pix_fmts[0];
}

/// 灏嗚В鐮佸抚鏁寸悊涓烘覆鏌撹矾寰勫彲鐩存帴娑堣垂鐨?`YUV420P` 杈撳嚭甯с€?
bool PlayerCore::prepareVideoOutputFrame(AVFrame* decoded_frame, VideoFrame& out) {
    if (!decoded_frame || !out.frame) {
        return false;
    }

    const AVPixelFormat decoded_format = static_cast<AVPixelFormat>(decoded_frame->format);
    const bool renderer_accepts_native =
        video_renderer_ &&
        video_renderer_->supportsNativeFrameFormat(decoded_format) &&
        !filter_pipeline_.hasEnabledVideoFilters();
    if (renderer_accepts_native) {
        return true;
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

/// 鎸夊綋鍓嶆簮甯у昂瀵稿拰鍍忕礌鏍煎紡鍑嗗 `swscale` 涓婁笅鏂囥€?
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

/// 鎶婁换鎰忚緭鍏ュ儚绱犳牸寮忚浆鎹负娓叉煋閾捐矾缁熶竴浣跨敤鐨?`YUV420P`銆?
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

/// 纭繚闊抽閲嶉噰鏍峰櫒杈撳嚭鏍煎紡涓庡綋鍓?SDL 璁惧閰嶇疆涓€鑷淬€?
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

/// 閲婃斁闊抽閲嶉噰鏍蜂笂涓嬫枃锛屽苟娓呯┖杈撳叆杈撳嚭甯冨眬缂撳瓨銆?
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

/// 鍚姩瑙ｅ鐢ㄧ嚎绋嬶紱璐熻矗璇诲寘銆佸垎鍙戝埌闊宠棰戝寘闃熷垪骞朵紶鎾?EOF銆?
void PlayerCore::startDemuxThread() {
    if (demux_running_.exchange(true)) {
        return;
    }
    demux_thread_ = std::thread([this] {
        // Demux only reads and dispatches packets; EOF is propagated downstream explicitly.
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
                // On successful push the queue owns the packet lifetime.
                while (demux_running_.load() && !queued) {
                    queued = video_packet_queue_->push(packet, 20);
                    if (queued) {
                        break;
                    }
                    demux_push_retries_.fetch_add(1);
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
                if (queued) {
                    demux_video_packets_.fetch_add(1);
                }
            } else if (packet->stream_index == info.audio_stream_idx && audio_packet_queue_) {
                while (demux_running_.load() && !queued) {
                    queued = audio_packet_queue_->push(packet, 20);
                    if (queued) {
                        break;
                    }
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

/// 鍋滄瑙ｅ鐢ㄧ嚎绋嬶紝骞堕噸缃寘闃熷垪鐨勫仠姝㈢姸鎬佷互渚垮悗缁鐢ㄣ€?
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

/// 鍚姩闊抽娑堣垂绾跨▼锛涙妸瑙ｇ爜 PCM 鎻愪氦缁?`AudioPlayer` 骞舵帹杩涢煶棰戜富鏃堕挓銆?
void PlayerCore::startAudioConsumer() {
    if (!audio_player_ || !audio_player_->isInitialized() || audio_consumer_running_.exchange(true)) {
        return;
    }
    audio_consumer_thread_ = std::thread([this] {
        while (audio_consumer_running_.load()) {
            AudioFrame frame;
            const double played_pts = audio_player_->getPlaybackPts();
            if (played_pts > 0.0 && state_.load() == PlaybackState::Playing) {
                // Use device playback progress as the audio master clock.
                const double audio_delay = audio_delay_seconds_.load();
                const double content_pts = std::max(0.0, played_pts - audio_delay);
                clock_.setAudioClock(content_pts);
                position_.store(content_pts);
                emitPositionChanged(content_pts);
            }

            if (audio_player_->getBufferedSeconds() > kMaxAudioBufferedSeconds) {
                // Apply backpressure when the audio device buffer gets too deep.
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
                maybeLogDiagnostics("audio-backpressure");
                continue;
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
            const double audio_delay = audio_delay_seconds_.load();
            const double delayed_pts = std::max(0.0, frame.pts + audio_delay);
            audio_player_->play(frame.samples, delayed_pts);
            audio_submitted_frames_.fetch_add(1);
            maybeLogDiagnostics("audio-play");
        }
    });
}

/// 鍋滄闊抽娑堣垂绾跨▼骞剁瓑寰呴€€鍑恒€?
void PlayerCore::stopAudioConsumer() {
    audio_consumer_running_.store(false);
    if (audio_consumer_thread_.joinable()) {
        audio_consumer_thread_.join();
    }
}

/// 娓呯┖璋冨害甯ч槦鍒楀拰鍘嬬缉鍖呴槦鍒楋紝涓㈠純褰撳墠鏃堕棿绾夸笂鐨勬畫鐣欐暟鎹€?
void PlayerCore::flushPipelines() {
    scheduler_.flush();
    if (video_packet_queue_) {
        video_packet_queue_->clear();
    }
    if (audio_packet_queue_) {
        audio_packet_queue_->clear();
    }
}

/// 閲嶇疆閾捐矾璇婃柇璁℃暟鍣紝涓烘柊涓€杞挱鏀炬垨 seek 瑙傚療绐楀彛鍋氬噯澶囥€?
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

/// 鑺傛祦杈撳嚭閾捐矾璇婃柇鏃ュ織锛屼究浜庤瀵?demux/decoder/render/audio 鍋ュ悍搴︺€?
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

/// 浠庤棰戝寘闃熷垪鍜岃В鐮佸櫒鍐呴儴缂撳瓨鎻愬彇涓€甯э紝骞惰鑼冨寲鍒版覆鏌撹緭鍑烘牸寮忋€?
bool PlayerCore::decodeVideoFrame(VideoFrame& out) {
    if (!video_codec_ctx_ || !video_packet_queue_) {
        return false;
    }

    std::lock_guard<std::mutex> codec_lock(video_codec_mutex_);
    av_frame_unref(out.frame);
    int ret = avcodec_receive_frame(video_codec_ctx_, out.frame);
    if (ret >= 0) {
        // Drain already-produced decoder frames before asking for more input.
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
    // Only send a new packet when receive needs more input.
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

/// 浠庨煶棰戝寘闃熷垪瑙ｇ爜涓€甯?PCM锛屽苟閲嶉噰鏍峰埌 SDL 杈撳嚭鏍煎紡銆?
bool PlayerCore::decodeAudioFrame(AudioFrame& out) {
    if (!audio_codec_ctx_ || !audio_packet_queue_ || !audio_player_ || !audio_player_->isInitialized()) {
        return false;
    }

    std::lock_guard<std::mutex> codec_lock(audio_codec_mutex_);
    AVFrame* frame = av_frame_alloc();
    if (!frame) {
        return false;
    }

    int ret = avcodec_receive_frame(audio_codec_ctx_, frame);
    if (ret == AVERROR(EAGAIN)) {
        // Mirror the video path: receive first, then feed more compressed data if needed.
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

    // Resample/convert to the SDL output format expected by AudioPlayer.
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
    // Prefer decoder timing metadata, and fall back to sample math when needed.
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

/// 鎻愪氦涓€甯ц棰戝埌娓叉煋鍣紝骞跺悓姝ュ瓧骞曘€丱SD銆佹埅鍥惧拰瑙嗛鏃堕挓銆?
void PlayerCore::renderFrame(VideoFrame&& frame) {
    if (!video_renderer_ || !frame.valid || !frame.frame) {
        return;
    }
    const double duration = demuxer_ ? demuxer_->getMediaInfo().duration : 0.0;
    // Maintain a fixed renderer order: subtitle state, overlay state, filters, submit, present.
    updateSubtitleOverlay(frame.pts);
    video_renderer_->setOverlayState(position_.load(), duration, volume_.load(), state_.load() == PlaybackState::Paused);
    if (!isHardwarePixelFormat(frame.getFormat())) {
        filter_pipeline_.processVideo(frame);
    }
    video_renderer_->renderFrame(frame);
    video_renderer_->present();
    updateLastRenderedFrame(frame);
    if (frame.duration > 0.0) {
        last_video_frame_duration_.store(frame.duration);
    }
    if (screenshot_requested_.exchange(false)) {
        if (!captureScreenshotFromCachedFrame()) {
            LOG_WARNING("Screenshot request failed");
        }
    }

    clock_.setVideoClock(frame.pts);
    render_frames_.fetch_add(1);
    // When audio is not the master clock, drive position from video PTS.
    if (clock_.getSource() != ClockSource::Audio) {
        position_.store(frame.pts);
        emitPositionChanged(frame.pts);
    }
    maybeLogDiagnostics("render");
    emitFrameRendered();
}

/// 鍦ㄦ覆鏌撶┖闂叉湡妫€鏌?EOF 鏀跺熬鏉′欢锛屽繀瑕佹椂鑷姩鍒囧埌鍋滄鎬併€?
void PlayerCore::onRenderIdle() {
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

/// 鏍规嵁褰撳墠浣嶇疆鍜屽瓧骞曞欢杩熻绠楀綋鍓嶅簲鏄剧ず鐨勫瓧骞曟枃鏈€?
void PlayerCore::updateSubtitleOverlay(double position_seconds) {
    if (!video_renderer_) {
        return;
    }

    const double adjusted_position = std::max(0.0, position_seconds - subtitle_delay_seconds_.load());

    std::vector<subtitle::SubtitleItem> active_items;
    bool subtitle_changed = false;
    {
        std::lock_guard<std::mutex> lock(subtitle_mutex_);
        std::vector<int> next_active_indices;
        if (subtitle_enabled_ && !subtitle_items_.empty()) {
            next_active_indices = subtitle::collectActiveSubtitleIndices(subtitle_items_, adjusted_position);
            if (next_active_indices != subtitle_active_indices_) {
                subtitle_active_indices_ = next_active_indices;
                subtitle_changed = true;
            }
            if (subtitle_changed) {
                active_items.reserve(subtitle_active_indices_.size());
                for (int index : subtitle_active_indices_) {
                    if (index >= 0 && index < static_cast<int>(subtitle_items_.size())) {
                        active_items.push_back(subtitle_items_[static_cast<size_t>(index)]);
                    }
                }
            }
        } else if (!subtitle_active_indices_.empty()) {
            subtitle_active_indices_.clear();
            subtitle_changed = true;
        }
    }

    if (subtitle_changed) {
        video_renderer_->setSubtitleItems(active_items);
    }
}

/// 鍚戝鍒嗗彂鐘舵€佸洖璋冿紱鍏堝鍒跺洖璋冨垪琛ㄤ互闄嶄綆鎸侀攣鏃堕棿銆?
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

/// 鑺傛祦鍒嗗彂浣嶇疆鍥炶皟锛岄伩鍏嶉珮棰戞覆鏌撴帹杩涚洿鎺ュ帇鍨?UI 灞傘€?
void PlayerCore::emitPositionChanged(double position) {
    const auto now = std::chrono::steady_clock::now();
    // Throttle position callbacks to avoid hammering the UI thread.
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

/// 骞挎挱涓€娆″抚娓叉煋瀹屾垚浜嬩欢銆?
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

/// 璁板綍閿欒鏃ュ織骞跺悜澶栧箍鎾敊璇爜涓庤鏄庢枃鏈€?
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














