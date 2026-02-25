# Requirements Document

## Introduction

Modern Video Player 需要进行架构重构，将现有的单线程播放逻辑升级为多线程架构，实现 Core API 层、任务调度器和 Filter 插件框架，以提升性能、扩展性和可维护性。

## Glossary

- **Core API**: 播放器核心接口层，提供统一的播放控制、状态管理和事件通知机制
- **Scheduler**: 任务调度器，管理解码线程、渲染线程、音频线程的生命周期和任务分发
- **Filter**: 可插拔的音视频处理插件，支持滤镜、特效、后处理等扩展
- **Frame Queue**: 帧队列，用于线程间传递解码后的音视频帧
- **Pipeline**: 处理管道，将 Filter 串联形成完整的处理链路
- **Clock**: 时钟同步器，基于音频或视频时钟进行音视频同步
- **Command**: 命令对象，封装播放控制操作（play/pause/stop/seek）

## Requirements

### Requirement 1 — Core API Layer

**User Story:** AS an application developer, I want a clean and stable API to control the video player so that I can easily integrate the player into my application.

#### Acceptance Criteria

1. WHEN an application calls `PlayerCore::open(file)`, the system SHALL parse the media file, initialize decoders, and prepare the playback pipeline without starting playback.
2. WHEN an application calls `PlayerCore::play()`, the system SHALL start the scheduler, begin decoding, and render the first video frame within 100ms.
3. WHEN an application calls `PlayerCore::pause()`, the system SHALL pause all decoder threads within 50ms and preserve the current frame for display.
4. WHEN an application calls `PlayerCore::stop()`, the system SHALL stop all threads, flush queues, and reset the playback position to zero within 200ms.
5. WHEN an application calls `PlayerCore::seek(timestamp)`, the system SHALL flush decoder buffers, reposition to the target timestamp, and resume playback from the new position within 500ms.
6. IF the media file is corrupted or unsupported, the system SHALL return an error code and emit an `onError` event with a descriptive message.

### Requirement 2 — Event Notification System

**User Story:** AS an application developer, I want to receive notifications about playback state changes so that I can update my UI accordingly.

#### Acceptance Criteria

1. WHEN playback state changes (playing/paused/stopped), the system SHALL invoke all registered `onStateChanged` callbacks within 10ms.
2. WHEN the playback position updates, the system SHALL invoke `onPositionChanged` callbacks at most once per 100ms.
3. WHEN a video frame is rendered, the system SHALL invoke `onFrameRendered` callbacks if registered.
4. WHEN an error occurs, the system SHALL invoke `onError` callbacks with error code and message.
5. WHILE the application is shutting down, the system SHALL allow safe removal of event callbacks without crashing.

### Requirement 3 — Multi-threaded Scheduler

**User Story:** AS a system architect, I want a scheduler to manage multiple worker threads so that decoding, rendering, and audio playback can run in parallel without blocking each other.

#### Acceptance Criteria

1. WHEN `PlayerCore::play()` is called, the scheduler SHALL spawn a video decoder thread, an audio decoder thread, and a render thread.
2. WHILE the scheduler is running, the video decoder thread SHALL decode frames into the video frame queue without blocking the render thread.
3. WHILE the scheduler is running, the audio decoder thread SHALL decode samples into the audio sample queue without blocking the audio callback.
4. WHEN the video frame queue reaches 80% capacity, the scheduler SHALL pause the video decoder thread until the queue drops below 50%.
5. WHEN the audio sample queue reaches 80% capacity, the scheduler SHALL pause the audio decoder thread until the queue drops below 50%.
6. IF a decoder thread crashes, the scheduler SHALL log the error, notify the application via `onError`, and attempt to restart the thread once.
7. WHEN `PlayerCore::stop()` is called, the scheduler SHALL join all worker threads within 500ms.

### Requirement 4 — Frame Queue System

**User Story:** AS a system architect, I want thread-safe frame queues so that decoder threads can produce frames and render/audio threads can consume them safely.

#### Acceptance Criteria

1. WHEN a decoder thread pushes a frame to a queue, the queue SHALL store the frame and signal waiting consumer threads.
2. WHEN a consumer thread pops a frame from a queue, the queue SHALL return the oldest frame in FIFO order.
3. IF a queue is full, the push operation SHALL block until space is available or return immediately with a timeout status.
4. IF a queue is empty, the pop operation SHALL block until a frame is available or return immediately with a timeout status.
5. WHEN the queue is flushed, all stored frames SHALL be cleared and waiting threads SHALL be notified.

### Requirement 5 — Audio-Video Synchronization

**User Story:** AS an end user, I want audio and video to stay synchronized so that the viewing experience is natural and enjoyable.

#### Acceptance Criteria

1. WHEN audio is present, the system SHALL use the audio clock as the master clock for synchronization.
2. WHEN audio is absent, the system SHALL use the system clock as the master clock.
3. WHILE rendering, the system SHALL compare video frame PTS against the master clock and delay rendering if video is ahead.
4. WHILE rendering, IF the video frame is more than 100ms behind the master clock, the system SHALL drop frames to catch up.
5. WHEN playback speed changes, the system SHALL adjust the clock rate accordingly and maintain synchronization.

### Requirement 6 — Filter Plugin Framework

**User Story:** AS a plugin developer, I want to create and register filter plugins so that I can add custom video/audio processing without modifying the core player.

#### Acceptance Criteria

1. WHEN a filter plugin is registered via `FilterRegistry::registerFilter(name, factory)`, the system SHALL store the factory for later instantiation.
2. WHEN a filter pipeline is created via `PipelineBuilder`, the system SHALL allow chaining multiple filters in a specified order.
3. WHEN a video frame passes through a video filter, the filter SHALL receive the frame, process it, and return the modified frame within 16ms (60fps budget).
4. WHEN an audio sample passes through an audio filter, the filter SHALL receive the sample buffer, process it, and return the modified buffer.
5. IF a filter throws an exception during processing, the system SHALL log the error, bypass the filter, and continue playback.
6. WHEN a filter is disabled, the system SHALL remove it from the processing chain without affecting other filters.

### Requirement 7 — Built-in Filters

**User Story:** AS an end user, I want basic video filters included out of the box so that I can enhance my viewing experience without installing additional plugins.

#### Acceptance Criteria

1. WHEN the brightness filter is applied, the system SHALL adjust pixel luminance by a configurable amount (-100 to +100).
2. WHEN the contrast filter is applied, the system SHALL adjust pixel contrast by a configurable amount (0.5 to 2.0).
3. WHEN the saturation filter is applied, the system SHALL adjust color saturation by a configurable amount (0.0 to 2.0).
4. WHEN a filter parameter is changed during playback, the system SHALL apply the new parameter to subsequent frames within 50ms.

### Requirement 8 — Migration Path

**User Story:** AS a maintainer, I want a gradual migration path from the existing single-threaded player so that I can migrate incrementally without breaking existing functionality.

#### Acceptance Criteria

1. WHEN the new Core API is used alongside the old VideoPlayer, both SHALL coexist in the same build without conflicts.
2. WHEN the migration flag `USE_NEW_PLAYER_CORE` is disabled, the build SHALL produce the same binary as before the migration.
3. WHEN the migration flag `USE_NEW_PLAYER_CORE` is enabled, the build SHALL use the new Core API internally while keeping the `VideoPlayer` interface unchanged.
4. IF a feature is not yet migrated, the system SHALL log a deprecation warning and delegate to the old implementation.
