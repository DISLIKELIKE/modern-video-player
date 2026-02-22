#pragma once

#include <string>
#include <iostream>

#ifdef USE_QUILL_LOGGING
#include <quill/Quill.h>
#define LOG_INFO(msg) LOG_INFO_L(msg)
#define LOG_WARNING(msg) LOG_WARNING_L(msg)
#define LOG_ERROR(msg) LOG_ERROR_L(msg)
#define LOG_DEBUG(msg) LOG_DEBUG_L(msg)
#else
#define LOG_INFO(msg) std::cout << "[INFO] " << msg << std::endl
#define LOG_WARNING(msg) std::cout << "[WARNING] " << msg << std::endl
#define LOG_ERROR(msg) std::cerr << "[ERROR] " << msg << std::endl
#ifdef DEBUG_MODE
#define LOG_DEBUG(msg) std::cerr << "[DEBUG] " << msg << std::endl
#define LOG_TRACE_VIDEO(msg) std::cerr << "[DEBUG] [VIDEO] " << msg << std::endl
#define LOG_TRACE_AUDIO(msg) std::cerr << "[DEBUG] [AUDIO] " << msg << std::endl
#define LOG_TRACE_EVENT(msg) std::cerr << "[DEBUG] [EVENT] " << msg << std::endl
#define LOG_TRACE_LOOP(msg) std::cerr << "[DEBUG] [LOOP] " << msg << std::endl
#else
#define LOG_DEBUG(msg) do {} while(0)
#define LOG_TRACE_VIDEO(msg) do {} while(0)
#define LOG_TRACE_AUDIO(msg) do {} while(0)
#define LOG_TRACE_EVENT(msg) do {} while(0)
#define LOG_TRACE_LOOP(msg) do {} while(0)
#endif
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
