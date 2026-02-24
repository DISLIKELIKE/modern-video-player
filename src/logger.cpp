#include "logger.h"

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
