#include "logger.h"

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
        []()
        {
            quill::ConsoleSinkConfig config;
            config.set_colourise(true);
            config.set_flush_level(quill::LogLevel::TraceL1);
            return config;
        }());

    auto file_sink = quill::Frontend::create_or_get_sink<quill::FileSink>(
        "logs/video_player.log",
        []
        {
            quill::FileSinkConfig config;
            config.set_open_mode('w');
            config.set_flush_interval(100);
            return config;
        }());

    logger_ = quill::Frontend::create_or_get_logger(
        "video_player",
        std::move(console_sink),
        std::move(file_sink));

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
