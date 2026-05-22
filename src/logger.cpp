#if __has_include("logger.h")
#include "logger.h"
#else
#include "../include/logger.h"
#endif

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <system_error>
#include <utility>
#include <vector>

#ifdef USE_QUILL_LOGGING
#define QUILL_DISABLE_NON_PREFIXED_MACROS
#include <quill/Backend.h>
#include <quill/Frontend.h>
#include <quill/Logger.h>
#include <quill/LogMacros.h>
#include <quill/core/QuillError.h>
#include <quill/sinks/ConsoleSink.h>
#include <quill/sinks/RotatingFileSink.h>
#endif

namespace vp {
namespace {

constexpr size_t kDefaultMaxFileSizeMb = 10;
constexpr size_t kDefaultMaxFiles = 5;
constexpr size_t kMinFileSizeMb = 1;
constexpr size_t kMaxFileSizeMb = 1024;
constexpr size_t kMinFileCount = 1;
constexpr size_t kMaxFileCount = 50;

struct ConfigNote {
    LogSeverity severity;
    std::string message;
};

struct LoggingConfig {
    std::filesystem::path log_dir{"logs"};
    size_t max_file_size_bytes{kDefaultMaxFileSizeMb * 1024ull * 1024ull};
    size_t max_files{kDefaultMaxFiles};
    LogSeverity min_severity{LogSeverity::Info};
    bool enable_quill{
#ifdef USE_QUILL_LOGGING
        true
#else
        false
#endif
    };
};

struct ConfigResult {
    LoggingConfig config;
    std::vector<ConfigNote> notes;
};

struct LoggerState {
    LoggingConfig config{};
    bool initialized{false};
    bool backend_started{false};
    bool quill_ready{false};
    bool shutdown_requested{false};
#ifdef USE_QUILL_LOGGING
    quill::Logger* quill_logger{nullptr};
#endif
};

/// 返回全局日志状态单例，集中保存后端配置和运行时句柄。
LoggerState& globalState() {
    static LoggerState state;
    return state;
}

/// 返回一次性初始化标记，保证日志系统只启动一次。
std::once_flag& initFlag() {
    static std::once_flag flag;
    return flag;
}

/// 将日志级别映射为可比较的数值，用于级别过滤。
int severityRank(LogSeverity severity) {
    switch (severity) {
        case LogSeverity::Trace: return 0;
        case LogSeverity::Debug: return 1;
        case LogSeverity::Info: return 2;
        case LogSeverity::Warning: return 3;
        case LogSeverity::Error: return 4;
    }
    return 2;
}

/// 判断当前日志级别是否达到最小输出阈值。
bool shouldEmit(LogSeverity severity, LogSeverity threshold) {
    return severityRank(severity) >= severityRank(threshold);
}

/// 返回日志级别对应的短标签，用于 fallback 输出。
const char* severityLabel(LogSeverity severity) {
    switch (severity) {
        case LogSeverity::Trace: return "TRACE";
        case LogSeverity::Debug: return "DEBUG";
        case LogSeverity::Info: return "INFO";
        case LogSeverity::Warning: return "WARN";
        case LogSeverity::Error: return "ERROR";
    }
    return "INFO";
}

/// 将日志直接写到标准输出/错误输出；作为未启用后端时的兜底路径。
void writeToStdStreams(LogSeverity severity, std::string_view category, const std::string& message) {
    static std::mutex fallback_mutex;
    std::lock_guard<std::mutex> lock(fallback_mutex);
    std::ostream& stream = (severity == LogSeverity::Error) ? std::cerr : std::cout;
    stream << '[' << severityLabel(severity) << "]";
    if (!category.empty()) {
        stream << " [" << category << "]";
    }
    stream << ' ' << message << std::endl;
}

/// 去掉配置项或环境变量值首尾空白。
std::string trimCopy(std::string_view view) {
    size_t start = 0;
    size_t end = view.size();
    while (start < end && std::isspace(static_cast<unsigned char>(view[start])) != 0) {
        ++start;
    }
    while (end > start && std::isspace(static_cast<unsigned char>(view[end - 1])) != 0) {
        --end;
    }
    return std::string(view.substr(start, end - start));
}

/// 将字符串转小写，便于做不区分大小写的配置解析。
std::string toLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

/// 解析无符号整数文本；非法输入返回空。
std::optional<size_t> parseUnsigned(const std::string& text) {
    if (text.empty()) {
        return std::nullopt;
    }
    try {
        size_t idx = 0;
        size_t value = std::stoull(text, &idx, 10);
        if (idx != text.size()) {
            return std::nullopt;
        }
        return value;
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<bool> parseBoolEnvValue(const std::string& text) {
    const std::string normalized = toLower(trimCopy(text));
    if (normalized == "1" || normalized == "true" || normalized == "yes" || normalized == "on") {
        return true;
    }
    if (normalized == "0" || normalized == "false" || normalized == "no" || normalized == "off") {
        return false;
    }
    return std::nullopt;
}

/// Read an environment variable without deprecated CRT APIs on Windows.
std::optional<std::string> readEnvVar(const char* key) {
    if (!key || key[0] == '\0') {
        return std::nullopt;
    }
#if defined(_WIN32)
    char* value = nullptr;
    size_t length = 0;
    if (_dupenv_s(&value, &length, key) != 0 || !value) {
        return std::nullopt;
    }
    std::string copy(value);
    std::free(value);
    return copy;
#else
    const char* value = std::getenv(key);
    if (!value) {
        return std::nullopt;
    }
    return std::string(value);
#endif
}

/// 在记录告警的同时把配置值裁剪到允许范围内。
size_t clampWithWarning(size_t value,
                        size_t min_value,
                        size_t max_value,
                        const std::string& key,
                        std::vector<ConfigNote>& notes) {
    if (value < min_value) {
        notes.push_back({LogSeverity::Warning, key + " is below minimum; clamping to " + std::to_string(min_value)});
        return min_value;
    }
    if (value > max_value) {
        notes.push_back({LogSeverity::Warning, key + " exceeds maximum; clamping to " + std::to_string(max_value)});
        return max_value;
    }
    return value;
}

/// 按环境变量和默认候选路径查找日志配置文件。
std::optional<std::filesystem::path> locateConfigFile() {
    auto resolveIfExists = [](const std::filesystem::path& candidate) -> std::optional<std::filesystem::path> {
        std::error_code ec;
        if (!std::filesystem::exists(candidate, ec) || ec) {
            return std::nullopt;
        }

        std::error_code abs_ec;
        auto absolute_path = std::filesystem::absolute(candidate, abs_ec);
        if (abs_ec) {
            return candidate;
        }
        return absolute_path;
    };

    if (auto env_path = readEnvVar("MVP_LOG_CONFIG")) {
        if (auto resolved = resolveIfExists(std::filesystem::path(*env_path))) {
            return resolved;
        }
    }

    const std::array<std::filesystem::path, 3> candidates = {
        std::filesystem::path("config/logging.conf"),
        std::filesystem::path("../config/logging.conf"),
        std::filesystem::path("../../config/logging.conf")
    };

    for (const auto& candidate : candidates) {
        if (auto resolved = resolveIfExists(candidate)) {
            return resolved;
        }
    }
    return std::nullopt;
}

/// 解析日志级别文本；未知值会回退到 `INFO` 并记告警。
LogSeverity parseLogLevel(const std::string& raw, std::vector<ConfigNote>& notes) {
    static const std::unordered_map<std::string, LogSeverity> level_map = {
        {"trace", LogSeverity::Trace},
        {"debug", LogSeverity::Debug},
        {"info", LogSeverity::Info},
        {"warning", LogSeverity::Warning},
        {"warn", LogSeverity::Warning},
        {"error", LogSeverity::Error}
    };

    auto it = level_map.find(raw);
    if (it != level_map.end()) {
        return it->second;
    }

    notes.push_back({LogSeverity::Warning, "Unknown log_level '" + raw + "'; using INFO"});
    return LogSeverity::Info;
}

/// 从环境变量覆盖日志配置，优先级高于文件配置。
void applyEnvOverrides(LoggingConfig& config, std::vector<ConfigNote>& notes) {
    if (auto disable_quill = readEnvVar("MVP_DISABLE_QUILL_LOGGING")) {
        auto parsed = parseBoolEnvValue(*disable_quill);
        if (parsed && *parsed) {
            config.enable_quill = false;
            notes.push_back({LogSeverity::Info, "MVP_DISABLE_QUILL_LOGGING override applied"});
        } else if (!parsed) {
            notes.push_back({LogSeverity::Warning, "Invalid MVP_DISABLE_QUILL_LOGGING value"});
        }
    }
    if (auto log_dir = readEnvVar("MVP_LOG_DIR")) {
        config.log_dir = trimCopy(*log_dir);
        notes.push_back({LogSeverity::Info, "MVP_LOG_DIR override applied"});
    }
    if (auto level = readEnvVar("MVP_LOG_LEVEL")) {
        config.min_severity = parseLogLevel(toLower(trimCopy(*level)), notes);
        notes.push_back({LogSeverity::Info, "MVP_LOG_LEVEL override applied"});
    }
    if (auto size_mb = readEnvVar("MVP_LOG_MAX_FILE_MB")) {
        auto parsed = parseUnsigned(trimCopy(*size_mb));
        if (parsed) {
            size_t clamped = clampWithWarning(*parsed, kMinFileSizeMb, kMaxFileSizeMb,
                                              "MVP_LOG_MAX_FILE_MB", notes);
            config.max_file_size_bytes = clamped * 1024ull * 1024ull;
            notes.push_back({LogSeverity::Info, "MVP_LOG_MAX_FILE_MB override applied"});
        } else {
            notes.push_back({LogSeverity::Warning, "Invalid MVP_LOG_MAX_FILE_MB value"});
        }
    }
    if (auto files = readEnvVar("MVP_LOG_MAX_FILES")) {
        auto parsed = parseUnsigned(trimCopy(*files));
        if (parsed) {
            size_t clamped = clampWithWarning(*parsed, kMinFileCount, kMaxFileCount,
                                              "MVP_LOG_MAX_FILES", notes);
            config.max_files = clamped;
            notes.push_back({LogSeverity::Info, "MVP_LOG_MAX_FILES override applied"});
        } else {
            notes.push_back({LogSeverity::Warning, "Invalid MVP_LOG_MAX_FILES value"});
        }
    }
}

/// 加载日志配置文件并应用环境变量覆盖，生成最终配置快照。
ConfigResult loadLoggingConfig() {
    ConfigResult result;

#ifndef USE_QUILL_LOGGING
    result.config.enable_quill = false;
    result.notes.push_back({LogSeverity::Info, "USE_QUILL_LOGGING not defined; using stdout/stderr logging."});
    return result;
#else
    auto config_path = locateConfigFile();
    if (config_path) {
        result.notes.push_back({LogSeverity::Info, "Loading logging config from " + config_path->string()});
        std::ifstream input(*config_path);
        if (input.good()) {
            std::string line;
            while (std::getline(input, line)) {
                std::string trimmed = trimCopy(line);
                if (trimmed.empty() || trimmed[0] == '#') {
                    continue;
                }
                auto delimiter = trimmed.find('=');
                if (delimiter == std::string::npos) {
                    result.notes.push_back({LogSeverity::Warning, "Ignoring malformed line in logging.conf: " + trimmed});
                    continue;
                }
                std::string key = toLower(trimCopy(trimmed.substr(0, delimiter)));
                std::string value = trimCopy(trimmed.substr(delimiter + 1));

                if (key == "log_dir") {
                    if (!value.empty()) {
                        result.config.log_dir = value;
                    }
                } else if (key == "max_file_size_mb") {
                    auto parsed = parseUnsigned(value);
                    if (parsed) {
                        size_t clamped = clampWithWarning(*parsed, kMinFileSizeMb, kMaxFileSizeMb,
                                                          "max_file_size_mb", result.notes);
                        result.config.max_file_size_bytes = clamped * 1024ull * 1024ull;
                    } else {
                        result.notes.push_back({LogSeverity::Warning, "Invalid max_file_size_mb value"});
                    }
                } else if (key == "max_files") {
                    auto parsed = parseUnsigned(value);
                    if (parsed) {
                        size_t clamped = clampWithWarning(*parsed, kMinFileCount, kMaxFileCount,
                                                          "max_files", result.notes);
                        result.config.max_files = clamped;
                    } else {
                        result.notes.push_back({LogSeverity::Warning, "Invalid max_files value"});
                    }
                } else if (key == "log_level") {
                    result.config.min_severity = parseLogLevel(toLower(value), result.notes);
                } else {
                    result.notes.push_back({LogSeverity::Warning, "Unknown logging key '" + key + "'"});
                }
            }
        } else {
            result.notes.push_back({LogSeverity::Warning, "Unable to read logging config file"});
        }
    }

    applyEnvOverrides(result.config, result.notes);

    if (!result.config.log_dir.is_absolute()) {
        std::error_code ec;
        auto absolute_path = std::filesystem::absolute(result.config.log_dir, ec);
        if (!ec) {
            result.config.log_dir = absolute_path;
        }
    }

    return result;
#endif
}

#ifdef USE_QUILL_LOGGING
/// 将项目日志级别映射到 Quill 的级别枚举。
quill::LogLevel toQuillLevel(LogSeverity severity) {
    switch (severity) {
        case LogSeverity::Trace: return quill::LogLevel::TraceL1;
        case LogSeverity::Debug: return quill::LogLevel::Debug;
        case LogSeverity::Info: return quill::LogLevel::Info;
        case LogSeverity::Warning: return quill::LogLevel::Warning;
        case LogSeverity::Error: return quill::LogLevel::Error;
    }
    return quill::LogLevel::Info;
}

/// 启动 Quill 后端、创建 sink/logger，并写回运行时状态。
bool startQuill(LoggerState& state, std::vector<ConfigNote>& notes) {
    std::error_code ec;
    if (!std::filesystem::exists(state.config.log_dir, ec)) {
        std::filesystem::create_directories(state.config.log_dir, ec);
    }

    if (ec) {
        notes.push_back({LogSeverity::Warning, "Cannot create log directory '" + state.config.log_dir.string() + "': " + ec.message()});
        return false;
    }

    try {
        quill::BackendOptions backend_options;
        quill::Backend::start(backend_options);
        state.backend_started = true;

        auto console_sink = quill::Frontend::create_or_get_sink<quill::ConsoleSink>(
            "mvp_console_sink", true, "stdout");

        const auto log_file = state.config.log_dir / "modern-video-player.log";
        auto rotating_sink = quill::Frontend::create_or_get_sink<quill::RotatingFileSink>(
            log_file.string(),
            [&state]() {
                quill::RotatingFileSinkConfig cfg;
                cfg.set_open_mode('a');
                cfg.set_rotation_max_file_size(state.config.max_file_size_bytes);
                cfg.set_max_backup_files(static_cast<uint32_t>(state.config.max_files));
                cfg.set_overwrite_rolled_files(true);
                return cfg;
            }());

        std::string pattern = "%(time) [%(log_level)] [%(thread_id)] [%(logger)] %(message)";
        std::string time_fmt = "%Y-%m-%d %H:%M:%S.%Qms";

        state.quill_logger = quill::Frontend::create_or_get_logger(
            "video_player", {std::move(console_sink), std::move(rotating_sink)}, pattern, time_fmt);
        state.quill_logger->set_log_level(toQuillLevel(state.config.min_severity));
        state.quill_ready = true;

        notes.push_back({LogSeverity::Info, "Quill logging enabled; writing to " + log_file.string()});
        return true;
    } catch (const quill::QuillError& error) {
        notes.push_back({LogSeverity::Error, std::string("Failed to start Quill backend: ") + error.what()});
    }

    state.backend_started = false;
    state.quill_logger = nullptr;
    state.quill_ready = false;
    return false;
}
#endif

/// 初始化日志配置与后端，并输出配置阶段产生的提示信息。
void initializeLogger() {
    auto result = loadLoggingConfig();
    auto& state = globalState();
    state.config = result.config;
    state.initialized = true;

#ifdef USE_QUILL_LOGGING
    if (state.config.enable_quill) {
        if (!startQuill(state, result.notes)) {
            result.notes.push_back({LogSeverity::Warning, "Falling back to stdout/stderr logging"});
        }
    }
#endif

    for (const auto& note : result.notes) {
        writeToStdStreams(note.severity, "LOGGER", note.message);
    }
}

/// 根据运行时状态把日志路由到 Quill 或标准输出后端。
void dispatchLog(LogSeverity severity, std::string_view category, const std::string& message) {
    auto& state = globalState();
    if (state.shutdown_requested) {
        writeToStdStreams(severity, category, message);
        return;
    }

    if (!shouldEmit(severity, state.config.min_severity)) {
        return;
    }

#ifdef USE_QUILL_LOGGING
    if (state.quill_ready && state.quill_logger != nullptr) {
        std::string_view safe_category = category.empty() ? std::string_view("GENERAL") : category;
        QUILL_LOG_DYNAMIC(state.quill_logger, toQuillLevel(severity), "[{}] {}", safe_category, message);
        return;
    }
#endif

    writeToStdStreams(severity, category, message);
}

} // namespace

/// 确保日志系统完成一次性初始化。
void Logger::ensureInitialized() {
    std::call_once(initFlag(), []() { initializeLogger(); });
}

/// 初始化全局日志系统，并按配置选择具体日志后端。
void Logger::init() {
    ensureInitialized();
}

/// 关闭日志系统并在支持的后端上做资源回收。
void Logger::shutdown() {
    auto& state = globalState();
    if (!state.initialized || state.shutdown_requested) {
        return;
    }

    state.shutdown_requested = true;

#ifdef USE_QUILL_LOGGING
    if (state.backend_started) {
        quill::Backend::stop();
    }
    state.backend_started = false;
    state.quill_ready = false;
    state.quill_logger = nullptr;
#endif
}

/// 输出 INFO 日志。
void Logger::info(const std::string& msg) {
    ensureInitialized();
    dispatchLog(LogSeverity::Info, "GENERAL", msg);
}

/// 输出 WARNING 日志。
void Logger::warning(const std::string& msg) {
    ensureInitialized();
    dispatchLog(LogSeverity::Warning, "GENERAL", msg);
}

/// 输出 ERROR 日志。
void Logger::error(const std::string& msg) {
    ensureInitialized();
    dispatchLog(LogSeverity::Error, "GENERAL", msg);
}

/// 在调试构建下输出 DEBUG 日志；非调试构建直接忽略。
void Logger::debug(const std::string& msg) {
#ifdef DEBUG_MODE
    ensureInitialized();
    dispatchLog(LogSeverity::Debug, "GENERAL", msg);
#else
    (void)msg;
#endif
}

/// 写入一条日志；内部会根据配置过滤级别并路由到具体后端。
void Logger::log(LogSeverity severity, std::string_view category, const std::string& message) {
    ensureInitialized();
    dispatchLog(severity, category, message);
}

} // namespace vp
