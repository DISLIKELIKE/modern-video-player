#include "core/scheduler.h"



#include <algorithm>

#include <chrono>

#include <exception>



#include "logger.h"



namespace vp::core {



namespace {



constexpr int kMaxSchedulerWorkerRestartsPerWindow = 4;

constexpr int64_t kSchedulerWorkerRestartWindowMs = 30000;

constexpr int kSchedulerWorkerRestartCooldownMs = 100;

constexpr double kVideoBackpressureEnterRatio = 0.92;

constexpr double kVideoBackpressureExitRatio = 0.65;

constexpr double kAudioBackpressureEnterRatio = 0.94;

constexpr double kAudioBackpressureExitRatio = 0.70;

constexpr double kAudioMasterWaitChunkMaxSeconds = 0.004;

constexpr double kAudioMasterWaitMinSeconds = 0.001;

constexpr double kAudioMasterWaitSlackMinSeconds = 0.001;

constexpr double kAudioMasterWaitSlackMaxSeconds = 0.003;

constexpr double kFallbackFrameDurationSeconds = 1.0 / 60.0;



int64_t steadyNowMs() {

    return std::chrono::duration_cast<std::chrono::milliseconds>(

               std::chrono::steady_clock::now().time_since_epoch())

        .count();

}



double normalizedFrameDuration(double duration_seconds) {

    if (duration_seconds > 0.0) {

        return std::clamp(duration_seconds, 1.0 / 240.0, 0.100);

    }

    return kFallbackFrameDurationSeconds;

}



double audioMasterEarlySlackSeconds(const VideoFrame& frame) {

    return std::clamp(normalizedFrameDuration(frame.duration) * 0.20,

                      kAudioMasterWaitSlackMinSeconds,

                      kAudioMasterWaitSlackMaxSeconds);

}



double audioMasterLateDropThresholdSeconds(const VideoFrame& frame,

                                           const FrameQueue<VideoFrame>* video_queue) {

    const double frame_duration = normalizedFrameDuration(frame.duration);

    const double fill_ratio = video_queue ? std::clamp(video_queue->getFillRatio(), 0.0, 1.0) : 0.0;

    const double threshold = frame_duration * (1.25 + (1.0 - fill_ratio));

    return -std::clamp(threshold, 0.030, 0.120);

}



bool allowsDecoderWork(const SchedulerControlSnapshot& control) {

    const bool run_state_allows = control.run_state == SchedulerRunState::Starting ||

                                  control.run_state == SchedulerRunState::Running;

    if (!run_state_allows) {

        return false;

    }

    return control.pipeline_phase != SchedulerPipelinePhase::Seeking &&

           control.pipeline_phase != SchedulerPipelinePhase::Flushing;

}



bool allowsRenderWork(const SchedulerControlSnapshot& control) {

    if (control.run_state == SchedulerRunState::Ended) {

        return control.ended_policy == SchedulerEndedPolicy::HoldLastFrame ||
               control.ended_policy == SchedulerEndedPolicy::HoldLastFrameNoClockSync;

    }

    const bool run_state_allows = control.run_state == SchedulerRunState::Starting ||

                                  control.run_state == SchedulerRunState::Running;

    if (!run_state_allows) {

        return false;

    }

    return control.pipeline_phase == SchedulerPipelinePhase::Idle ||

           control.pipeline_phase == SchedulerPipelinePhase::Normal ||

           control.pipeline_phase == SchedulerPipelinePhase::Draining;

}



bool isAudioMasterActive(const SchedulerControlSnapshot& control) {

    if (!control.audio_master_sync_active || !control.audio_output_initialized) {

        return false;

    }

    switch (control.audio_master_policy) {

    case SchedulerAudioMasterPolicy::Disabled:

        return false;

    case SchedulerAudioMasterPolicy::SoftWhenAudioReady:

        return true;

    case SchedulerAudioMasterPolicy::RequireBufferedAudio:

        return control.audio_buffered_seconds > 0.0005;

    default:

        return false;

    }

}



bool isVideoMasterActive(const SchedulerControlSnapshot& control) {

    switch (control.clock_policy) {

    case SchedulerClockPolicy::VideoMaster:

        return true;

    case SchedulerClockPolicy::AudioMaster:

    case SchedulerClockPolicy::SystemMonotonic:

        return false;

    case SchedulerClockPolicy::UseClockSource:

    default:

        return control.clock_source == ClockSource::Video;

    }

}



bool shouldApplyClockSync(const SchedulerControlSnapshot& control) {

    return !(control.run_state == SchedulerRunState::Ended &&

             control.ended_policy == SchedulerEndedPolicy::HoldLastFrameNoClockSync);

}



bool frameMatchesAcceptedTimeline(const VideoFrame& frame, const SchedulerControlSnapshot& control) {

    if (control.accepted_timeline_serial == kInvalidTimelineSerial || frame.serial == kInvalidTimelineSerial) {

        return true;

    }

    return frame.serial == control.accepted_timeline_serial;

}



}  // namespace



Scheduler::Scheduler() = default;



Scheduler::~Scheduler() {

    stop();

}



void Scheduler::setVideoDecoder(std::function<bool(VideoFrame&)> decoder) {

    video_decoder_ = std::move(decoder);

}



void Scheduler::setAudioDecoder(std::function<bool(AudioFrame&)> decoder) {

    audio_decoder_ = std::move(decoder);

}



void Scheduler::setVideoQueue(FrameQueue<VideoFrame>* queue) {

    video_queue_ = queue;

}



void Scheduler::setAudioQueue(FrameQueue<AudioFrame>* queue) {

    audio_queue_ = queue;

}



void Scheduler::setRenderCallback(std::function<bool(VideoFrame&&)> callback) {

    render_callback_ = std::move(callback);

}



void Scheduler::setIdleCallback(std::function<void()> callback) {

    idle_callback_ = std::move(callback);

}



void Scheduler::setClock(Clock* clock) {

    clock_ = clock;

}



void Scheduler::setControlSnapshotProvider(std::function<SchedulerControlSnapshot()> provider) {

    control_snapshot_provider_ = std::move(provider);

}



void Scheduler::start() {

    if (!running_.load()) {

        if (video_thread_.joinable()) {

            video_thread_.join();

        }

        if (audio_thread_.joinable()) {

            audio_thread_.join();

        }

        if (render_thread_.joinable()) {

            render_thread_.join();

        }

    }



    if (running_.exchange(true)) {

        paused_.store(false);

        return;

    }



    paused_.store(false);

    video_restart_budget_.total_restarts.store(0);

    video_restart_budget_.window_restarts.store(0);

    video_restart_budget_.window_start_ms.store(0);

    video_restart_budget_.limit_hits.store(0);

    audio_restart_budget_.total_restarts.store(0);

    audio_restart_budget_.window_restarts.store(0);

    audio_restart_budget_.window_start_ms.store(0);

    audio_restart_budget_.limit_hits.store(0);

    render_restart_budget_.total_restarts.store(0);

    render_restart_budget_.window_restarts.store(0);

    render_restart_budget_.window_start_ms.store(0);

    render_restart_budget_.limit_hits.store(0);

    video_decoded_frames_.store(0);

    audio_decoded_frames_.store(0);

    rendered_frames_.store(0);

    dropped_late_frames_.store(0);

    wait_events_.store(0);

    video_backpressure_events_.store(0);

    audio_backpressure_events_.store(0);

    video_backpressure_wait_ms_.store(0);

    audio_backpressure_wait_ms_.store(0);

    last_render_wall_tp_ = std::chrono::steady_clock::time_point{};



    video_thread_ = std::thread([this] {

        runProtectedLoop("video-decode", [this] { videoDecoderLoop(); }, video_restart_budget_);

    });

    audio_thread_ = std::thread([this] {

        runProtectedLoop("audio-decode", [this] { audioDecoderLoop(); }, audio_restart_budget_);

    });

    render_thread_ = std::thread([this] {

        runProtectedLoop("render", [this] { renderLoop(); }, render_restart_budget_);

    });

}



void Scheduler::pause() {

    paused_.store(true);

}



void Scheduler::resume() {

    paused_.store(false);

}



void Scheduler::stop() {

    requestStopAsync();



    if (video_thread_.joinable() && video_thread_.get_id() != std::this_thread::get_id()) {

        video_thread_.join();

    }

    if (audio_thread_.joinable() && audio_thread_.get_id() != std::this_thread::get_id()) {

        audio_thread_.join();

    }

    if (render_thread_.joinable() && render_thread_.get_id() != std::this_thread::get_id()) {

        render_thread_.join();

    }

}



void Scheduler::requestStopAsync() {

    running_.store(false);

    paused_.store(false);

}



void Scheduler::flush() {

    if (video_queue_) {

        video_queue_->flush();

    }

    if (audio_queue_) {

        audio_queue_->flush();

    }

}



size_t Scheduler::getVideoQueueSize() const {

    return video_queue_ ? video_queue_->size() : 0;

}



size_t Scheduler::getAudioQueueSize() const {

    return audio_queue_ ? audio_queue_->size() : 0;

}



SchedulerStats Scheduler::getStats() const {

    SchedulerStats stats;

    stats.video_decoded_frames = video_decoded_frames_.load();

    stats.audio_decoded_frames = audio_decoded_frames_.load();

    stats.rendered_frames = rendered_frames_.load();

    stats.dropped_late_frames = dropped_late_frames_.load();

    stats.wait_events = wait_events_.load();

    stats.video_backpressure_events = video_backpressure_events_.load();

    stats.audio_backpressure_events = audio_backpressure_events_.load();

    stats.video_backpressure_wait_ms = video_backpressure_wait_ms_.load();

    stats.audio_backpressure_wait_ms = audio_backpressure_wait_ms_.load();

    stats.video_restart_attempts = static_cast<uint64_t>(std::max(0, video_restart_budget_.total_restarts.load()));

    stats.audio_restart_attempts = static_cast<uint64_t>(std::max(0, audio_restart_budget_.total_restarts.load()));

    stats.render_restart_attempts = static_cast<uint64_t>(std::max(0, render_restart_budget_.total_restarts.load()));

    stats.video_restart_limit_hits = video_restart_budget_.limit_hits.load();

    stats.audio_restart_limit_hits = audio_restart_budget_.limit_hits.load();

    stats.render_restart_limit_hits = render_restart_budget_.limit_hits.load();

    return stats;

}



SchedulerControlSnapshot Scheduler::getControlSnapshot() const {

    if (control_snapshot_provider_) {

        return control_snapshot_provider_();

    }



    SchedulerControlSnapshot snapshot;

    if (running_.load()) {

        snapshot.run_state = paused_.load() ? SchedulerRunState::Paused : SchedulerRunState::Running;

        snapshot.pipeline_phase = SchedulerPipelinePhase::Normal;

        snapshot.clock_source = clock_ ? clock_->getSource() : ClockSource::Audio;

        switch (snapshot.clock_source) {

        case ClockSource::Audio:

            snapshot.clock_policy = SchedulerClockPolicy::AudioMaster;

            break;

        case ClockSource::Video:

            snapshot.clock_policy = SchedulerClockPolicy::VideoMaster;

            break;

        case ClockSource::System:

            snapshot.clock_policy = SchedulerClockPolicy::SystemMonotonic;

            break;

        }

        snapshot.audio_output_initialized = snapshot.clock_source == ClockSource::Audio;

        snapshot.audio_master_policy = snapshot.audio_output_initialized ? SchedulerAudioMasterPolicy::SoftWhenAudioReady

                                                                        : SchedulerAudioMasterPolicy::Disabled;

        snapshot.audio_buffered_seconds = 0.0;

        snapshot.audio_master_sync_active = snapshot.audio_output_initialized;

    }

    return snapshot;

}



void Scheduler::videoDecoderLoop() {

    while (running_.load()) {

        const SchedulerControlSnapshot control = getControlSnapshot();

        if (paused_.load() || !allowsDecoderWork(control)) {

            std::this_thread::sleep_for(std::chrono::milliseconds(5));

            continue;

        }

        if (!video_decoder_ || !video_queue_) {

            std::this_thread::sleep_for(std::chrono::milliseconds(5));

            continue;

        }



        if (video_queue_->getFillRatio() >= kVideoBackpressureEnterRatio) {

            video_backpressure_events_.fetch_add(1);

            const auto wait_start = std::chrono::steady_clock::now();

            while (running_.load() && video_queue_->getFillRatio() > kVideoBackpressureExitRatio) {

                std::this_thread::sleep_for(std::chrono::milliseconds(2));

            }

            const auto waited_ms = std::chrono::duration_cast<std::chrono::milliseconds>(

                                       std::chrono::steady_clock::now() - wait_start)

                                       .count();

            video_backpressure_wait_ms_.fetch_add(static_cast<uint64_t>(std::max<int64_t>(0, waited_ms)));

            if (!running_.load()) {

                return;

            }

        }



        VideoFrame frame;

        if (!video_decoder_(frame) || !frame.valid) {

            std::this_thread::sleep_for(std::chrono::milliseconds(1));

            continue;

        }

        if (video_queue_->push(std::move(frame), std::chrono::milliseconds(20))) {

            video_decoded_frames_.fetch_add(1);

        }

    }

}



void Scheduler::audioDecoderLoop() {

    while (running_.load()) {

        const SchedulerControlSnapshot control = getControlSnapshot();

        if (paused_.load() || !allowsDecoderWork(control)) {

            std::this_thread::sleep_for(std::chrono::milliseconds(5));

            continue;

        }

        if (!audio_decoder_ || !audio_queue_) {

            std::this_thread::sleep_for(std::chrono::milliseconds(5));

            continue;

        }



        if (audio_queue_->getFillRatio() >= kAudioBackpressureEnterRatio) {

            audio_backpressure_events_.fetch_add(1);

            const auto wait_start = std::chrono::steady_clock::now();

            while (running_.load() && audio_queue_->getFillRatio() > kAudioBackpressureExitRatio) {

                std::this_thread::sleep_for(std::chrono::milliseconds(2));

            }

            const auto waited_ms = std::chrono::duration_cast<std::chrono::milliseconds>(

                                       std::chrono::steady_clock::now() - wait_start)

                                       .count();

            audio_backpressure_wait_ms_.fetch_add(static_cast<uint64_t>(std::max<int64_t>(0, waited_ms)));

            if (!running_.load()) {

                return;

            }

        }



        AudioFrame frame;

        if (!audio_decoder_(frame) || !frame.valid) {

            std::this_thread::sleep_for(std::chrono::milliseconds(1));

            continue;

        }

        if (audio_queue_->push(std::move(frame), std::chrono::milliseconds(20))) {

            audio_decoded_frames_.fetch_add(1);

        }

    }

}



void Scheduler::pumpRenderOnce() {

    if (paused_.load()) {

        if (idle_callback_) {

            idle_callback_();

        }

        return;

    }

    if (!video_queue_ || !render_callback_) {

        if (idle_callback_) {

            idle_callback_();

        }

        return;

    }



    SchedulerControlSnapshot control = getControlSnapshot();

    if (!allowsRenderWork(control)) {

        if (idle_callback_) {

            idle_callback_();

        }

        return;

    }



    while (true) {

        VideoFrame frame;

        if (!video_queue_->pop(frame, std::chrono::milliseconds(0))) {

            if (idle_callback_) {

                idle_callback_();

            }

            return;

        }

        if (!frame.valid) {

            continue;

        }

        if (!frameMatchesAcceptedTimeline(frame, control)) {

            continue;

        }



        bool drop_frame = false;

        if (clock_ && shouldApplyClockSync(control)) {

            while (running_.load() && !paused_.load()) {

                const SchedulerControlSnapshot wait_control = getControlSnapshot();

                if (!allowsRenderWork(wait_control)) {

                    if (idle_callback_) {

                        idle_callback_();

                    }

                    return;

                }

                if (!frameMatchesAcceptedTimeline(frame, wait_control)) {

                    drop_frame = true;

                    break;

                }

                if (!shouldApplyClockSync(wait_control)) {

                    break;

                }

                const bool video_master = isVideoMasterActive(wait_control);

                const bool audio_master = isAudioMasterActive(wait_control);



                const double master = clock_->getTime();

                const double diff = frame.pts - master;



                double wait_seconds = 0.0;

                if (diff > 0.0) {

                    if (video_master && frame.duration > 0.0 &&

                        last_render_wall_tp_ != std::chrono::steady_clock::time_point{}) {

                        const auto target_tp =

                            last_render_wall_tp_ + std::chrono::duration_cast<std::chrono::steady_clock::duration>(

                                                       std::chrono::duration<double>(frame.duration));

                        const auto now = std::chrono::steady_clock::now();

                        if (target_tp > now) {

                            wait_seconds =

                                std::chrono::duration_cast<std::chrono::duration<double>>(target_tp - now).count();

                        }

                    } else if (audio_master) {

                        const double early_slack = audioMasterEarlySlackSeconds(frame);

                        if (diff > early_slack) {

                            const double requested_wait = std::min(diff - early_slack, kAudioMasterWaitChunkMaxSeconds);

                            if (requested_wait >= kAudioMasterWaitMinSeconds) {

                                wait_seconds = requested_wait;

                            }

                        }

                    }

                }



                if (wait_seconds > 0.0) {

                    wait_events_.fetch_add(1);

                    std::this_thread::sleep_for(std::chrono::duration<double>(wait_seconds));

                    continue;

                }



                const double late_drop_threshold =

                    video_master ? -0.25

                                 : (audio_master ? audioMasterLateDropThresholdSeconds(frame, video_queue_) : -0.25);

                if (diff < late_drop_threshold) {

                    dropped_late_frames_.fetch_add(1);

                    drop_frame = true;

                }

                break;

            }



            if (paused_.load()) {

                if (idle_callback_) {

                    idle_callback_();

                }

                return;

            }

            if (!running_.load()) {

                return;

            }

            if (drop_frame) {

                continue;

            }

        }



        const bool rendered = render_callback_(std::move(frame));

        if (rendered) {

            rendered_frames_.fetch_add(1);

            last_render_wall_tp_ = std::chrono::steady_clock::now();

            return;

        }

        control = getControlSnapshot();

        if (!allowsRenderWork(control)) {

            if (idle_callback_) {

                idle_callback_();

            }

            return;

        }

        continue;

    }

}



void Scheduler::renderLoop() {

    while (running_.load()) {

        pumpRenderOnce();

        std::this_thread::sleep_for(std::chrono::milliseconds(1));

    }

}



template <typename Func>

void Scheduler::runProtectedLoop(const char* worker_name, Func&& fn, WorkerRestartBudget& restart_budget) {

    while (running_.load()) {

        try {

            fn();

            if (!running_.load()) {

                return;

            }

            LOG_ERROR("Scheduler worker exited unexpectedly [" << worker_name << "]");

        } catch (const std::exception& ex) {

            LOG_ERROR("Scheduler thread crashed [" << worker_name << "]: " << ex.what());

        } catch (...) {

            LOG_ERROR("Scheduler thread crashed with unknown exception [" << worker_name << "]");

        }



        if (!running_.load()) {

            return;

        }



        const int64_t now_ms = steadyNowMs();

        const int64_t window_start_ms = restart_budget.window_start_ms.load();

        if (window_start_ms == 0 || (now_ms - window_start_ms) > kSchedulerWorkerRestartWindowMs) {

            restart_budget.window_start_ms.store(now_ms);

            restart_budget.window_restarts.store(0);

        }



        const int total_count = restart_budget.total_restarts.fetch_add(1) + 1;

        const int window_count = restart_budget.window_restarts.fetch_add(1) + 1;

        if (window_count > kMaxSchedulerWorkerRestartsPerWindow) {

            restart_budget.limit_hits.fetch_add(1);

            LOG_ERROR("Scheduler thread restart budget exhausted [" << worker_name

                      << "] window=" << window_count << "/" << kMaxSchedulerWorkerRestartsPerWindow

                      << " in " << kSchedulerWorkerRestartWindowMs << "ms"

                      << " total=" << total_count);

            running_.store(false);

            return;

        }



        LOG_WARNING("Restarting scheduler worker thread [" << worker_name << "]"

                    << " total=" << total_count

                    << " window=" << window_count << "/" << kMaxSchedulerWorkerRestartsPerWindow);

        std::this_thread::sleep_for(std::chrono::milliseconds(kSchedulerWorkerRestartCooldownMs));

    }

}



}  // namespace vp::core
