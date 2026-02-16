#include "logger.h"

namespace vp {

bool Logger::initialized_ = false;

void Logger::init() {
#ifdef USE_QUILL_LOGGING
    if (!initialized_) {
        quill::start();
        
        auto file_handler = quill::FileHandler(
            "video_player.log",
            []() { return quill::RotatingFileHandlerConfig(); },
            FileHandlerNotifier{});
        file_handler->set_log_level(quill::LogLevel::Info);
        quill::Logger* logger = quill::create_logger("root", {file_handler});
        
        auto console_handler = quill::ConsoleHandler(
            "stdout", quill::ConsoleColoursMode::Automatic);
        console_handler->set_log_level(quill::LogLevel::Debug);
        logger->add_handler(console_handler);
        
        initialized_ = true;
    }
#else
    initialized_ = true;
#endif
}

void Logger::shutdown() {
#ifdef USE_QUILL_LOGGING
    if (initialized_) {
        quill::flush();
        initialized_ = false;
    }
#else
    initialized_ = false;
#endif
}

void Logger::info(const std::string& msg) {
#ifdef USE_QUILL_LOGGING
    LOG_INFO(msg);
#else
    std::cout << "[INFO] " << msg << std::endl;
#endif
}

void Logger::warning(const std::string& msg) {
#ifdef USE_QUILL_LOGGING
    LOG_WARNING(msg);
#else
    std::cout << "[WARNING] " << msg << std::endl;
#endif
}

void Logger::error(const std::string& msg) {
#ifdef USE_QUILL_LOGGING
    LOG_ERROR(msg);
#else
    std::cerr << "[ERROR] " << msg << std::endl;
#endif
}

void Logger::debug(const std::string& msg) {
#ifdef USE_QUILL_LOGGING
    LOG_DEBUG(msg);
#else
    std::cout << "[DEBUG] " << msg << std::endl;
#endif
}

} // namespace vp
