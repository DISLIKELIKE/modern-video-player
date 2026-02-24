#include "logger.h"

#ifdef USE_QUILL_LOGGING
#include <quill/Frontend.h>
#include <quill/sinks/ConsoleSink.h>
#endif

namespace vp {

bool Logger::initialized_ = false;
quill::Logger* Logger::logger_ = nullptr;

void Logger::init() {
#ifdef USE_QUILL_LOGGING
    if (!initialized_) {
        auto console_sink = quill::Frontend::create_or_get_sink<quill::ConsoleSink>();
        
        logger_ = quill::Frontend::create_or_get_logger("video_player", std::move(console_sink));
        
        quill::Frontend::start();
        initialized_ = true;
    }
#else
    initialized_ = true;
#endif
}

void Logger::shutdown() {
#ifdef USE_QUILL_LOGGING
    if (initialized_) {
        quill::Frontend::flush_log();
        initialized_ = false;
    }
#else
    initialized_ = false;
#endif
}

void Logger::info(const std::string& msg) {
#ifdef USE_QUILL_LOGGING
    LOG_INFO(logger_, "{}", msg);
#else
    std::cout << "[INFO] " << msg << std::endl;
#endif
}

void Logger::warning(const std::string& msg) {
#ifdef USE_QUILL_LOGGING
    LOG_WARNING(logger_, "{}", msg);
#else
    std::cout << "[WARNING] " << msg << std::endl;
#endif
}

void Logger::error(const std::string& msg) {
#ifdef USE_QUILL_LOGGING
    LOG_ERROR(logger_, "{}", msg);
#else
    std::cerr << "[ERROR] " << msg << std::endl;
#endif
}

void Logger::debug(const std::string& msg) {
#ifdef USE_QUILL_LOGGING
    LOG_DEBUG(logger_, "{}", msg);
#else
    std::cout << "[DEBUG] " << msg << std::endl;
#endif
}

} // namespace vp
