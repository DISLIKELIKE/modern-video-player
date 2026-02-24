# Requirements Document

## Introduction

Modern Video Player 需要重新启用基于 Quill 的企业级日志通道，以同时满足终端调试与合规记录的需求。本功能将实现“方案 A”（双通道异步 + 滚动文件），并提供可配置的日志目录、文件大小、保留数量与日志级别，同时保持现有 `LOG_*` 宏与 `DEBUG_MODE` 控制行为。

## Glossary

- **ConsoleSink**: Quill 内建的标准输出接收器，用于实时终端输出。
- **RotatingFileSink**: 支持最大文件大小与保留数量的 Quill 滚动文件接收器。
- **Quill Backend**: Quill 的异步后台线程，负责刷新日志并管理 sinks。
- **LoggingConfig**: `config/logging.conf` 或环境变量解析得到的结构化配置。
- **USE_QUILL_LOGGING**: CMake 定义，用于切换 Quill 与 stdout 实现。
- **DEBUG_MODE**: CMake 选项，控制调试与 trace 级日志是否生成。

## Requirements

### Requirement 1 — Enterprise Quill Logging Pipeline

**User Story:** AS a platform maintainer, I want the logger to use Quill with asynchronous dual sinks so that operational logs reach both console and rotating files without blocking playback threads.

#### Acceptance Criteria

1. WHEN `USE_QUILL_LOGGING` is enabled and `vp::Logger::init()` runs during process bootstrap, the system SHALL start the Quill backend in asynchronous mode with both `ConsoleSink` and `RotatingFileSink` registered.
2. WHILE log events are emitted through any `LOG_*` macro, the system SHALL enqueue them once and fan out to both sinks preserving severity, timestamp, and ordering metadata.
3. IF the Quill backend cannot start or crashes, the system SHALL emit a single `LOG_ERROR` describing the failure and SHALL continue with the stdout/stderr fallback without terminating the player.

### Requirement 2 — Configurable Logging Parameters

**User Story:** AS a DevOps engineer, I want to configure log directory, log rotation size/count, and log level without recompiling so that deployments can align with enterprise retention rules.

#### Acceptance Criteria

1. WHEN `config/logging.conf` exists, the system SHALL parse the keys `log_dir`, `max_file_size_mb`, `max_files`, and `log_level` before starting Quill, applying the values atomically.
2. WHEN configuration keys are missing, the system SHALL apply documented defaults (`log_dir=logs`, `max_file_size_mb=10`, `max_files=5`, `log_level=info`) and SHALL expose them via `docs/LOGGING.md`.
3. WHEN an environment variable (`MVP_LOG_DIR`, `MVP_LOG_LEVEL`, `MVP_LOG_MAX_FILE_MB`, `MVP_LOG_MAX_FILES`) is present, the system SHALL override the corresponding file value during initialization.
4. IF any configured directory is not writable or disk usage is exhausted, the system SHALL emit a `LOG_WARNING`, create the directory if possible, and OTHERWISE fallback to console-only logging without crashing.
5. WHEN numeric values are invalid (non-numeric or outside `[1, 1024]` MB / `[1, 50]` files), the system SHALL clamp them to defaults and SHALL record the correction via `LOG_WARNING`.

### Requirement 3 — Macro Compatibility and Debug Control

**User Story:** AS an application developer, I want all existing `LOG_*` macros and `DEBUG_MODE` switches to behave identically so that no calling code needs to change when Quill is reintroduced.

#### Acceptance Criteria

1. WHILE the feature flag `USE_QUILL_LOGGING` is defined, the header `include/logger.h` SHALL retain the current macro names, signatures, and include order so that compilation units require no edits.
2. WHILE `DEBUG_MODE` is `OFF`, the system SHALL suppress `LOG_TRACE_*` and `LOG_DEBUG` invocations at compile-time even if the runtime log level is set to `trace`.
3. WHEN `DEBUG_MODE` is `ON` and the configured log level threshold is `trace`, the system SHALL route every `LOG_TRACE_*` call to both sinks; WHEN the level is higher than `trace`, the macros SHALL evaluate to no-ops without formatting overhead.
4. IF the binary is built without `USE_QUILL_LOGGING`, the system SHALL continue writing to stdout/stderr exactly as today and SHALL ignore `config/logging.conf` while surfacing an INFO message that Quill logging is disabled.

### Requirement 4 — Documentation, Testing, and Change Records

**User Story:** AS a release owner, I want documentation, changelog, version records, and automated tests updated so that future engineers understand the new logging controls and regressions are prevented.

#### Acceptance Criteria

1. WHEN the feature ships, `docs/LOGGING.md` SHALL describe the Quill configuration workflow, default/fallback behavior, environment overrides, and troubleshooting steps for permission errors.
2. WHEN documentation is updated, `docs/CHANGELOG.md` SHALL add an entry summarizing the enterprise logging feature, and `docs/VERSION.md` SHALL update the Quill dependency status plus new configuration artifacts.
3. WHEN CI runs, the test suite SHALL include coverage for configuration parsing (valid/missing/invalid values), log rotation triggering at the configured size, combined Console/File sink output, and fallback when Quill is disabled or directories are unwritable.
4. WHEN manual verification occurs, the engineering playbook SHALL instruct developers to validate both sinks by inspecting stdout and the newest rotated file after a sample playback run.
