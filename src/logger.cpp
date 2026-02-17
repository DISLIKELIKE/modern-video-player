#include "logger.h"

namespace vp {

bool Logger::initialized_ = false;

void Logger::init() {
#ifdef USE_QUILL_LOGGING
    if (!initialized_) {
        quill::start();
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
    LOG_INFO("%s", msg.c_str());
#else
    std::cout << "[INFO] " << msg << std::endl;
#endif
}

void Logger::warning(const std::string& msg) {
#ifdef USE_QUILL_LOGGING
    LOG_WARNING("%s", msg.c_str());
#else
    std::cout << "[WARNING] " << msg << std::endl;
#endif
}

void Logger::error(const std::string& msg) {
#ifdef USE_QUILL_LOGGING
    LOG_ERROR("%s", msg.c_str());
#else
    std::cerr << "[ERROR] " << msg << std::endl;
#endif
}

void Logger::debug(const std::string& msg) {
#ifdef USE_QUILL_LOGGING
    LOG_DEBUG("%s", msg.c_str());
#else
    std::cout << "[DEBUG] " << msg << std::endl;
#endif
}

} // namespace vp
