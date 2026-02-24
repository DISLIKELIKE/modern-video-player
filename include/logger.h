#pragma once

#include <string>
#include <iostream>
#include <sstream>

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

}
