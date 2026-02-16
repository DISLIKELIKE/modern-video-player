#pragma once

#include <string>

#ifdef USE_QUILL_LOGGING
#include <quill/Quill.h>
#define LOG_INFO(msg) LOG_INFO_L(msg)
#define LOG_WARNING(msg) LOG_WARNING_L(msg)
#define LOG_ERROR(msg) LOG_ERROR_L(msg)
#define LOG_DEBUG(msg) LOG_DEBUG_L(msg)
#else
#include <iostream>
#define LOG_INFO(msg) std::cout << "[INFO] " << msg << std::endl
#define LOG_WARNING(msg) std::cout << "[WARNING] " << msg << std::endl
#define LOG_ERROR(msg) std::cerr << "[ERROR] " << msg << std::endl
#define LOG_DEBUG(msg) std::cout << "[DEBUG] " << msg << std::endl
#endif

namespace vp {

class Logger {
public:
    static void init();
    static void shutdown();
    
    static void info(const std::string& msg);
    static void warning(const std::string& msg);
    static void error(const std::string& msg);
    static void debug(const std::string& msg);
    
private:
    static bool initialized_;
};

} // namespace vp
