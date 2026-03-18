#pragma once

#include <sstream>
#include <string>
#include <string_view>

namespace vp {

enum class LogSeverity {
    Trace = 0,
    Debug,
    Info,
    Warning,
    Error
};

class Logger {
public:
    static void init();
    static void shutdown();

    static void info(const std::string& msg);
    static void warning(const std::string& msg);
    static void error(const std::string& msg);
    static void debug(const std::string& msg);

    static void log(LogSeverity severity, std::string_view category, const std::string& message);

private:
    static void ensureInitialized();
};

}  // namespace vp

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
