#pragma once

#include <string>
#include <string_view>
#include <sstream>

namespace vp {

/// 日志严重级别。
enum class LogSeverity {
    Trace = 0,
    Debug,
    Info,
    Warning,
    Error
};

/// 日志门面；对外屏蔽具体日志后端并提供统一类别/级别接口。
class Logger {
public:
    /// 初始化全局日志系统。
    static void init();
    /// 关闭全局日志系统并刷新尾部日志。
    static void shutdown();

    /// 记录一条 Info 日志。
    static void info(const std::string& msg);
    /// 记录一条 Warning 日志。
    static void warning(const std::string& msg);
    /// 记录一条 Error 日志。
    static void error(const std::string& msg);
    /// 记录一条 Debug 日志。
    static void debug(const std::string& msg);

    /// 记录指定级别与类别的日志。
    static void log(LogSeverity severity, std::string_view category, const std::string& message);

private:
    static void ensureInitialized();
};

} // namespace vp

#define VP_BUILD_LOG_MESSAGE(level, category, text) \
    do { \
        std::ostringstream _oss; \
        _oss << text; \
        vp::Logger::log(level, category, _oss.str()); \
    } while (0)

#define LOG_INFO(msg) VP_BUILD_LOG_MESSAGE(vp::LogSeverity::Info, "GENERAL", msg)
#define LOG_WARNING(msg) VP_BUILD_LOG_MESSAGE(vp::LogSeverity::Warning, "GENERAL", msg)
#define LOG_ERROR(msg) VP_BUILD_LOG_MESSAGE(vp::LogSeverity::Error, "GENERAL", msg)

#ifdef DEBUG_MODE
#define LOG_DEBUG(msg) VP_BUILD_LOG_MESSAGE(vp::LogSeverity::Debug, "GENERAL", msg)
#define LOG_TRACE_VIDEO(msg) VP_BUILD_LOG_MESSAGE(vp::LogSeverity::Trace, "VIDEO", msg)
#define LOG_TRACE_AUDIO(msg) VP_BUILD_LOG_MESSAGE(vp::LogSeverity::Trace, "AUDIO", msg)
#define LOG_TRACE_EVENT(msg) VP_BUILD_LOG_MESSAGE(vp::LogSeverity::Trace, "EVENT", msg)
#define LOG_TRACE_LOOP(msg) VP_BUILD_LOG_MESSAGE(vp::LogSeverity::Trace, "LOOP", msg)
#else
#define LOG_DEBUG(msg) do {} while (0)
#define LOG_TRACE_VIDEO(msg) do {} while (0)
#define LOG_TRACE_AUDIO(msg) do {} while (0)
#define LOG_TRACE_EVENT(msg) do {} while (0)
#define LOG_TRACE_LOOP(msg) do {} while (0)
#endif