#pragma once

#include <string>
#include <iostream>
#include <sstream>

#ifdef USE_QUILL_LOGGING
#include <quill/Backend.h>
#include <quill/Frontend.h>
#include <quill/Logger.h>
#include <quill/sinks/FileSink.h>
#include <quill/sinks/ConsoleSink.h>
#endif

namespace vp {

#ifdef USE_QUILL_LOGGING
class QuillLogger {
public:
    static QuillLogger& instance() {
        static QuillLogger logger;
        return logger;
    }

    void init() {
        if (initialized_) return;

        quill::BackendOptions backend_options;
        quill::Backend::start(backend_options);

        auto console_sink = quill::Frontend::create_or_get_sink<quill::ConsoleSink>(
            "console", true, "stdout");

        auto file_sink = quill::Frontend::create_or_get_sink<quill::FileSink>(
            "file_sink", "logs/video_player.log");

        logger_ = quill::Frontend::create_or_get_logger(
            "video_player", { console_sink, file_sink });

        logger_->set_log_level(quill::LogLevel::TraceL1);
        initialized_ = true;
    }

    void shutdown() {
        if (!initialized_) return;
        quill::Backend::stop();
        initialized_ = false;
    }

    quill::Logger* getLogger() { return logger_; }

private:
    QuillLogger() : initialized_(false), logger_(nullptr) {}
    ~QuillLogger() { shutdown(); }
    QuillLogger(const QuillLogger&) = delete;
    QuillLogger& operator=(const QuillLogger&) = delete;
    bool initialized_;
    quill::Logger* logger_;
};
#endif

class Logger {
public:
    static void init() {
#ifdef USE_QUILL_LOGGING
        QuillLogger::instance().init();
#endif
    }
    
    static void shutdown() {
#ifdef USE_QUILL_LOGGING
        QuillLogger::instance().shutdown();
#endif
    }
    
    static void info(const std::string& msg) {
#ifdef USE_QUILL_LOGGING
        auto logger = QuillLogger::instance().getLogger();
        if (logger) {
            LOG_INFO(logger, "{}", msg);
            return;
        }
#endif
        std::cout << "[INFO] " << msg << std::endl;
    }
    
    static void warning(const std::string& msg) {
#ifdef USE_QUILL_LOGGING
        auto logger = QuillLogger::instance().getLogger();
        if (logger) {
            LOG_WARNING(logger, "{}", msg);
            return;
        }
#endif
        std::cout << "[WARNING] " << msg << std::endl;
    }
    
    static void error(const std::string& msg) {
#ifdef USE_QUILL_LOGGING
        auto logger = QuillLogger::instance().getLogger();
        if (logger) {
            LOG_ERROR(logger, "{}", msg);
            return;
        }
#endif
        std::cerr << "[ERROR] " << msg << std::endl;
    }
    
    static void debug(const std::string& msg) {
#ifdef USE_QUILL_LOGGING
        auto logger = QuillLogger::instance().getLogger();
        if (logger) {
            LOG_DEBUG(logger, "{}", msg);
            return;
        }
#endif
        std::cout << "[DEBUG] " << msg << std::endl;
    }
};

}

#ifdef USE_QUILL_LOGGING
#include <quill/LogMacros.h>

#define LOG_INFO(msg) do { \
    std::ostringstream _oss; _oss << msg; \
    auto _logger = vp::QuillLogger::instance().getLogger(); \
    if (_logger) LOG_INFO(_logger, "{}", _oss.str()); \
} while(0)

#define LOG_WARNING(msg) do { \
    std::ostringstream _oss; _oss << msg; \
    auto _logger = vp::QuillLogger::instance().getLogger(); \
    if (_logger) LOG_WARNING(_logger, "{}", _oss.str()); \
} while(0)

#define LOG_ERROR(msg) do { \
    std::ostringstream _oss; _oss << msg; \
    auto _logger = vp::QuillLogger::instance().getLogger(); \
    if (_logger) LOG_ERROR(_logger, "{}", _oss.str()); \
} while(0)

#ifdef DEBUG_MODE
#define LOG_DEBUG(msg) do { \
    std::ostringstream _oss; _oss << msg; \
    auto _logger = vp::QuillLogger::instance().getLogger(); \
    if (_logger) LOG_DEBUG(_logger, "{}", _oss.str()); \
} while(0)

#define LOG_TRACE_VIDEO(msg) do { \
    std::ostringstream _oss; _oss << msg; \
    auto _logger = vp::QuillLogger::instance().getLogger(); \
    if (_logger) LOG_TRACE_L1(_logger, "{}", _oss.str()); \
} while(0)

#define LOG_TRACE_AUDIO(msg) do { \
    std::ostringstream _oss; _oss << msg; \
    auto _logger = vp::QuillLogger::instance().getLogger(); \
    if (_logger) LOG_TRACE_L1(_logger, "{}", _oss.str()); \
} while(0)

#define LOG_TRACE_EVENT(msg) do { \
    std::ostringstream _oss; _oss << msg; \
    auto _logger = vp::QuillLogger::instance().getLogger(); \
    if (_logger) LOG_TRACE_L1(_logger, "{}", _oss.str()); \
} while(0)

#define LOG_TRACE_LOOP(msg) do { \
    std::ostringstream _oss; _oss << msg; \
    auto _logger = vp::QuillLogger::instance().getLogger(); \
    if (_logger) LOG_TRACE_L1(_logger, "{}", _oss.str()); \
} while(0)
#else
#define LOG_DEBUG(msg) do {} while(0)
#define LOG_TRACE_VIDEO(msg) do {} while(0)
#define LOG_TRACE_AUDIO(msg) do {} while(0)
#define LOG_TRACE_EVENT(msg) do {} while(0)
#define LOG_TRACE_LOOP(msg) do {} while(0)
#endif

#else

#define LOG_INFO(msg) do { std::ostringstream _oss; _oss << msg; std::cout << "[INFO] " << _oss.str() << std::endl; } while(0)
#define LOG_WARNING(msg) do { std::ostringstream _oss; _oss << msg; std::cout << "[WARNING] " << _oss.str() << std::endl; } while(0)
#define LOG_ERROR(msg) do { std::ostringstream _oss; _oss << msg; std::cerr << "[ERROR] " << _oss.str() << std::endl; } while(0)

#ifdef DEBUG_MODE
#define LOG_DEBUG(msg) do { std::ostringstream _oss; _oss << msg; std::cerr << "[DEBUG] " << _oss.str() << std::endl; } while(0)
#define LOG_TRACE_VIDEO(msg) do { std::ostringstream _oss; _oss << msg; std::cerr << "[DEBUG] [VIDEO] " << _oss.str() << std::endl; } while(0)
#define LOG_TRACE_AUDIO(msg) do { std::ostringstream _oss; _oss << msg; std::cerr << "[DEBUG] [AUDIO] " << _oss.str() << std::endl; } while(0)
#define LOG_TRACE_EVENT(msg) do { std::ostringstream _oss; _oss << msg; std::cerr << "[DEBUG] [EVENT] " << _oss.str() << std::endl; } while(0)
#define LOG_TRACE_LOOP(msg) do { std::ostringstream _oss; _oss << msg; std::cerr << "[DEBUG] [LOOP] " << _oss.str() << std::endl; } while(0)
#else
#define LOG_DEBUG(msg) do {} while(0)
#define LOG_TRACE_VIDEO(msg) do {} while(0)
#define LOG_TRACE_AUDIO(msg) do {} while(0)
#define LOG_TRACE_EVENT(msg) do {} while(0)
#define LOG_TRACE_LOOP(msg) do {} while(0)
#endif

#endif
