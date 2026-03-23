#include "core/player_core.h"



#include <algorithm>

#include <cctype>

#include <cerrno>

#include <chrono>

#include <cmath>

#include <cstdlib>

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

#include <libavutil/error.h>

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

constexpr size_t kMinVideoFrameQueueCapacity = 16;

constexpr size_t kMaxVideoFrameQueueCapacitySoftware = 24;

constexpr size_t kMaxVideoFrameQueueCapacityHardware = 24;

constexpr size_t kMinAudioFrameQueueCapacity = 32;

constexpr size_t kMaxAudioFrameQueueCapacity = 48;

constexpr int kVideoSendPacketRetNotSet = std::numeric_limits<int>::min();



int64_t nowSteadyMs() {

    return std::chrono::duration_cast<std::chrono::milliseconds>(

               std::chrono::steady_clock::now().time_since_epoch())

        .count();

}



template <typename T>

T clampValue(T value, T min_value, T max_value) {

    return std::max(min_value, std::min(max_value, value));

}



void updateMaxAtomic(std::atomic<uint64_t>& target, uint64_t value) {

    uint64_t current = target.load();

    while (value > current && !target.compare_exchange_weak(current, value)) {

    }

}



bool envFlagEnabled(const char* key) {

    if (!key || key[0] == '\0') {

        return false;

    }



#if defined(_WIN32)

    char* raw_value = nullptr;

    size_t raw_len = 0;

    if (_dupenv_s(&raw_value, &raw_len, key) != 0 || !raw_value) {

        return false;

    }

    std::string normalized(raw_value);

    std::free(raw_value);

#else

    const char* value = std::getenv(key);

    if (!value) {

        return false;

    }

    std::string normalized(value);

#endif



    std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char ch) {

        return static_cast<char>(std::tolower(ch));

    });

    normalized.erase(

        std::remove_if(normalized.begin(), normalized.end(), [](unsigned char ch) { return std::isspace(ch) != 0; }),

        normalized.end());

    return normalized == "1" || normalized == "true" || normalized == "yes" || normalized == "on";

}



size_t selectVideoFrameQueueCapacity(const MediaInfo& info, decoder::DecoderBackend backend) {

    const double fps = info.fps > 1.0 ? info.fps : 30.0;

    const int64_t pixel_count = static_cast<int64_t>(std::max(0, info.width)) *

                                static_cast<int64_t>(std::max(0, info.height));



    double target_buffer_seconds = 0.28;

    if (fps >= 50.0) {

        target_buffer_seconds += 0.14;

    }

    if (pixel_count >= 1920LL * 1080LL) {

        target_buffer_seconds += 0.10;

    }

    if (pixel_count >= 3840LL * 2160LL) {

        target_buffer_seconds += 0.08;

    }



    const size_t requested = static_cast<size_t>(std::ceil(fps * target_buffer_seconds));

    const size_t max_capacity =

        backend == decoder::DecoderBackend::D3D11VA ? kMaxVideoFrameQueueCapacityHardware

                                                    : kMaxVideoFrameQueueCapacitySoftware;

    return clampValue<size_t>(requested, kMinVideoFrameQueueCapacity, max_capacity);

}



size_t selectAudioFrameQueueCapacity(const MediaInfo& info) {

    const double sample_rate = info.sample_rate > 0 ? static_cast<double>(info.sample_rate) : 48000.0;

    double target_buffer_seconds = 0.72;

    if (info.channels >= 6) {

        target_buffer_seconds += 0.10;

    }

    if (info.fps >= 50.0) {

        target_buffer_seconds += 0.05;

    }



    const double estimated_frames_per_second = sample_rate / 1024.0;

    const size_t requested = static_cast<size_t>(std::ceil(estimated_frames_per_second * target_buffer_seconds));

    return clampValue<size_t>(requested, kMinAudioFrameQueueCapacity, kMaxAudioFrameQueueCapacity);

}



std::string codecThreadTypeToString(int thread_type) {

    if (thread_type == 0) {

        return "none";

    }



    std::string name;

    if ((thread_type & FF_THREAD_SLICE) != 0) {

        name += "slice";

    }

    if ((thread_type & FF_THREAD_FRAME) != 0) {

        if (!name.empty()) {

            name += "+";

        }

        name += "frame";

    }

    return name.empty() ? "unknown" : name;

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



const char* clockSourceName(ClockSource source) {

    switch (source) {

    case ClockSource::Audio:

        return "Audio";

    case ClockSource::Video:

        return "Video";

    case ClockSource::System:

        return "System";

    default:

        return "Unknown";

    }

}



const char* endedReasonName(EndedReason reason) {

    switch (reason) {

    case EndedReason::None:

        return "None";

    case EndedReason::Eof:

        return "Eof";

    default:

        return "Unknown";

    }

}

const char* schedulerClockPolicyName(SchedulerClockPolicy policy) {

    switch (policy) {

    case SchedulerClockPolicy::UseClockSource:

        return "UseClockSource";

    case SchedulerClockPolicy::AudioMaster:

        return "AudioMaster";

    case SchedulerClockPolicy::VideoMaster:

        return "VideoMaster";

    case SchedulerClockPolicy::SystemMonotonic:

        return "SystemMonotonic";

    default:

        return "Unknown";

    }

}

const char* schedulerAudioMasterPolicyName(SchedulerAudioMasterPolicy policy) {

    switch (policy) {

    case SchedulerAudioMasterPolicy::Disabled:

        return "Disabled";

    case SchedulerAudioMasterPolicy::SoftWhenAudioReady:

        return "SoftWhenAudioReady";

    case SchedulerAudioMasterPolicy::RequireBufferedAudio:

        return "RequireBufferedAudio";

    default:

        return "Unknown";

    }

}

const char* schedulerEndedPolicyName(SchedulerEndedPolicy policy) {

    switch (policy) {

    case SchedulerEndedPolicy::StopOutput:

        return "StopOutput";

    case SchedulerEndedPolicy::HoldLastFrame:

        return "HoldLastFrame";

    case SchedulerEndedPolicy::HoldLastFrameNoClockSync:

        return "HoldLastFrameNoClockSync";

    default:

        return "Unknown";

    }

}



bool isHardwarePixelFormat(AVPixelFormat format) {

    const AVPixFmtDescriptor* desc = av_pix_fmt_desc_get(format);

    return desc && (desc->flags & AV_PIX_FMT_FLAG_HWACCEL) != 0;

}



std::string avErrorToString(int error_code) {

    char buffer[AV_ERROR_MAX_STRING_SIZE] = {};

    if (av_strerror(error_code, buffer, sizeof(buffer)) == 0) {

        return std::string(buffer);

    }

    return "unknown";

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



/// 构造播放核心，并绑定调度器、队列、滤镜和默认回调。

PlayerCore::PlayerCore() {

    filters::builtin::registerBuiltinFilters();

    scheduler_.setVideoQueue(&video_queue_);

    scheduler_.setAudioQueue(&audio_queue_);

    scheduler_.setClock(&clock_);

    scheduler_.setVideoDecoder([this](VideoFrame& frame) { return decodeVideoFrame(frame); });

    scheduler_.setAudioDecoder([this](AudioFrame& frame) { return decodeAudioFrame(frame); });

    scheduler_.setRenderCallback([this](VideoFrame&& frame) { return renderFrame(std::move(frame)); });

    scheduler_.setIdleCallback([this] { onRenderIdle(); });

    scheduler_.setControlSnapshotProvider([this] { return makeSchedulerControlSnapshot(); });

    last_position_emit_tp_ = std::chrono::steady_clock::now();

    resetDiagnostics();

}



PlayerCore::~PlayerCore() {

    close();

}



PlayerCore::CoreStateSnapshot PlayerCore::getCoreStateSnapshot() const {

    std::lock_guard<std::mutex> lock(core_state_mutex_);

    return core_state_;

}



SchedulerControlSnapshot PlayerCore::makeSchedulerControlSnapshot() const {

    const CoreStateSnapshot snapshot = getCoreStateSnapshot();

    SchedulerControlSnapshot control;



    switch (snapshot.run_state) {

    case RunState::Stopped:

        control.run_state = SchedulerRunState::Stopped;

        break;

    case RunState::Starting:

        control.run_state = SchedulerRunState::Starting;

        break;

    case RunState::Running:

        control.run_state = SchedulerRunState::Running;

        break;

    case RunState::Pausing:

        control.run_state = SchedulerRunState::Pausing;

        break;

    case RunState::Paused:

        control.run_state = SchedulerRunState::Paused;

        break;

    case RunState::Stopping:

        control.run_state = SchedulerRunState::Stopping;

        break;

    case RunState::Ended:

        control.run_state = SchedulerRunState::Ended;

        break;

    }



    switch (snapshot.pipeline_phase) {

    case PipelinePhase::Idle:

        control.pipeline_phase = SchedulerPipelinePhase::Idle;

        break;

    case PipelinePhase::Normal:

        control.pipeline_phase = SchedulerPipelinePhase::Normal;

        break;

    case PipelinePhase::Seeking:

        control.pipeline_phase = SchedulerPipelinePhase::Seeking;

        break;

    case PipelinePhase::Draining:

        control.pipeline_phase = SchedulerPipelinePhase::Draining;

        break;

    case PipelinePhase::Flushing:

        control.pipeline_phase = SchedulerPipelinePhase::Flushing;

        break;

    }



    if (snapshot.pending_seek && snapshot.pending_seek_serial != kInvalidTimelineSerial) {

        control.accepted_timeline_serial = snapshot.pending_seek_serial;

    } else {

        control.accepted_timeline_serial = snapshot.timeline_serial;

    }

    control.clock_source = clock_.getSource();

    switch (control.clock_source) {

    case ClockSource::Audio:

        control.clock_policy = SchedulerClockPolicy::AudioMaster;

        break;

    case ClockSource::Video:

        control.clock_policy = SchedulerClockPolicy::VideoMaster;

        break;

    case ClockSource::System:

        control.clock_policy = SchedulerClockPolicy::SystemMonotonic;

        break;

    default:

        control.clock_policy = SchedulerClockPolicy::UseClockSource;

        break;

    }

    control.audio_output_initialized = audio_player_ && audio_player_->isInitialized();

    control.audio_buffered_seconds =
        (audio_player_ && control.audio_output_initialized) ? std::max(0.0, audio_player_->getBufferedSeconds()) : 0.0;

    if (control.clock_policy == SchedulerClockPolicy::AudioMaster) {

        control.audio_master_policy = snapshot.run_state == RunState::Starting
                                          ? SchedulerAudioMasterPolicy::SoftWhenAudioReady
                                          : SchedulerAudioMasterPolicy::RequireBufferedAudio;

    } else {

        control.audio_master_policy = SchedulerAudioMasterPolicy::Disabled;

    }

    control.audio_master_sync_active =
        control.audio_master_policy != SchedulerAudioMasterPolicy::Disabled && control.audio_output_initialized;

    if (snapshot.run_state == RunState::Ended) {

        control.ended_policy = snapshot.ended_reason == EndedReason::Eof
                                   ? SchedulerEndedPolicy::HoldLastFrameNoClockSync
                                   : SchedulerEndedPolicy::HoldLastFrame;

    } else {

        control.ended_policy = SchedulerEndedPolicy::StopOutput;

    }

    return control;

}



bool PlayerCore::transitionSessionState(SessionState next, const char* reason) {

    const auto state_name = [](SessionState state) {

        switch (state) {

        case SessionState::Closed:

            return "Closed";

        case SessionState::Opening:

            return "Opening";

        case SessionState::Ready:

            return "Ready";

        case SessionState::Closing:

            return "Closing";

        case SessionState::Failed:

            return "Failed";

        default:

            return "Unknown";

        }

    };

    const auto can_transition = [](SessionState from, SessionState to) {

        if (from == to) {

            return true;

        }

        switch (from) {

        case SessionState::Closed:

            return to == SessionState::Opening;

        case SessionState::Opening:

            return to == SessionState::Ready || to == SessionState::Closing || to == SessionState::Failed;

        case SessionState::Ready:

            return to == SessionState::Closing || to == SessionState::Failed;

        case SessionState::Closing:

            return to == SessionState::Closed || to == SessionState::Failed;

        case SessionState::Failed:

            return to == SessionState::Closing || to == SessionState::Opening || to == SessionState::Closed;

        default:

            return false;

        }

    };



    SessionState previous = SessionState::Closed;

    {

        std::lock_guard<std::mutex> lock(core_state_mutex_);

        previous = core_state_.session_state;

        if (previous == next) {

            return false;

        }

        if (!can_transition(previous, next)) {

            illegal_session_transitions_.fetch_add(1);

            LOG_WARNING("PlayerCore illegal session transition: " << state_name(previous)

                        << " -> " << state_name(next)

                        << " reason=" << (reason ? reason : "unspecified"));

            return false;

        }

        core_state_.session_state = next;

    }



    LOG_INFO("PlayerCore session_state: " << state_name(previous)

             << " -> " << state_name(next)

             << " reason=" << (reason ? reason : "unspecified"));

    return true;

}



bool PlayerCore::transitionRunState(RunState next, const char* reason) {

    const auto state_name = [](RunState state) {

        switch (state) {

        case RunState::Stopped:

            return "Stopped";

        case RunState::Starting:

            return "Starting";

        case RunState::Running:

            return "Running";

        case RunState::Pausing:

            return "Pausing";

        case RunState::Paused:

            return "Paused";

        case RunState::Stopping:

            return "Stopping";

        case RunState::Ended:

            return "Ended";

        default:

            return "Unknown";

        }

    };

    const auto can_transition = [](RunState from, RunState to) {

        if (from == to) {

            return true;

        }

        switch (from) {

        case RunState::Stopped:

            return to == RunState::Starting || to == RunState::Stopping;

        case RunState::Starting:

            return to == RunState::Running || to == RunState::Stopping || to == RunState::Stopped;

        case RunState::Running:

            return to == RunState::Pausing || to == RunState::Stopping || to == RunState::Ended;

        case RunState::Pausing:

            return to == RunState::Paused || to == RunState::Running || to == RunState::Stopping;

        case RunState::Paused:

            return to == RunState::Running || to == RunState::Stopping || to == RunState::Ended;

        case RunState::Stopping:

            return to == RunState::Stopped;

        case RunState::Ended:

            return to == RunState::Starting || to == RunState::Stopping || to == RunState::Stopped;

        default:

            return false;

        }

    };



    RunState previous = RunState::Stopped;

    {

        std::lock_guard<std::mutex> lock(core_state_mutex_);

        previous = core_state_.run_state;

        if (previous == next) {

            return false;

        }

        if (!can_transition(previous, next)) {

            illegal_run_transitions_.fetch_add(1);

            LOG_WARNING("PlayerCore illegal run transition: " << state_name(previous)

                        << " -> " << state_name(next)

                        << " reason=" << (reason ? reason : "unspecified"));

            return false;

        }

        core_state_.run_state = next;

    }



    LOG_INFO("PlayerCore run_state: " << state_name(previous)

             << " -> " << state_name(next)

             << " reason=" << (reason ? reason : "unspecified"));

    return true;

}



bool PlayerCore::transitionPipelinePhase(PipelinePhase next, const char* reason) {

    const auto phase_name = [](PipelinePhase phase) {

        switch (phase) {

        case PipelinePhase::Idle:

            return "Idle";

        case PipelinePhase::Normal:

            return "Normal";

        case PipelinePhase::Seeking:

            return "Seeking";

        case PipelinePhase::Draining:

            return "Draining";

        case PipelinePhase::Flushing:

            return "Flushing";

        default:

            return "Unknown";

        }

    };

    const auto can_transition = [](PipelinePhase from, PipelinePhase to) {

        if (from == to) {

            return true;

        }

        switch (from) {

        case PipelinePhase::Idle:

            return to == PipelinePhase::Normal || to == PipelinePhase::Seeking || to == PipelinePhase::Flushing;

        case PipelinePhase::Normal:

            return to == PipelinePhase::Idle || to == PipelinePhase::Seeking || to == PipelinePhase::Draining ||

                   to == PipelinePhase::Flushing;

        case PipelinePhase::Seeking:

            return to == PipelinePhase::Idle || to == PipelinePhase::Normal || to == PipelinePhase::Flushing;

        case PipelinePhase::Draining:

            return to == PipelinePhase::Idle || to == PipelinePhase::Normal ||
                   to == PipelinePhase::Seeking || to == PipelinePhase::Flushing;

        case PipelinePhase::Flushing:

            return to == PipelinePhase::Idle || to == PipelinePhase::Normal || to == PipelinePhase::Seeking;

        default:

            return false;

        }

    };



    PipelinePhase previous = PipelinePhase::Idle;

    {

        std::lock_guard<std::mutex> lock(core_state_mutex_);

        previous = core_state_.pipeline_phase;

        if (previous == next) {

            return false;

        }

        if (!can_transition(previous, next)) {

            illegal_pipeline_transitions_.fetch_add(1);

            LOG_WARNING("PlayerCore illegal pipeline transition: " << phase_name(previous)

                        << " -> " << phase_name(next)

                        << " reason=" << (reason ? reason : "unspecified"));

            return false;

        }

        core_state_.pipeline_phase = next;

    }



    LOG_INFO("PlayerCore pipeline_phase: " << phase_name(previous)

             << " -> " << phase_name(next)

             << " reason=" << (reason ? reason : "unspecified"));

    return true;

}



TimelineSerial PlayerCore::allocateNextTimelineSerial(const char* reason) {

    const TimelineSerial serial = next_timeline_serial_.fetch_add(1);

    LOG_INFO("PlayerCore timeline_serial allocated: " << serial

             << " reason=" << (reason ? reason : "unspecified"));

    return serial;

}



void PlayerCore::activateTimelineSerial(TimelineSerial serial, const char* reason) {

    TimelineSerial previous = kInvalidTimelineSerial;

    TimelineSerial previous_pending = kInvalidTimelineSerial;

    {

        std::lock_guard<std::mutex> lock(core_state_mutex_);

        previous = core_state_.timeline_serial;

        previous_pending = core_state_.pending_seek_serial;

        if (previous == serial && previous_pending != serial) {

            return;

        }

        core_state_.timeline_serial = serial;

        if (core_state_.pending_seek_serial == serial) {

            core_state_.pending_seek_serial = kInvalidTimelineSerial;

            core_state_.pending_seek = false;

        }

    }



    audio_output_serial_.store(kInvalidTimelineSerial);

    LOG_INFO("PlayerCore timeline_serial: " << previous

             << " -> " << serial

             << " reason=" << (reason ? reason : "unspecified")

             << " pending_seek_serial_before=" << previous_pending);

}



void PlayerCore::setPendingSeekSerial(TimelineSerial serial, const char* reason) {

    TimelineSerial previous = kInvalidTimelineSerial;

    bool pending_seek = false;

    {

        std::lock_guard<std::mutex> lock(core_state_mutex_);

        previous = core_state_.pending_seek_serial;

        pending_seek = serial != kInvalidTimelineSerial;

        if (previous == serial && core_state_.pending_seek == pending_seek) {

            return;

        }

        core_state_.pending_seek_serial = serial;

        core_state_.pending_seek = pending_seek;

    }



    LOG_INFO("PlayerCore pending_seek_serial: " << previous

             << " -> " << serial

             << " reason=" << (reason ? reason : "unspecified"));

}



void PlayerCore::clearPendingSeekSerial(const char* reason) {

    setPendingSeekSerial(kInvalidTimelineSerial, reason);

}



TimelineSerial PlayerCore::currentTimelineSerial() const {

    return getCoreStateSnapshot().timeline_serial;

}



TimelineSerial PlayerCore::acceptedTimelineSerial() const {

    const CoreStateSnapshot snapshot = getCoreStateSnapshot();

    if (snapshot.pending_seek && snapshot.pending_seek_serial != kInvalidTimelineSerial) {

        return snapshot.pending_seek_serial;

    }

    return snapshot.timeline_serial;

}



bool PlayerCore::isCurrentTimelineSerial(TimelineSerial serial) const {

    return serial != kInvalidTimelineSerial && serial == currentTimelineSerial();

}



bool PlayerCore::isAcceptedTimelineSerial(TimelineSerial serial) const {

    const TimelineSerial accepted = acceptedTimelineSerial();

    return serial != kInvalidTimelineSerial && accepted != kInvalidTimelineSerial && serial == accepted;

}



void PlayerCore::setEofReached(bool eof_reached, const char* reason) {

    bool previous = false;

    {

        std::lock_guard<std::mutex> lock(core_state_mutex_);

        previous = core_state_.eof_reached;

        if (previous == eof_reached) {

            return;

        }

        core_state_.eof_reached = eof_reached;

    }



    LOG_INFO("PlayerCore eof_reached: " << (previous ? "true" : "false")

             << " -> " << (eof_reached ? "true" : "false")

             << " reason=" << (reason ? reason : "unspecified"));

}



void PlayerCore::setDeferredStopPending(bool deferred_stop_pending, const char* reason) {

    bool previous = false;

    {

        std::lock_guard<std::mutex> lock(core_state_mutex_);

        previous = core_state_.deferred_stop_pending;

        if (previous == deferred_stop_pending) {

            return;

        }

        core_state_.deferred_stop_pending = deferred_stop_pending;

    }



    LOG_INFO("PlayerCore deferred_stop_pending: " << (previous ? "true" : "false")

             << " -> " << (deferred_stop_pending ? "true" : "false")

             << " reason=" << (reason ? reason : "unspecified"));

}



void PlayerCore::setEndedReason(EndedReason ended_reason, const char* reason) {

    EndedReason previous = EndedReason::None;

    {

        std::lock_guard<std::mutex> lock(core_state_mutex_);

        previous = core_state_.ended_reason;

        if (previous == ended_reason) {

            return;

        }

        core_state_.ended_reason = ended_reason;

    }



    LOG_INFO("PlayerCore ended_reason: " << endedReasonName(previous)

             << " -> " << endedReasonName(ended_reason)

             << " reason=" << (reason ? reason : "unspecified"));

}



bool PlayerCore::consumeDeferredStopPending(const char* reason) {

    {

        std::lock_guard<std::mutex> lock(core_state_mutex_);

        if (!core_state_.deferred_stop_pending) {

            return false;

        }

        core_state_.deferred_stop_pending = false;

    }



    LOG_INFO("PlayerCore deferred_stop_pending: true -> false"

             << " reason=" << (reason ? reason : "unspecified"));

    return true;

}



void PlayerCore::publishPlaybackStateFromInternalState(const char* reason) {

    const auto playback_state_name = [](PlaybackState state) {

        switch (state) {

        case PlaybackState::Stopped:

            return "Stopped";

        case PlaybackState::Playing:

            return "Playing";

        case PlaybackState::Paused:

            return "Paused";

        default:

            return "Unknown";

        }

    };

    const auto session_state_name = [](SessionState state) {

        switch (state) {

        case SessionState::Closed:

            return "Closed";

        case SessionState::Opening:

            return "Opening";

        case SessionState::Ready:

            return "Ready";

        case SessionState::Closing:

            return "Closing";

        case SessionState::Failed:

            return "Failed";

        default:

            return "Unknown";

        }

    };

    const auto run_state_name = [](RunState state) {

        switch (state) {

        case RunState::Stopped:

            return "Stopped";

        case RunState::Starting:

            return "Starting";

        case RunState::Running:

            return "Running";

        case RunState::Pausing:

            return "Pausing";

        case RunState::Paused:

            return "Paused";

        case RunState::Stopping:

            return "Stopping";

        case RunState::Ended:

            return "Ended";

        default:

            return "Unknown";

        }

    };

    const auto pipeline_phase_name = [](PipelinePhase phase) {

        switch (phase) {

        case PipelinePhase::Idle:

            return "Idle";

        case PipelinePhase::Normal:

            return "Normal";

        case PipelinePhase::Seeking:

            return "Seeking";

        case PipelinePhase::Draining:

            return "Draining";

        case PipelinePhase::Flushing:

            return "Flushing";

        default:

            return "Unknown";

        }

    };

    const auto map_playback_state = [](const CoreStateSnapshot& snapshot) {

        if (snapshot.session_state != SessionState::Ready) {

            return PlaybackState::Stopped;

        }

        switch (snapshot.run_state) {

        case RunState::Starting:

        case RunState::Running:

            return PlaybackState::Playing;

        case RunState::Pausing:

        case RunState::Paused:

            return PlaybackState::Paused;

        case RunState::Stopped:

        case RunState::Stopping:

        case RunState::Ended:

        default:

            return PlaybackState::Stopped;

        }

    };



    const CoreStateSnapshot snapshot = getCoreStateSnapshot();

    const PlaybackState next = map_playback_state(snapshot);

    const PlaybackState previous = state_.exchange(next);

    if (previous == next) {

        return;

    }



    LOG_INFO("PlayerCore playback_state: " << playback_state_name(previous)

             << " -> " << playback_state_name(next)

             << " reason=" << (reason ? reason : "unspecified")

             << " session=" << session_state_name(snapshot.session_state)

             << " run=" << run_state_name(snapshot.run_state)

             << " pipeline=" << pipeline_phase_name(snapshot.pipeline_phase)

             << " eof=" << (snapshot.eof_reached ? "true" : "false")

             << " pending_seek=" << (snapshot.pending_seek ? "true" : "false")

             << " deferred_stop=" << (snapshot.deferred_stop_pending ? "true" : "false")

             << " ended_reason=" << endedReasonName(snapshot.ended_reason)

             << " timeline=" << snapshot.timeline_serial

             << " pending_seek_serial=" << snapshot.pending_seek_serial);

    emitStateChanged(next);

}



/// 打开媒体，并串起渲染器、音频设备、解码器和内部队列资源。

bool PlayerCore::open(const std::string& filename) {

    close();



    transitionSessionState(SessionState::Opening, "open requested");

    transitionRunState(RunState::Stopped, "open requested");

    transitionPipelinePhase(PipelinePhase::Idle, "open requested");

    setEofReached(false, "open requested");

    clearPendingSeekSerial("open requested");

    setDeferredStopPending(false, "open requested");

    setEndedReason(EndedReason::None, "open requested");

    publishPlaybackStateFromInternalState("open requested");



    const auto fail_open = [&](ErrorCode code, const char* message, const char* reason) {

        transitionRunState(RunState::Stopped, reason);

        transitionPipelinePhase(PipelinePhase::Idle, reason);

        setEofReached(false, reason);

        clearPendingSeekSerial(reason);

        setDeferredStopPending(false, reason);

        setEndedReason(EndedReason::None, reason);

        applySessionReleaseSideEffects(reason);

        transitionSessionState(SessionState::Failed, reason);

        publishPlaybackStateFromInternalState(reason);

        emitError(code, message);

        return false;

    };



    demuxer_ = std::make_unique<Demuxer>();

    if (!demuxer_->open(filename)) {

        return fail_open(ErrorCode::FileNotFound, "failed to open media file", "open demuxer failed");

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

                    return fail_open(ErrorCode::DisplayInitFailed,

                                     "failed to initialize video renderer",

                                     "open renderer failed");

                }

            } else {

                return fail_open(ErrorCode::DisplayInitFailed,

                                 "failed to initialize video renderer",

                                 "open renderer failed");

            }

        }



        video_renderer_->setHotkeyManager(hotkey_manager_);

    }



    const bool has_video_stream = info.video_stream_idx >= 0;

    const bool has_audio_stream = info.audio_stream_idx >= 0;

    const bool disable_audio_output = envFlagEnabled("MVP_DISABLE_AUDIO_OUTPUT");



    // Audio output is optional when video playback can continue independently.

    if (has_audio_stream && !disable_audio_output) {

        audio_player_ = std::make_unique<AudioPlayer>();

        if (!audio_player_->init(info.sample_rate, info.channels)) {

            if (!has_video_stream) {

                audio_player_.reset();

                return fail_open(ErrorCode::AudioInitFailed, "failed to initialize audio", "open audio init failed");

            }

            LOG_WARNING("Audio output init failed, continuing with video-only playback");

            audio_player_.reset();

        }

    } else if (has_audio_stream && disable_audio_output) {

        LOG_INFO("Audio output disabled by MVP_DISABLE_AUDIO_OUTPUT, continuing with video-only playback");

    }



    // Decoder init is required for playback; fail fast if it cannot be set up.

    if (!initDecoders()) {

        return fail_open(ErrorCode::DecoderInitFailed, "failed to initialize decoders", "open decoder init failed");

    }



    configureFrameQueues(info);



    // Create packet queues only for streams we can actually decode/output.

    if (has_video_stream) {

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

    const ClockSource clock_source =

        has_audio_clock ? ClockSource::Audio : (has_video_stream ? ClockSource::Video : ClockSource::System);

    clock_.setSource(clock_source);

    activateTimelineSerial(allocateNextTimelineSerial("open ready"), "open ready");

    LOG_INFO("Playback opened with clock source " << clockSourceName(clock_source)

             << " (video_stream=" << (has_video_stream ? "true" : "false")

             << ", audio_stream=" << (has_audio_stream ? "true" : "false")

             << ", audio_output=" << (has_audio_clock ? "true" : "false") << ")");

    transitionSessionState(SessionState::Ready, "open ready");

    transitionRunState(RunState::Stopped, "open ready");

    transitionPipelinePhase(PipelinePhase::Idle, "open ready");

    setEofReached(false, "open ready");

    clearPendingSeekSerial("open ready");

    setDeferredStopPending(false, "open ready");

    setEndedReason(EndedReason::None, "open ready");

    publishPlaybackStateFromInternalState("open ready");

    resetDiagnostics();

    return true;

}



/// 停止全链路线程，并释放解码、渲染、字幕和截图缓存资源。

void PlayerCore::close() {

    const CoreStateSnapshot snapshot = getCoreStateSnapshot();

    if (snapshot.session_state == SessionState::Closed &&

        !opened_.load() &&

        !demuxer_ &&

        !video_renderer_ &&

        !audio_player_ &&

        !video_packet_queue_ &&

        !audio_packet_queue_) {

        return;

    }



    const bool stop_would_early_return = snapshot.run_state == RunState::Stopped &&

                                         !demux_running_.load() &&

                                         !audio_consumer_running_.load() &&

                                         !demux_thread_.joinable() &&

                                         !audio_consumer_thread_.joinable();



    transitionSessionState(SessionState::Closing, "close requested");

    if (stop_would_early_return && snapshot.timeline_serial != kInvalidTimelineSerial) {

        activateTimelineSerial(allocateNextTimelineSerial("close requested"), "close requested");

        clearPendingSeekSerial("close requested");

    }

    setEndedReason(EndedReason::None, "close requested");

    publishPlaybackStateFromInternalState("close requested");

    stop();
    applySessionReleaseSideEffects("close completed");

    setEofReached(false, "close completed");

    clearPendingSeekSerial("close completed");

    setDeferredStopPending(false, "close completed");

    setEndedReason(EndedReason::None, "close completed");

    transitionPipelinePhase(PipelinePhase::Idle, "close completed");

    transitionRunState(RunState::Stopped, "close completed");

    transitionSessionState(SessionState::Closed, "close completed");

    publishPlaybackStateFromInternalState("close completed");

}



/// 从停止态启动线程，或从暂停态恢复调度、时钟和音频设备。

void PlayerCore::play() {

    if (!opened_.load()) {

        return;

    }

    serviceDeferredStop();



    CoreStateSnapshot snapshot = getCoreStateSnapshot();

    if (snapshot.session_state != SessionState::Ready) {

        return;

    }

    if (snapshot.run_state == RunState::Ended) {

        seek(0.0);

        snapshot = getCoreStateSnapshot();

        if (snapshot.session_state != SessionState::Ready) {

            return;

        }

    }

    if (snapshot.run_state == RunState::Paused) {

        transitionRunState(RunState::Running, "play resume");

        applyResumePlaybackSideEffects("play resume");

        publishPlaybackStateFromInternalState("play resume");

        return;

    }

    if (snapshot.run_state == RunState::Running || snapshot.run_state == RunState::Starting) {

        return;

    }



    // Transition from Stopped/Ended -> Playing: start the pipeline threads.

    transitionRunState(RunState::Starting, "play requested");

    transitionPipelinePhase(PipelinePhase::Normal, "play requested");

    setEofReached(false, "play requested");

    setEndedReason(EndedReason::None, "play requested");
    applyStartPlaybackSideEffects("play requested");

    transitionRunState(RunState::Running, "play started");

    publishPlaybackStateFromInternalState("play started");

}



/// 暂停调度器与主时钟，但保留当前解码器和缓冲队列状态。

void PlayerCore::pause() {

    const CoreStateSnapshot snapshot = getCoreStateSnapshot();

    if (snapshot.run_state != RunState::Running) {

        return;

    }

    transitionRunState(RunState::Pausing, "pause requested");

    applyPausePlaybackSideEffects("pause requested");

    transitionRunState(RunState::Paused, "pause completed");

    publishPlaybackStateFromInternalState("pause completed");

}



/// 停止生产/消费线程并清空管线，同时把媒体位置复位到开头。

void PlayerCore::stop() {

    if (getCoreStateSnapshot().deferred_stop_pending) {

        serviceDeferredStop();

        return;

    }



    const CoreStateSnapshot snapshot = getCoreStateSnapshot();

    if (snapshot.run_state == RunState::Stopped &&

        !demux_running_.load() &&

        !audio_consumer_running_.load() &&

        !demux_thread_.joinable() &&

        !audio_consumer_thread_.joinable()) {

        return;

    }



    transitionRunState(RunState::Stopping, "stop requested");

    transitionPipelinePhase(PipelinePhase::Flushing, "stop requested");
    applyStopRequestSideEffects("stop requested");
    applyStopCompletionSideEffects("stop completed", true, true);

    transitionRunState(RunState::Stopped, "stop completed");

    transitionPipelinePhase(PipelinePhase::Idle, "stop completed");

    publishPlaybackStateFromInternalState("stop completed");

}



/// 执行一次带 flush 的时间线切换，确保旧包、旧帧和旧音频缓冲全部失效。

void PlayerCore::seek(double timestamp) {

    if (!opened_.load() || !demuxer_) {

        return;

    }

    serviceDeferredStop();

    const double duration = demuxer_->getMediaInfo().duration;

    if (duration > 0.0) {

        timestamp = std::max(0.0, std::min(duration, timestamp));

    } else {

        timestamp = std::max(0.0, timestamp);

    }

    LOG_INFO("Seek request: target=" << timestamp << "s");

    CoreStateSnapshot snapshot = getCoreStateSnapshot();

    if (snapshot.run_state == RunState::Ended) {

        transitionRunState(RunState::Stopped, "seek requested from ended");

        snapshot = getCoreStateSnapshot();

    }

    const bool was_playing = snapshot.run_state == RunState::Running;

    const bool demux_was_running = demux_running_.load();

    const TimelineSerial pending_seek_serial = allocateNextTimelineSerial("seek requested");

    setPendingSeekSerial(pending_seek_serial, "seek requested");

    transitionPipelinePhase(PipelinePhase::Seeking, "seek requested");

    setEofReached(false, "seek requested");
    const bool seek_ok =
        applySeekSideEffects(timestamp, was_playing, demux_was_running, pending_seek_serial, "seek requested");

    publishPlaybackStateFromInternalState(seek_ok ? "seek completed" : "seek failed");

}



/// 在暂停态按估算帧间隔执行单帧步进，并尝试命中目标时间附近的视频帧。

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



/// 按最近渲染帧时长、解码器帧率和媒体帧率估算单帧时长。

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



/// 在暂停态消费或主动解码一帧，并渲染到不早于目标时间的位置。

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

        if (!isAcceptedTimelineSerial(frame.serial)) {

            stale_paused_render_frames_dropped_.fetch_add(1);

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



/// 驱动显示层事件泵，把键鼠请求翻译为 seek、音量和播放控制。

void PlayerCore::pumpEvents() {

    serviceDeferredStop();

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

            stop();

        }



        if (video_renderer_->consumePreviousItemRequest()) {

            previous_item_requested_.store(true);

            stop();

        }



        if (video_renderer_->shouldQuit()) {

            quit_requested_.store(true);

            stop();

        }

    }



    handleABRepeatLoop();

}



/// 从章节元数据重建有序跳转时间点，供上一章/下一章逻辑复用。

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



/// 在播放位置触达 B 点附近时回跳到 A 点，形成闭环播放。

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



/// 缓存最近一次成功渲染的帧，供暂停态截图和逐帧浏览复用。

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



/// 从缓存帧复制出一个快照，并复用统一的截图落盘逻辑。

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

    const CoreStateSnapshot core_snapshot = getCoreStateSnapshot();

    const bool has_video_stream = demuxer_ && demuxer_->getMediaInfo().video_stream_idx >= 0;

    const bool has_audio_stream = demuxer_ && demuxer_->getMediaInfo().audio_stream_idx >= 0;

    snapshot.audio_output_initialized = audio_player_ && audio_player_->isInitialized();

    snapshot.video_only_fallback = has_video_stream && has_audio_stream && !snapshot.audio_output_initialized;

    snapshot.clock_source = clock_.getSource();

    const SchedulerControlSnapshot scheduler_control = makeSchedulerControlSnapshot();

    snapshot.scheduler_clock_policy = scheduler_control.clock_policy;

    snapshot.scheduler_audio_master_policy = scheduler_control.audio_master_policy;

    snapshot.scheduler_ended_policy = scheduler_control.ended_policy;

    snapshot.scheduler_audio_buffered_seconds = scheduler_control.audio_buffered_seconds;

    snapshot.timeline_serial = core_snapshot.timeline_serial;

    snapshot.pending_seek_serial = core_snapshot.pending_seek_serial;

    snapshot.ended_reason = core_snapshot.ended_reason;

    snapshot.video_packet_queue_generation = video_packet_queue_ ? video_packet_queue_->generation() : 0;

    snapshot.audio_packet_queue_generation = audio_packet_queue_ ? audio_packet_queue_->generation() : 0;

    snapshot.demux_video_packets = demux_video_packets_.load();

    snapshot.demux_audio_packets = demux_audio_packets_.load();

    snapshot.demux_push_retries = demux_push_retries_.load();

    snapshot.demux_dropped_packets = demux_dropped_packets_.load();

    snapshot.demux_ignored_packets = demux_ignored_packets_.load();

    snapshot.demux_queue_drop_packets = demux_queue_drop_packets_.load();

    snapshot.decode_video_ok = decode_video_ok_.load();

    snapshot.decode_audio_ok = decode_audio_ok_.load();

    snapshot.stale_video_packets_dropped = stale_video_packets_dropped_.load();

    snapshot.stale_audio_packets_dropped = stale_audio_packets_dropped_.load();

    snapshot.stale_video_frames_dropped = stale_video_frames_dropped_.load();

    snapshot.stale_audio_frames_dropped = stale_audio_frames_dropped_.load();

    snapshot.stale_audio_submit_frames_dropped = stale_audio_submit_frames_dropped_.load();

    snapshot.stale_render_frames_dropped = stale_render_frames_dropped_.load();

    snapshot.stale_paused_render_frames_dropped = stale_paused_render_frames_dropped_.load();

    snapshot.video_packet_dequeue_count = video_packet_dequeue_count_.load();

    snapshot.video_send_packet_ok = video_send_packet_ok_.load();

    snapshot.video_send_packet_last_ret = video_send_packet_last_ret_.load();

    snapshot.decode_video_send_eagain = decode_video_send_eagain_.load();

    snapshot.decode_audio_send_eagain = decode_audio_send_eagain_.load();

    snapshot.video_decoder_drain_signals = video_decoder_drain_signals_.load();

    snapshot.audio_decoder_drain_signals = audio_decoder_drain_signals_.load();

    snapshot.video_native_output_frames = video_native_output_frames_.load();

    snapshot.video_copy_back_frames = video_copy_back_frames_.load();

    snapshot.video_swscale_frames = video_swscale_frames_.load();

    snapshot.video_filter_blocked_native_frames = video_filter_blocked_native_frames_.load();

    snapshot.audio_submitted_frames = audio_submitted_frames_.load();

    snapshot.render_frames = render_frames_.load();



    const SchedulerStats scheduler_stats = scheduler_.getStats();

    snapshot.scheduler_video_decoded_frames = scheduler_stats.video_decoded_frames;

    snapshot.scheduler_audio_decoded_frames = scheduler_stats.audio_decoded_frames;

    snapshot.scheduler_late_drops = scheduler_stats.dropped_late_frames;

    snapshot.scheduler_wait_events = scheduler_stats.wait_events;

    snapshot.scheduler_video_backpressure_events = scheduler_stats.video_backpressure_events;

    snapshot.scheduler_audio_backpressure_events = scheduler_stats.audio_backpressure_events;

    snapshot.scheduler_video_backpressure_wait_ms = scheduler_stats.video_backpressure_wait_ms;

    snapshot.scheduler_audio_backpressure_wait_ms = scheduler_stats.audio_backpressure_wait_ms;

    snapshot.scheduler_video_restart_attempts = scheduler_stats.video_restart_attempts;

    snapshot.scheduler_audio_restart_attempts = scheduler_stats.audio_restart_attempts;

    snapshot.scheduler_render_restart_attempts = scheduler_stats.render_restart_attempts;

    snapshot.scheduler_video_restart_limit_hits = scheduler_stats.video_restart_limit_hits;

    snapshot.scheduler_audio_restart_limit_hits = scheduler_stats.audio_restart_limit_hits;

    snapshot.scheduler_render_restart_limit_hits = scheduler_stats.render_restart_limit_hits;

    snapshot.runtime_failure_stop_requests = runtime_failure_stop_requests_.load();

    snapshot.runtime_failure_fail_sessions = runtime_failure_fail_sessions_.load();

    snapshot.illegal_session_transitions = illegal_session_transitions_.load();

    snapshot.illegal_run_transitions = illegal_run_transitions_.load();

    snapshot.illegal_pipeline_transitions = illegal_pipeline_transitions_.load();

    snapshot.video_copy_back_time_us_total = video_copy_back_time_us_total_.load();

    snapshot.video_swscale_time_us_total = video_swscale_time_us_total_.load();

    snapshot.video_copy_back_time_us_max = video_copy_back_time_us_max_.load();

    snapshot.video_swscale_time_us_max = video_swscale_time_us_max_.load();

    const render::RendererDiagnostics renderer_diagnostics =

        video_renderer_ ? video_renderer_->getDiagnostics() : render::RendererDiagnostics{};

    snapshot.display_copy_frames = renderer_diagnostics.display_copy_frames;

    snapshot.display_copy_bytes = renderer_diagnostics.display_copy_bytes;

    snapshot.display_copy_time_us_total = renderer_diagnostics.display_copy_time_us_total;

    snapshot.display_copy_time_us_max = renderer_diagnostics.display_copy_time_us_max;

    snapshot.video_packet_queue_size = video_packet_queue_ ? video_packet_queue_->size() : 0;

    snapshot.audio_packet_queue_size = audio_packet_queue_ ? audio_packet_queue_->size() : 0;

    const FrameQueueStats video_queue_stats = video_queue_.getStats();

    const FrameQueueStats audio_queue_stats = audio_queue_.getStats();

    snapshot.video_frame_queue_size = has_video_stream ? video_queue_stats.size : 0;

    snapshot.audio_frame_queue_size = has_audio_stream ? audio_queue_stats.size : 0;

    snapshot.video_frame_queue_capacity = has_video_stream ? video_queue_stats.capacity : 0;

    snapshot.audio_frame_queue_capacity = has_audio_stream ? audio_queue_stats.capacity : 0;

    snapshot.video_frame_queue_peak_size = has_video_stream ? video_queue_stats.peak_size : 0;

    snapshot.audio_frame_queue_peak_size = has_audio_stream ? audio_queue_stats.peak_size : 0;

    snapshot.video_frame_queue_push_timeouts = has_video_stream ? video_queue_stats.push_timeout_count : 0;

    snapshot.audio_frame_queue_push_timeouts = has_audio_stream ? audio_queue_stats.push_timeout_count : 0;

    snapshot.video_frame_queue_generation = has_video_stream ? video_queue_stats.generation : 0;

    snapshot.audio_frame_queue_generation = has_audio_stream ? audio_queue_stats.generation : 0;

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



/// 初始化音视频解码器，并按配置选择硬解或软解后端。

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



        auto configure_video_codec_ctx = [&](AVCodecContext* ctx, decoder::DecoderBackend backend) -> bool {

            if (!ctx || avcodec_parameters_to_context(ctx, vs->codecpar) < 0) {

                return false;

            }



            if (backend == decoder::DecoderBackend::Software) {

                // Keep software video decode conservative first; current scheduler model

                // must prove it can stably produce frames before re-enabling aggressive threading.

                ctx->thread_count = 1;

                ctx->thread_type = 0;

            } else {

                int thread_type = 0;

                if ((codec->capabilities & AV_CODEC_CAP_SLICE_THREADS) != 0) {

                    thread_type |= FF_THREAD_SLICE;

                }

                if ((codec->capabilities & AV_CODEC_CAP_FRAME_THREADS) != 0) {

                    thread_type |= FF_THREAD_FRAME;

                }

                ctx->thread_count = 0;  // auto

                ctx->thread_type = thread_type;

            }

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

            if (!video_codec_ctx_ || !configure_video_codec_ctx(video_codec_ctx_, backend)) {

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



            if (backend == decoder::DecoderBackend::D3D11VA) {

                const size_t planned_queue_capacity = selectVideoFrameQueueCapacity(info, backend);

                video_codec_ctx_->extra_hw_frames = static_cast<int>(clampValue<size_t>(

                    planned_queue_capacity / 2,

                    8,

                    16));

                LOG_INFO("Configuring D3D11VA decoder with extra_hw_frames="

                         << video_codec_ctx_->extra_hw_frames

                         << " for planned video queue capacity " << planned_queue_capacity);

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

            LOG_INFO("Video decoder threading: backend="

                     << decoder::DecoderFactory::backendName(video_decoder_backend_)

                     << " thread_count=" << video_codec_ctx_->thread_count

                     << " thread_type=" << codecThreadTypeToString(video_codec_ctx_->thread_type));

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



/// 释放解码器、硬件设备上下文以及音视频格式转换资源。

void PlayerCore::releaseDecoders() {

    std::scoped_lock codec_lock(video_codec_mutex_, audio_codec_mutex_);

    releaseVideoScaler();

    releaseAudioResampler();

    video_decoder_draining_ = false;

    audio_decoder_draining_ = false;

    video_decoder_packet_serial_ = kInvalidTimelineSerial;

    audio_decoder_packet_serial_ = kInvalidTimelineSerial;

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



/// 在 Windows 上为视频解码器绑定 D3D11VA 设备；失败时回退软解。

/// 在 get_format() 阶段为 D3D11VA decoder 配置可直接做 shader sampling 的 frames ctx。
bool PlayerCore::configureD3D11HardwareFramesContext(AVCodecContext* codec_ctx) {

#if defined(_WIN32)

    if (!codec_ctx || !video_hw_device_ctx_ || video_hw_pixel_fmt_ == AV_PIX_FMT_NONE) {

        return false;

    }



    AVBufferRef* frames_ref = nullptr;

    const int params_ret = avcodec_get_hw_frames_parameters(codec_ctx, video_hw_device_ctx_, video_hw_pixel_fmt_, &frames_ref);

    if (params_ret < 0 || !frames_ref) {

        LOG_WARNING("Failed to query D3D11VA frames parameters for shader-resource pool: "

                    << avErrorToString(params_ret) << " (" << params_ret << ")");

        return false;

    }



    auto* frames_ctx = reinterpret_cast<AVHWFramesContext*>(frames_ref->data);

    auto* d3d11_frames_ctx = frames_ctx ? reinterpret_cast<AVD3D11VAFramesContext*>(frames_ctx->hwctx) : nullptr;

    if (!frames_ctx || !d3d11_frames_ctx) {

        LOG_WARNING("D3D11VA frames parameters returned empty hw frames context, fallback to decoder-owned surfaces");

        av_buffer_unref(&frames_ref);

        return false;

    }



    const UINT original_bind_flags = d3d11_frames_ctx->BindFlags;

    d3d11_frames_ctx->BindFlags |= D3D11_BIND_SHADER_RESOURCE;

    if (frames_ctx->initial_pool_size > 0 && codec_ctx->extra_hw_frames > 0) {

        frames_ctx->initial_pool_size += codec_ctx->extra_hw_frames;

    }



    const UINT requested_bind_flags = d3d11_frames_ctx->BindFlags;

    const int initial_pool_size = frames_ctx->initial_pool_size;

    const AVPixelFormat sw_format = frames_ctx->sw_format;

    const int width = frames_ctx->width;

    const int height = frames_ctx->height;

    const int init_ret = av_hwframe_ctx_init(frames_ref);

    if (init_ret < 0) {

        LOG_WARNING("Failed to initialize D3D11VA shader-resource frames context, fallback to decoder-owned surfaces: "

                    << avErrorToString(init_ret) << " (" << init_ret << ")"

                    << " original_bind_flags=" << original_bind_flags

                    << " requested_bind_flags=" << requested_bind_flags

                    << " initial_pool_size=" << initial_pool_size);

        av_buffer_unref(&frames_ref);

        return false;

    }



    codec_ctx->hw_frames_ctx = frames_ref;

    LOG_INFO("Configured D3D11VA frames context for direct shader sampling: bind_flags="

             << requested_bind_flags

             << " initial_pool_size=" << initial_pool_size

             << " sw_format=" << (av_get_pix_fmt_name(sw_format) ? av_get_pix_fmt_name(sw_format) : "unknown")

             << " size=" << width << "x" << height);

    return true;

#else

    (void)codec_ctx;

    return false;

#endif

}
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

                    if (!self->video_hw_device_ctx_) {

                        LOG_WARNING("D3D11VA device context is missing during get_format, fallback to software decode backend");

                        self->video_hw_pixel_fmt_ = AV_PIX_FMT_NONE;

                        self->video_decoder_backend_ = decoder::DecoderBackend::Software;

                        return selectSoftwarePixelFormat(pix_fmts);

                    }

                    if (!self->configureD3D11HardwareFramesContext(ctx)) {

                        LOG_WARNING("Continuing with decoder-owned D3D11VA surfaces because shader-resource pool setup failed");

                    }

                    return *p;

                }

            }

            LOG_WARNING("Requested D3D11VA pixel format not offered by decoder, fallback to software decode backend");

            self->video_hw_pixel_fmt_ = AV_PIX_FMT_NONE;

            self->video_decoder_backend_ = decoder::DecoderBackend::Software;

            return selectSoftwarePixelFormat(pix_fmts);

        }

    }



    return selectSoftwarePixelFormat(pix_fmts);

}

AVPixelFormat PlayerCore::selectSoftwarePixelFormat(const AVPixelFormat* pix_fmts) {

    if (!pix_fmts) {

        return AV_PIX_FMT_NONE;

    }



    for (const AVPixelFormat* p = pix_fmts; *p != AV_PIX_FMT_NONE; ++p) {

        if (!isHardwarePixelFormat(*p)) {

            return *p;

        }

    }



    return pix_fmts[0];

}
bool PlayerCore::prepareVideoOutputFrame(AVFrame* decoded_frame, VideoFrame& out) {

    if (!decoded_frame || !out.frame) {

        return false;

    }



    const AVPixelFormat decoded_format = static_cast<AVPixelFormat>(decoded_frame->format);

    const bool renderer_supports_native =

        video_renderer_ && video_renderer_->supportsNativeFrameFormat(decoded_format);

    const bool video_filters_enabled = filter_pipeline_.hasEnabledVideoFilters();

    const bool renderer_accepts_native = renderer_supports_native && !video_filters_enabled;

    if (renderer_supports_native && video_filters_enabled) {

        video_filter_blocked_native_frames_.fetch_add(1);

    }

    if (renderer_accepts_native) {

        video_native_output_frames_.fetch_add(1);

        return true;

    }



    AVFrame* software_frame = nullptr;

    AVFrame* src_frame = decoded_frame;



    if (video_hw_pixel_fmt_ != AV_PIX_FMT_NONE && decoded_frame->format == video_hw_pixel_fmt_) {

        software_frame = av_frame_alloc();

        if (!software_frame) {

            return false;

        }



        const auto copy_back_start = std::chrono::steady_clock::now();

        if (av_hwframe_transfer_data(software_frame, decoded_frame, 0) < 0 ||

            av_frame_copy_props(software_frame, decoded_frame) < 0) {

            av_frame_free(&software_frame);

            return false;

        }

        const uint64_t copy_back_us = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(

                                                                std::chrono::steady_clock::now() - copy_back_start)

                                                                .count());

        video_copy_back_frames_.fetch_add(1);

        video_copy_back_time_us_total_.fetch_add(copy_back_us);

        updateMaxAtomic(video_copy_back_time_us_max_, copy_back_us);

        src_frame = software_frame;

    }



    const AVPixelFormat output_format = static_cast<AVPixelFormat>(src_frame->format);

    const bool renderer_supports_direct_frame =

        video_renderer_ && video_renderer_->supportsDirectFrameFormat(output_format);



    bool ok = true;

    if (renderer_supports_direct_frame && !video_filters_enabled) {

        if (src_frame != out.frame) {

            av_frame_unref(out.frame);

            ok = av_frame_ref(out.frame, src_frame) >= 0;

        }

    } else if (src_frame->format != AV_PIX_FMT_YUV420P) {

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



        const auto swscale_start = std::chrono::steady_clock::now();

        ok = convertVideoFrameToYuv420(conversion_source, out.frame);

        if (ok) {

            const uint64_t swscale_us = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(

                                                                  std::chrono::steady_clock::now() - swscale_start)

                                                                  .count());

            video_swscale_frames_.fetch_add(1);

            video_swscale_time_us_total_.fetch_add(swscale_us);

            updateMaxAtomic(video_swscale_time_us_max_, swscale_us);

        }

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



/// 按当前源帧尺寸和像素格式准备 `swscale` 上下文。

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



/// 把任意输入像素格式转换为渲染链统一使用的 `YUV420P`。

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



/// 确保音频重采样器输出格式与当前 SDL 设备配置一致。

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



/// 释放音频重采样上下文，并清空输入输出布局缓存。

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



void PlayerCore::configureFrameQueues(const MediaInfo& info) {

    const size_t video_capacity =

        info.video_stream_idx >= 0 ? selectVideoFrameQueueCapacity(info, video_decoder_backend_) : 0;

    const size_t audio_capacity = (info.audio_stream_idx >= 0 && audio_codec_ctx_)

                                      ? selectAudioFrameQueueCapacity(info)

                                      : 0;



    if (video_capacity > 0) {

        video_queue_.setCapacity(video_capacity);

    }

    if (audio_capacity > 0) {

        audio_queue_.setCapacity(audio_capacity);

    }



    LOG_INFO("Frame queues configured:"

             << " video=" << (video_capacity > 0 ? std::to_string(video_capacity) : std::string("disabled"))

             << " audio=" << (audio_capacity > 0 ? std::to_string(audio_capacity) : std::string("disabled"))

             << " fps=" << info.fps

             << " size=" << info.width << "x" << info.height

             << " backend=" << decoder::DecoderFactory::backendName(video_decoder_backend_));

}



/// 启动解复用线程；负责读包、分发到音视频包队列并传播 EOF。

void PlayerCore::startDemuxThread() {

    reapCompletedWorkers();

    if (demux_running_.exchange(true)) {

        return;

    }

    const TimelineSerial demux_serial = currentTimelineSerial();

    if (demux_serial == kInvalidTimelineSerial) {

        demux_running_.store(false);

        LOG_WARNING("Demux thread start ignored: no active timeline serial");

        return;

    }

    setEofReached(false, "demux thread started");

    if (video_packet_queue_) {

        video_packet_queue_->start();

    }

    if (audio_packet_queue_) {

        audio_packet_queue_->start();

    }

    demux_thread_ = std::thread([this, demux_serial] {

        auto ensureQueuedPacketOwnsData = [&](PacketPtr& packet, const char* stream_tag) -> bool {

            if (!packet || packet->buf) {

                return true;

            }



            const int ret = av_packet_make_refcounted(packet.get());

            if (ret < 0) {

                LOG_WARNING("Failed to make queued " << stream_tag

                            << " packet refcounted before async handoff: "

                            << avErrorToString(ret) << " (" << ret << ")"

                            << " size=" << packet->size

                            << " pts=" << packet->pts

                            << " dts=" << packet->dts);

                return false;

            }

            return true;

        };



        // Demux only reads and dispatches packets; EOF is propagated downstream explicitly.

        while (demux_running_.load() && demuxer_ && !demuxer_->isEof()) {

            if (!video_packet_queue_ && !audio_packet_queue_) {

                std::this_thread::sleep_for(std::chrono::milliseconds(2));

                continue;

            }



            PacketPtr packet(av_packet_alloc());

            if (!packet) {

                std::this_thread::sleep_for(std::chrono::milliseconds(1));

                continue;

            }



            if (!demuxer_->readPacket(packet.get())) {

                if (video_packet_queue_) {

                    video_packet_queue_->setEof(true);

                }

                if (audio_packet_queue_) {

                    audio_packet_queue_->setEof(true);

                }

                setEofReached(true, "demux reached EOF");

                break;

            }



            const MediaInfo& info = demuxer_->getMediaInfo();

            bool queued = false;

            bool targeted_stream = false;

            if (packet->stream_index == info.video_stream_idx && video_packet_queue_) {

                targeted_stream = true;

                if (ensureQueuedPacketOwnsData(packet, "video")) {

                    // On successful push the queue owns the packet lifetime.

                    while (demux_running_.load() && !queued) {

                        queued = video_packet_queue_->push(DemuxPacket{std::move(packet), demux_serial}, 20);

                        if (queued) {

                            break;

                        }

                        demux_push_retries_.fetch_add(1);

                        std::this_thread::sleep_for(std::chrono::milliseconds(1));

                    }

                }

                if (queued) {

                    demux_video_packets_.fetch_add(1);

                }

            } else if (packet->stream_index == info.audio_stream_idx && audio_packet_queue_) {

                targeted_stream = true;

                if (ensureQueuedPacketOwnsData(packet, "audio")) {

                    while (demux_running_.load() && !queued) {

                        queued = audio_packet_queue_->push(DemuxPacket{std::move(packet), demux_serial}, 20);

                        if (queued) {

                            break;

                        }

                        demux_push_retries_.fetch_add(1);

                        std::this_thread::sleep_for(std::chrono::milliseconds(1));

                    }

                }

                if (queued) {

                    demux_audio_packets_.fetch_add(1);

                }

            }



            if (!queued) {

                demux_dropped_packets_.fetch_add(1);

                if (targeted_stream) {

                    demux_queue_drop_packets_.fetch_add(1);

                } else {

                    demux_ignored_packets_.fetch_add(1);

                }

            }

            maybeLogDiagnostics("demux");

        }

        demux_running_.store(false);

    });

}



/// 停止解复用线程，并重置包队列的停止状态以便后续复用。

void PlayerCore::stopDemuxThread() {

    demux_running_.store(false);

    if (video_packet_queue_) {

        video_packet_queue_->stop();

    }

    if (audio_packet_queue_) {

        audio_packet_queue_->stop();

    }

    if (demux_thread_.joinable() && demux_thread_.get_id() != std::this_thread::get_id()) {

        demux_thread_.join();

    }

    if (video_packet_queue_) {

        video_packet_queue_->start();

    }

    if (audio_packet_queue_) {

        audio_packet_queue_->start();

    }

}



/// 启动音频消费线程；把解码 PCM 提交给 `AudioPlayer` 并推进音频主时钟。

void PlayerCore::startAudioConsumer() {

    reapCompletedWorkers();

    if (!audio_player_ || !audio_player_->isInitialized() || audio_consumer_running_.exchange(true)) {

        return;

    }

    audio_consumer_thread_ = std::thread([this] {

        while (audio_consumer_running_.load()) {

            AudioFrame frame;

            const double played_pts = audio_player_->getPlaybackPts();

            if (played_pts > 0.0 &&

                state_.load() == PlaybackState::Playing &&

                isAcceptedTimelineSerial(audio_output_serial_.load())) {

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

            if (!isAcceptedTimelineSerial(frame.serial)) {

                stale_audio_submit_frames_dropped_.fetch_add(1);

                maybeLogDiagnostics("audio-stale");

                continue;

            }



            if (audio_player_->outputBytesPerSample() == 2) {

                const size_t sample_count = frame.samples.size() / 2;

                filter_pipeline_.processAudio(frame.samples.data(), sample_count, frame.channels);

            }

            const double audio_delay = audio_delay_seconds_.load();

            const double delayed_pts = std::max(0.0, frame.pts + audio_delay);

            audio_output_serial_.store(frame.serial);

            audio_player_->play(frame.samples, delayed_pts);

            audio_submitted_frames_.fetch_add(1);

            maybeLogDiagnostics("audio-play");

        }

        audio_consumer_running_.store(false);

    });

}



/// 停止音频消费线程并等待退出。

void PlayerCore::stopAudioConsumer() {

    audio_consumer_running_.store(false);

    if (audio_consumer_thread_.joinable() && audio_consumer_thread_.get_id() != std::this_thread::get_id()) {

        audio_consumer_thread_.join();

    }

}



/// 清空调度帧队列和压缩包队列，丢弃当前时间线上的残留数据。

void PlayerCore::flushPipelines() {

    scheduler_.flush();

    if (video_packet_queue_) {

        video_packet_queue_->clear();

    }

    if (audio_packet_queue_) {

        audio_packet_queue_->clear();

    }

}



void PlayerCore::applyStartPlaybackSideEffects(const char* reason) {

    startDemuxThread();

    startAudioConsumer();

    scheduler_.start();

    resetDiagnostics();

    LOG_INFO("PlayerCore applied start playback side effects"

             << " reason=" << (reason ? reason : "unspecified"));

}



void PlayerCore::applyResumePlaybackSideEffects(const char* reason) {

    scheduler_.resume();

    clock_.resume();

    if (audio_player_) {

        audio_player_->resume();

    }

    LOG_INFO("PlayerCore applied resume playback side effects"

             << " reason=" << (reason ? reason : "unspecified"));

}



void PlayerCore::applyPausePlaybackSideEffects(const char* reason) {

    scheduler_.pause();

    clock_.pause();

    if (audio_player_) {

        audio_player_->pause();

    }

    LOG_INFO("PlayerCore applied pause playback side effects"

             << " reason=" << (reason ? reason : "unspecified"));

}



void PlayerCore::applyStopRequestSideEffects(const char* reason) {

    activateTimelineSerial(allocateNextTimelineSerial(reason), reason);

    clearPendingSeekSerial(reason);

    setDeferredStopPending(true, reason);

    setEofReached(false, reason);

    setEndedReason(EndedReason::None, reason);

    demux_running_.store(false);

    audio_consumer_running_.store(false);

    scheduler_.requestStopAsync();

    if (video_packet_queue_) {

        video_packet_queue_->stop();

    }

    if (audio_packet_queue_) {

        audio_packet_queue_->stop();

    }

    if (audio_player_) {

        audio_player_->pause();

    }

    LOG_INFO("PlayerCore applied stop-request side effects"

             << " reason=" << (reason ? reason : "unspecified"));

}



void PlayerCore::applyStopCompletionSideEffects(const char* reason, bool rewind_demux, bool reset_position) {

    reapCompletedWorkers();

    stopDemuxThread();

    scheduler_.stop();

    stopAudioConsumer();

    flushPipelines();

    if (audio_player_) {

        audio_player_->stop();

    }

    if (rewind_demux && demuxer_ && demuxer_->isOpen()) {

        demuxer_->seek(0.0);

    }

    if (reset_position) {

        position_.store(0.0);

        clock_.reset();

    }

    setDeferredStopPending(false, reason);

    LOG_INFO("PlayerCore applied stop-completion side effects"

             << " reason=" << (reason ? reason : "unspecified")

             << " rewind_demux=" << (rewind_demux ? "true" : "false")

             << " reset_position=" << (reset_position ? "true" : "false"));

}



bool PlayerCore::applySeekSideEffects(double timestamp,

                                      bool was_playing,

                                      bool demux_was_running,

                                      TimelineSerial pending_seek_serial,

                                      const char* reason) {

    if (was_playing) {

        scheduler_.pause();

    }

    if (demux_was_running) {

        stopDemuxThread();

    }

    transitionPipelinePhase(PipelinePhase::Flushing, "seek flushing");

    flushPipelines();

    const bool seek_ok = demuxer_->seek(timestamp);

    {

        std::scoped_lock codec_lock(video_codec_mutex_, audio_codec_mutex_);

        if (video_codec_ctx_) {

            avcodec_flush_buffers(video_codec_ctx_);

        }

        if (audio_codec_ctx_) {

            avcodec_flush_buffers(audio_codec_ctx_);

        }

        video_decoder_draining_ = false;

        audio_decoder_draining_ = false;

        video_decoder_packet_serial_ = kInvalidTimelineSerial;

        audio_decoder_packet_serial_ = kInvalidTimelineSerial;

        releaseAudioResampler();

    }

    if (!seek_ok) {

        handleRuntimeFailure(ErrorCode::SeekFailed, "seek failed", reason, FailureRecoveryPolicy::EmitOnly);

    }

    if (audio_player_) {

        audio_player_->stop();

    }

    flushPipelines();

    position_.store(timestamp);

    clock_.setTime(timestamp);

    clock_.setAudioClock(timestamp);

    clock_.setVideoClock(timestamp);

    updateSubtitleOverlay(timestamp);

    emitPositionChanged(timestamp);

    if (seek_ok) {

        activateTimelineSerial(pending_seek_serial, "seek completed");

    } else {

        clearPendingSeekSerial("seek failed");

    }

    if (demux_was_running) {

        startDemuxThread();

    }

    if (was_playing) {

        if (audio_player_) {

            audio_player_->resume();

        }

        scheduler_.resume();

    }

    transitionPipelinePhase(demux_was_running ? PipelinePhase::Normal : PipelinePhase::Idle,

                            seek_ok ? "seek completed" : "seek failed");

    LOG_INFO("PlayerCore applied seek side effects"

             << " reason=" << (reason ? reason : "unspecified")

             << " target=" << timestamp

             << " seek_ok=" << (seek_ok ? "true" : "false"));

    return seek_ok;

}



void PlayerCore::applySessionReleaseSideEffects(const char* reason) {

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

    LOG_INFO("PlayerCore applied session-release side effects"

             << " reason=" << (reason ? reason : "unspecified"));

}



bool PlayerCore::handleRuntimeFailure(ErrorCode code,

                                      const char* message,

                                      const char* reason,

                                      FailureRecoveryPolicy policy) {

    switch (policy) {

    case FailureRecoveryPolicy::EmitOnly:

        emitError(code, message ? message : "runtime failure");

        return false;

    case FailureRecoveryPolicy::StopPlayback: {

        CoreStateSnapshot snapshot = getCoreStateSnapshot();

        if (snapshot.run_state != RunState::Stopped && snapshot.run_state != RunState::Stopping) {

            transitionRunState(RunState::Stopping, reason);

            transitionPipelinePhase(PipelinePhase::Flushing, reason);

        }

        if (snapshot.run_state != RunState::Stopped && !snapshot.deferred_stop_pending) {

            applyStopRequestSideEffects(reason);

            runtime_failure_stop_requests_.fetch_add(1);

        }

        publishPlaybackStateFromInternalState(reason);

        emitError(code, message ? message : "runtime failure");

        return false;

    }

    case FailureRecoveryPolicy::FailSession: {

        runtime_failure_fail_sessions_.fetch_add(1);

        CoreStateSnapshot snapshot = getCoreStateSnapshot();

        if (snapshot.run_state != RunState::Stopped && snapshot.run_state != RunState::Stopping) {

            transitionRunState(RunState::Stopping, reason);

            transitionPipelinePhase(PipelinePhase::Flushing, reason);

        }

        if (snapshot.run_state != RunState::Stopped && !snapshot.deferred_stop_pending) {

            applyStopRequestSideEffects(reason);

            runtime_failure_stop_requests_.fetch_add(1);

        }

        if (getCoreStateSnapshot().deferred_stop_pending) {

            applyStopCompletionSideEffects(reason, true, true);

        }

        applySessionReleaseSideEffects(reason);

        setEofReached(false, reason);

        clearPendingSeekSerial(reason);

        setDeferredStopPending(false, reason);

        setEndedReason(EndedReason::None, reason);

        transitionRunState(RunState::Stopped, reason);

        transitionPipelinePhase(PipelinePhase::Idle, reason);

        transitionSessionState(SessionState::Failed, reason);

        publishPlaybackStateFromInternalState(reason);

        emitError(code, message ? message : "runtime failure");

        return false;

    }

    }

    return false;

}



void PlayerCore::requestDeferredStop() {

    transitionRunState(RunState::Stopping, "deferred stop requested");

    transitionPipelinePhase(PipelinePhase::Flushing, "deferred stop requested");

    applyStopRequestSideEffects("deferred stop requested");

    publishPlaybackStateFromInternalState("deferred stop requested");

}



void PlayerCore::serviceDeferredStop() {

    reapCompletedWorkers();

    if (!consumeDeferredStopPending("service deferred stop")) {

        return;

    }



    applyStopCompletionSideEffects("service deferred stop", true, true);

    transitionRunState(RunState::Stopped, "service deferred stop");

    transitionPipelinePhase(PipelinePhase::Idle, "service deferred stop");

    publishPlaybackStateFromInternalState("service deferred stop");

}



void PlayerCore::reapCompletedWorkers() {

    if (!demux_running_.load() && demux_thread_.joinable() && demux_thread_.get_id() != std::this_thread::get_id()) {

        demux_thread_.join();

    }

    if (!audio_consumer_running_.load() &&

        audio_consumer_thread_.joinable() &&

        audio_consumer_thread_.get_id() != std::this_thread::get_id()) {

        audio_consumer_thread_.join();

    }

}



/// 重置链路诊断计数器，为新一轮播放或 seek 观察窗口做准备。

void PlayerCore::resetDiagnostics() {

    demux_video_packets_.store(0);

    demux_audio_packets_.store(0);

    demux_push_retries_.store(0);

    demux_dropped_packets_.store(0);

    demux_ignored_packets_.store(0);

    demux_queue_drop_packets_.store(0);

    decode_video_ok_.store(0);

    decode_audio_ok_.store(0);

    stale_video_packets_dropped_.store(0);

    stale_audio_packets_dropped_.store(0);

    stale_video_frames_dropped_.store(0);

    stale_audio_frames_dropped_.store(0);

    stale_audio_submit_frames_dropped_.store(0);

    stale_render_frames_dropped_.store(0);

    stale_paused_render_frames_dropped_.store(0);

    video_packet_dequeue_count_.store(0);

    video_send_packet_ok_.store(0);

    video_send_packet_last_ret_.store(kVideoSendPacketRetNotSet);

    decode_video_send_eagain_.store(0);

    decode_audio_send_eagain_.store(0);

    video_decoder_drain_signals_.store(0);

    audio_decoder_drain_signals_.store(0);

    video_native_output_frames_.store(0);

    video_copy_back_frames_.store(0);

    video_swscale_frames_.store(0);

    video_filter_blocked_native_frames_.store(0);

    audio_submitted_frames_.store(0);

    render_frames_.store(0);

    video_copy_back_time_us_total_.store(0);

    video_swscale_time_us_total_.store(0);

    video_copy_back_time_us_max_.store(0);

    video_swscale_time_us_max_.store(0);

    runtime_failure_stop_requests_.store(0);

    runtime_failure_fail_sessions_.store(0);

    illegal_session_transitions_.store(0);

    illegal_run_transitions_.store(0);

    illegal_pipeline_transitions_.store(0);

    if (video_renderer_) {

        video_renderer_->resetDiagnostics();

    }

    video_queue_.resetStats();

    audio_queue_.resetStats();

    last_diag_log_ms_.store(nowSteadyMs());

    last_video_decode_issue_log_ms_.store(0);

    first_video_send_started_logged_.store(false);

    first_video_send_completed_logged_.store(false);

}



/// 节流输出链路诊断日志，便于观察 demux/decoder/render/audio 健康度。

void PlayerCore::maybeLogDiagnostics(const char* source_tag) {

    const int64_t now_ms = nowSteadyMs();

    int64_t last_ms = last_diag_log_ms_.load();

    while (now_ms - last_ms >= 1000) {

        if (last_diag_log_ms_.compare_exchange_weak(last_ms, now_ms)) {

            const SchedulerStats scheduler_stats = scheduler_.getStats();

            const size_t video_pkt_q = video_packet_queue_ ? video_packet_queue_->size() : 0;

            const size_t audio_pkt_q = audio_packet_queue_ ? audio_packet_queue_->size() : 0;

            const FrameQueueStats video_frame_queue_stats = video_queue_.getStats();

            const FrameQueueStats audio_frame_queue_stats = audio_queue_.getStats();

            const CoreStateSnapshot core_snapshot = getCoreStateSnapshot();

            const SchedulerControlSnapshot scheduler_control = makeSchedulerControlSnapshot();

            const render::RendererDiagnostics renderer_diagnostics =

                video_renderer_ ? video_renderer_->getDiagnostics() : render::RendererDiagnostics{};

            LOG_INFO("[diag:" << source_tag << "]"

                     << " serial(current=" << currentTimelineSerial()

                     << ",accepted=" << acceptedTimelineSerial()

                     << ",pending=" << core_snapshot.pending_seek_serial << ")"

                     << " sched(policy_clock="
                     << schedulerClockPolicyName(scheduler_control.clock_policy)

                     << ",policy_audio="
                     << schedulerAudioMasterPolicyName(scheduler_control.audio_master_policy)

                     << ",policy_ended="
                     << schedulerEndedPolicyName(scheduler_control.ended_policy)

                     << ",audio_buf_s=" << scheduler_control.audio_buffered_seconds << ")"

                     << " gen(pkt_v=" << (video_packet_queue_ ? video_packet_queue_->generation() : 0)

                     << ",pkt_a=" << (audio_packet_queue_ ? audio_packet_queue_->generation() : 0)

                     << ",frm_v=" << video_frame_queue_stats.generation

                     << ",frm_a=" << audio_frame_queue_stats.generation << ")"

                     << " ended=" << endedReasonName(core_snapshot.ended_reason)

                     << " demux(v=" << demux_video_packets_.load()

                     << ",a=" << demux_audio_packets_.load()

                     << ",retry=" << demux_push_retries_.load()

                     << ",drop=" << demux_dropped_packets_.load()

                     << ",ignore=" << demux_ignored_packets_.load()

                     << ",qdrop=" << demux_queue_drop_packets_.load() << ")"

                     << " pkt_q(v=" << video_pkt_q << ",a=" << audio_pkt_q << ")"

                     << " dec(core v=" << decode_video_ok_.load()

                     << ",a=" << decode_audio_ok_.load()

                     << ",stale_v_pkt=" << stale_video_packets_dropped_.load()

                     << ",stale_a_pkt=" << stale_audio_packets_dropped_.load()

                     << ",stale_v_frm=" << stale_video_frames_dropped_.load()

                     << ",stale_a_frm=" << stale_audio_frames_dropped_.load()

                     << ",stale_a_submit=" << stale_audio_submit_frames_dropped_.load()

                     << ",stale_render=" << stale_render_frames_dropped_.load()

                     << ",stale_pause_render=" << stale_paused_render_frames_dropped_.load()

                     << ",fail_stop=" << runtime_failure_stop_requests_.load()

                     << ",fail_session=" << runtime_failure_fail_sessions_.load()

                     << ",illegal_session=" << illegal_session_transitions_.load()

                     << ",illegal_run=" << illegal_run_transitions_.load()

                     << ",illegal_pipeline=" << illegal_pipeline_transitions_.load()

                     << ",v_pkt_deq=" << video_packet_dequeue_count_.load()

                     << ",v_send_ok=" << video_send_packet_ok_.load()

                     << ",v_send_ret=" << video_send_packet_last_ret_.load()

                     << ",v_send_eagain=" << decode_video_send_eagain_.load()

                     << ",a_send_eagain=" << decode_audio_send_eagain_.load()

                     << ",v_drain=" << video_decoder_drain_signals_.load()

                     << ",a_drain=" << audio_decoder_drain_signals_.load() << ")"

                     << " dec(sched v=" << scheduler_stats.video_decoded_frames

                     << ",a=" << scheduler_stats.audio_decoded_frames << ")"

                     << " frame_q(v=" << video_frame_queue_stats.size << "/" << video_frame_queue_stats.capacity

                     << ",peak=" << video_frame_queue_stats.peak_size

                     << ",tmo=" << video_frame_queue_stats.push_timeout_count

                     << ";a=" << audio_frame_queue_stats.size << "/" << audio_frame_queue_stats.capacity

                     << ",peak=" << audio_frame_queue_stats.peak_size

                     << ",tmo=" << audio_frame_queue_stats.push_timeout_count << ")"

                     << " render(out=" << scheduler_stats.rendered_frames

                     << ",late_drop=" << scheduler_stats.dropped_late_frames

                     << ",wait=" << scheduler_stats.wait_events

                     << ",cb=" << render_frames_.load() << ")"

                     << " sched(bp_v=" << scheduler_stats.video_backpressure_events

                     << ",bp_a=" << scheduler_stats.audio_backpressure_events

                     << ",bp_v_ms=" << scheduler_stats.video_backpressure_wait_ms

                     << ",bp_a_ms=" << scheduler_stats.audio_backpressure_wait_ms

                     << ",restart_v=" << scheduler_stats.video_restart_attempts

                     << ",restart_a=" << scheduler_stats.audio_restart_attempts

                     << ",restart_r=" << scheduler_stats.render_restart_attempts

                     << ",limit_v=" << scheduler_stats.video_restart_limit_hits

                     << ",limit_a=" << scheduler_stats.audio_restart_limit_hits

                     << ",limit_r=" << scheduler_stats.render_restart_limit_hits << ")"

                     << " video(path_native=" << video_native_output_frames_.load()

                     << ",copyback=" << video_copy_back_frames_.load()

                     << ",copyback_us=" << video_copy_back_time_us_total_.load()

                     << ",swscale=" << video_swscale_frames_.load()

                     << ",swscale_us=" << video_swscale_time_us_total_.load()

                     << ",filter_block=" << video_filter_blocked_native_frames_.load() << ")"

                     << " display(copy=" << renderer_diagnostics.display_copy_frames

                     << ",copy_us=" << renderer_diagnostics.display_copy_time_us_total

                     << ",copy_bytes=" << renderer_diagnostics.display_copy_bytes << ")"

                     << " audio(submit=" << audio_submitted_frames_.load()

                     << ",play_pts=" << (audio_player_ ? audio_player_->getPlaybackPts() : 0.0) << ")"

                     << " clock(a=" << clock_.getAudioClock()

                     << ",v=" << clock_.getVideoClock()

                     << ",m=" << clock_.getTime() << ")");

            return;

        }

    }

}



/// 从视频包队列和解码器内部缓存提取一帧，并规整为渲染输出格式。

bool PlayerCore::decodeVideoFrame(VideoFrame& out) {

    if (!video_codec_ctx_ || !video_packet_queue_) {

        return false;

    }



    std::lock_guard<std::recursive_mutex> codec_lock(video_codec_mutex_);

    const auto maybe_log_decode_issue = [&](const std::string& reason) {

        const int64_t now_ms = nowSteadyMs();

        int64_t last_ms = last_video_decode_issue_log_ms_.load();

        while (last_ms == 0 || now_ms - last_ms >= 1000) {

            if (last_video_decode_issue_log_ms_.compare_exchange_weak(last_ms, now_ms)) {

                LOG_WARNING("Video decode stalled: " << reason

                            << " backend=" << decoder::DecoderFactory::backendName(video_decoder_backend_)

                            << " pkt_q=" << video_packet_queue_->size()

                            << " eof=" << (video_packet_queue_->isEof() ? "true" : "false")

                            << " timeline=" << currentTimelineSerial()

                            << " accepted=" << acceptedTimelineSerial()

                            << " decode_ok=" << decode_video_ok_.load()

                            << " pkt_deq=" << video_packet_dequeue_count_.load()

                            << " send_ok=" << video_send_packet_ok_.load()

                            << " last_send_ret=" << video_send_packet_last_ret_.load()

                            << " send_eagain=" << decode_video_send_eagain_.load()

                            << " draining=" << (video_decoder_draining_ ? "true" : "false"));

                break;

            }

        }

    };



    while (true) {

        av_frame_unref(out.frame);

        int ret = avcodec_receive_frame(video_codec_ctx_, out.frame);

        if (ret >= 0) {

            const TimelineSerial frame_serial = video_decoder_packet_serial_;

            if (!isAcceptedTimelineSerial(frame_serial)) {

                stale_video_frames_dropped_.fetch_add(1);

                maybeLogDiagnostics("vdec-stale-frame");

                continue;

            }

            if (!prepareVideoOutputFrame(out.frame, out)) {

                const AVPixelFormat format = static_cast<AVPixelFormat>(out.frame->format);

                const char* format_name = av_get_pix_fmt_name(format);

                LOG_WARNING("prepareVideoOutputFrame failed for decoded video frame: format="

                            << (format_name ? format_name : "unknown")

                            << " size=" << out.frame->width << "x" << out.frame->height

                            << " backend=" << decoder::DecoderFactory::backendName(video_decoder_backend_));

                return handleRuntimeFailure(ErrorCode::DecoderInitFailed,
                                            "failed to prepare video output frame",
                                            "video output frame prepare failed",
                                            FailureRecoveryPolicy::FailSession);

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

            out.serial = frame_serial;

            decode_video_ok_.fetch_add(1);

            maybeLogDiagnostics(video_decoder_draining_ ? "vdec-drain" : "vdec-recv");

            return true;

        }

        if (ret == AVERROR_EOF) {

            return false;

        }

        if (ret != AVERROR(EAGAIN)) {

            LOG_WARNING("avcodec_receive_frame failed for video decoder backend "

                        << decoder::DecoderFactory::backendName(video_decoder_backend_)

                        << ": " << avErrorToString(ret) << " (" << ret << ")");

            return handleRuntimeFailure(ErrorCode::DecoderInitFailed,
                                        "video decoder receive_frame failed",
                                        "video decoder receive failure",
                                        FailureRecoveryPolicy::StopPlayback);

        }



        DemuxPacket demux_packet;

        if (video_packet_queue_->pop(demux_packet, 20) && demux_packet.packet) {

            video_packet_dequeue_count_.fetch_add(1);

            if (!isAcceptedTimelineSerial(demux_packet.serial)) {

                stale_video_packets_dropped_.fetch_add(1);

                maybeLogDiagnostics("vdec-stale-packet");

                continue;

            }

            if (envFlagEnabled("MVP_FORCE_FAIL_SESSION_ON_VIDEO_DECODE") &&
                runtime_failure_fail_sessions_.load() == 0 &&
                video_send_packet_ok_.load() == 0) {

                LOG_WARNING("Forced FailSession injection at video decode boundary"
                            << " serial=" << demux_packet.serial
                            << " timeline=" << currentTimelineSerial()
                            << " accepted=" << acceptedTimelineSerial());

                return handleRuntimeFailure(ErrorCode::DecoderInitFailed,
                                            "forced fail-session injection (video decode)",
                                            "forced fail-session video decode injection",
                                            FailureRecoveryPolicy::FailSession);

            }

            video_decoder_draining_ = false;

            bool expected = false;

            if (first_video_send_started_logged_.compare_exchange_strong(expected, true)) {

                LOG_INFO("Video decode first send_packet start: backend="

                         << decoder::DecoderFactory::backendName(video_decoder_backend_)

                         << " packet_size=" << demux_packet.packet->size

                         << " pts=" << demux_packet.packet->pts

                         << " dts=" << demux_packet.packet->dts

                         << " serial=" << demux_packet.serial);

            }

            const bool offthread_send_diag =

                video_decoder_backend_ == decoder::DecoderBackend::Software &&

                envFlagEnabled("MVP_SOFTWARE_VIDEO_SEND_OFFTHREAD");

            if (offthread_send_diag) {

                AVPacket* offthread_packet = av_packet_alloc();

                if (!offthread_packet || av_packet_ref(offthread_packet, demux_packet.packet.get()) < 0) {

                    if (offthread_packet) {

                        av_packet_free(&offthread_packet);

                    }

                    LOG_WARNING("Failed to clone software video packet for offthread send probe");

                    return handleRuntimeFailure(ErrorCode::DecoderInitFailed,
                                                "failed to clone software video probe packet",
                                                "software video probe clone failure",
                                                FailureRecoveryPolicy::FailSession);

                }



                std::atomic<bool> send_completed{false};

                std::atomic<int> send_ret_atomic{kVideoSendPacketRetNotSet};

                std::thread send_thread([this, offthread_packet, &send_completed, &send_ret_atomic] {

                    const int thread_ret = avcodec_send_packet(video_codec_ctx_, offthread_packet);

                    send_ret_atomic.store(thread_ret);

                    send_completed.store(true);

                    AVPacket* packet_to_free = offthread_packet;

                    av_packet_free(&packet_to_free);

                });



                const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(500);

                while (std::chrono::steady_clock::now() < deadline && !send_completed.load()) {

                    std::this_thread::sleep_for(std::chrono::milliseconds(2));

                }



                if (send_completed.load()) {

                    send_thread.join();

                    ret = send_ret_atomic.load();

                } else {

                    LOG_WARNING("Offthread software video send_packet probe timed out after 500ms"

                                << " pkt_q=" << video_packet_queue_->size()

                                << " decode_ok=" << decode_video_ok_.load()

                                << " pkt_deq=" << video_packet_dequeue_count_.load());

                    send_thread.detach();

                    return handleRuntimeFailure(ErrorCode::DecoderInitFailed,
                                                "software video send probe timed out",
                                                "software video send probe timeout",
                                                FailureRecoveryPolicy::FailSession);

                }

            } else {

                ret = avcodec_send_packet(video_codec_ctx_, demux_packet.packet.get());

            }

            video_send_packet_last_ret_.store(ret);

            expected = false;

            if (first_video_send_completed_logged_.compare_exchange_strong(expected, true)) {

                LOG_INFO("Video decode first send_packet returned: backend="

                         << decoder::DecoderFactory::backendName(video_decoder_backend_)

                         << " ret=" << ret

                         << " message=" << avErrorToString(ret)

                         << " serial=" << demux_packet.serial);

            }

            if (ret == AVERROR(EAGAIN)) {

                decode_video_send_eagain_.fetch_add(1);

                continue;

            }

            if (ret == AVERROR_EOF) {

                video_decoder_draining_ = true;

                return false;

            }

            if (ret < 0) {

                LOG_WARNING("avcodec_send_packet failed for video decoder backend "

                            << decoder::DecoderFactory::backendName(video_decoder_backend_)

                            << ": " << avErrorToString(ret) << " (" << ret << ")");

                return handleRuntimeFailure(ErrorCode::DecoderInitFailed,
                                            "video decoder send_packet failed",
                                            "video decoder send failure",
                                            FailureRecoveryPolicy::StopPlayback);

            }

            video_decoder_packet_serial_ = demux_packet.serial;

            video_send_packet_ok_.fetch_add(1);

            continue;

        }



        if (!video_packet_queue_->isEof()) {

            maybe_log_decode_issue("packet pop timed out without EOF");

            return false;

        }

        if (video_decoder_draining_) {

            return false;

        }



        ret = avcodec_send_packet(video_codec_ctx_, nullptr);

        if (ret == AVERROR(EAGAIN)) {

            decode_video_send_eagain_.fetch_add(1);

            continue;

        }

        if (ret == AVERROR_EOF) {

            video_decoder_draining_ = true;

            return false;

        }

        if (ret < 0) {

            LOG_WARNING("avcodec_send_packet(nullptr) failed for video decoder backend "

                        << decoder::DecoderFactory::backendName(video_decoder_backend_)

                        << ": " << avErrorToString(ret) << " (" << ret << ")");

            return false;

        }

        video_decoder_draining_ = true;

        video_decoder_drain_signals_.fetch_add(1);

    }

}



/// 从音频包队列解码一帧 PCM，并重采样到 SDL 输出格式。

bool PlayerCore::decodeAudioFrame(AudioFrame& out) {

    if (!audio_codec_ctx_ || !audio_packet_queue_ || !audio_player_ || !audio_player_->isInitialized()) {

        return false;

    }



    std::lock_guard<std::recursive_mutex> codec_lock(audio_codec_mutex_);

    AVFrame* frame = av_frame_alloc();

    if (!frame) {

        return handleRuntimeFailure(ErrorCode::DecoderInitFailed,
                                    "failed to allocate audio decode frame",
                                    "audio decode frame alloc failure",
                                    FailureRecoveryPolicy::FailSession);

    }

    TimelineSerial frame_serial = kInvalidTimelineSerial;



    while (true) {

        av_frame_unref(frame);

        int ret = avcodec_receive_frame(audio_codec_ctx_, frame);

        if (ret >= 0) {

            frame_serial = audio_decoder_packet_serial_;

            if (!isAcceptedTimelineSerial(frame_serial)) {

                stale_audio_frames_dropped_.fetch_add(1);

                maybeLogDiagnostics("adec-stale-frame");

                continue;

            }

            break;

        }

        if (ret == AVERROR_EOF) {

            av_frame_free(&frame);

            return false;

        }

        if (ret != AVERROR(EAGAIN)) {

            av_frame_free(&frame);

            return handleRuntimeFailure(ErrorCode::DecoderInitFailed,
                                        "audio decoder receive_frame failed",
                                        "audio decoder receive failure",
                                        FailureRecoveryPolicy::StopPlayback);

        }



        DemuxPacket demux_packet;

        if (audio_packet_queue_->pop(demux_packet, 20) && demux_packet.packet) {

            if (!isAcceptedTimelineSerial(demux_packet.serial)) {

                stale_audio_packets_dropped_.fetch_add(1);

                maybeLogDiagnostics("adec-stale-packet");

                continue;

            }

            audio_decoder_draining_ = false;

            ret = avcodec_send_packet(audio_codec_ctx_, demux_packet.packet.get());

            if (ret == AVERROR(EAGAIN)) {

                decode_audio_send_eagain_.fetch_add(1);

                continue;

            }

            if (ret == AVERROR_EOF) {

                audio_decoder_draining_ = true;

                av_frame_free(&frame);

                return false;

            }

            if (ret < 0) {

                av_frame_free(&frame);

                return handleRuntimeFailure(ErrorCode::DecoderInitFailed,
                                            "audio decoder send_packet failed",
                                            "audio decoder send failure",
                                            FailureRecoveryPolicy::StopPlayback);

            }

            audio_decoder_packet_serial_ = demux_packet.serial;

            continue;

        }



        if (!audio_packet_queue_->isEof()) {

            av_frame_free(&frame);

            return false;

        }

        if (audio_decoder_draining_) {

            av_frame_free(&frame);

            return false;

        }



        ret = avcodec_send_packet(audio_codec_ctx_, nullptr);

        if (ret == AVERROR(EAGAIN)) {

            decode_audio_send_eagain_.fetch_add(1);

            continue;

        }

        if (ret == AVERROR_EOF) {

            audio_decoder_draining_ = true;

            av_frame_free(&frame);

            return false;

        }

        if (ret < 0) {

            av_frame_free(&frame);

            return handleRuntimeFailure(ErrorCode::DecoderInitFailed,
                                        "audio decoder drain send_packet failed",
                                        "audio decoder drain send failure",
                                        FailureRecoveryPolicy::StopPlayback);

        }

        audio_decoder_draining_ = true;

        audio_decoder_drain_signals_.fetch_add(1);

    }



    // Resample/convert to the SDL output format expected by AudioPlayer.

    if (!ensureAudioResampler(frame)) {

        av_frame_free(&frame);

        return handleRuntimeFailure(ErrorCode::AudioInitFailed,
                                    "failed to configure audio resampler",
                                    "audio resampler setup failure",
                                    FailureRecoveryPolicy::FailSession);

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

        return handleRuntimeFailure(ErrorCode::AudioInitFailed,
                                    "failed to allocate audio conversion buffer",
                                    "audio conversion buffer alloc failure",
                                    FailureRecoveryPolicy::FailSession);

    }



    const int converted = swr_convert(audio_swr_ctx_, &dst_data, dst_nb_samples,

                                      (const uint8_t**)frame->extended_data, frame->nb_samples);

    if (converted <= 0) {

        av_freep(&dst_data);

        av_frame_free(&frame);

        return handleRuntimeFailure(ErrorCode::AudioInitFailed,
                                    "audio resampler convert failed",
                                    "audio resampler convert failure",
                                    FailureRecoveryPolicy::FailSession);

    }

    // Prefer decoder timing metadata, and fall back to sample math when needed.

    const int byte_count = av_samples_get_buffer_size(

        nullptr, channels, converted, swr_out_sample_fmt_, 1);

    if (byte_count <= 0) {

        av_freep(&dst_data);

        av_frame_free(&frame);

        return handleRuntimeFailure(ErrorCode::AudioInitFailed,
                                    "audio conversion produced invalid byte count",
                                    "audio conversion byte count failure",
                                    FailureRecoveryPolicy::FailSession);

    }



    out.samples.assign(dst_data, dst_data + std::max(0, byte_count));

    out.sample_rate = swr_out_sample_rate_;

    out.channels = channels;

    out.serial = frame_serial;

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

        maybeLogDiagnostics(audio_decoder_draining_ ? "adec-drain" : "adec");

    }



    av_freep(&dst_data);

    av_frame_free(&frame);

    return out.valid;

}



/// 提交一帧视频到渲染器，并同步字幕、OSD、截图和视频时钟。

bool PlayerCore::renderFrame(VideoFrame&& frame) {

    if (!video_renderer_ || !frame.valid || !frame.frame) {

        return false;

    }

    if (!isAcceptedTimelineSerial(frame.serial)) {

        stale_render_frames_dropped_.fetch_add(1);

        maybeLogDiagnostics("render-stale");

        return false;

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

    return true;

}



/// 在渲染空闲期检查 EOF 收尾条件，必要时自动切到停止态。

void PlayerCore::onRenderIdle() {

    const CoreStateSnapshot snapshot = getCoreStateSnapshot();

    if (snapshot.run_state != RunState::Running || !demuxer_ || !demuxer_->isEof()) {

        return;

    }



    setEofReached(true, "render idle observed EOF");

    transitionPipelinePhase(PipelinePhase::Draining, "render idle observed EOF");



    const bool video_done = (!video_packet_queue_ || (video_packet_queue_->isEof() && video_packet_queue_->empty())) &&

                            video_queue_.empty();

    const bool audio_done = (!audio_packet_queue_ || (audio_packet_queue_->isEof() && audio_packet_queue_->empty())) &&

                            audio_queue_.empty();

    const bool audio_device_drained = !audio_player_ || audio_player_->getBufferedSeconds() <= 0.001;

    if (!video_done || !audio_done || !audio_device_drained) {

        return;

    }



    demux_running_.store(false);

    audio_consumer_running_.store(false);

    scheduler_.requestStopAsync();

    if (audio_player_) {

        audio_player_->pause();

    }

    setDeferredStopPending(false, "render idle EOF ended");

    clearPendingSeekSerial("render idle EOF ended");

    setEndedReason(EndedReason::Eof, "render idle EOF ended");

    transitionRunState(RunState::Ended, "render idle EOF ended");

    transitionPipelinePhase(PipelinePhase::Idle, "render idle EOF ended");

    publishPlaybackStateFromInternalState("render idle EOF ended");

    LOG_INFO("Playback reached EOF, entering Ended state");

}



/// 根据当前位置和字幕延迟，计算当前应显示的字幕条目。

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



/// 向外分发状态回调；先复制回调列表以缩短持锁时间。

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



/// 节流分发位置回调，避免高频渲染推进直接压垮 UI 层。

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



/// 广播一次帧渲染完成事件。

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



/// 记录错误日志，并向外广播错误码与说明文本。

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
