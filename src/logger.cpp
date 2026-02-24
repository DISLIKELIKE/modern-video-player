#include "logger.h"

#ifdef USE_QUILL_LOGGING
#include <quill/Backend.h>
#include <quill/Frontend.h>
#include <quill/LogMacros.h>
#include <quill/Logger.h>
#include <quill/sinks/FileSink.h>
#include <quill/sinks/ConsoleSink.h>

namespace vp {

bool Logger::initialized_ = false;
quill::Logger* logger_ = nullptr;

void Logger::init() {
    if (initialized_) {
        return;
    }

    quill::BackendOptions backend_options;
    quill::Backend::start(backend_options);

    auto console_sink = quill::Frontend::create_or_get_sink<quill::ConsoleSink>(
        "console",
        true,
        "stdout");

    quill::FileSinkConfig file_config;
    file_config.set_open_mode('w');

    auto file_sink = quill::Frontend::create_or_get_sink<quill::FileSink>(
        "file_sink",
        "logs/video_player.log",
        file_config);

    logger_ = quill::Frontend::create_or_get_logger(
        "video_player",
        { console_sink, file_sink });

    logger_->set_log_level(quill::LogLevel::TraceL1);

    initialized_ = true;
}

void Logger::shutdown() {
    if (!initialized_) {
        return;
    }

    quill::Backend::stop();
    initialized_ = false;
    logger_ = nullptr;
}

void Logger::info(const std::string& msg) {
    if (logger_) {
        LOG_INFO(logger_, "{}", msg);
    }
}

void Logger::warning(const std::string& msg) {
    if (logger_) {
        LOG_WARNING(logger_, "{}", msg);
    }
}

void Logger::error(const std::string& msg) {
    if (logger_) {
        LOG_ERROR(logger_, "{}", msg);
    }
}

void Logger::debug(const std::string& msg) {
    if (logger_) {
        LOG_DEBUG(logger_, "{}", msg);
    }
}

quill::Logger* Logger::getLogger() {
    return logger_;
}

}

#else

#include <chrono>
#include <iomanip>

namespace vp {

bool Logger::initialized_ = false;

void Logger::init() {
    initialized_ = true;
}

void Logger::shutdown() {
    initialized_ = false;
}

void Logger::info(const std::string& msg) {
    std::cout << "[INFO] " << msg << std::endl;
}

void Logger::warning(const std::string& msg) {
    std::cout << "[WARNING] " << msg << std::endl;
}

void Logger::error(const std::string& msg) {
    std::cerr << "[ERROR] " << msg << std::endl;
}

void Logger::debug(const std::string& msg) {
    std::cout << "[DEBUG] " << msg << std::endl;
}

}
#endif
