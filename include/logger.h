#pragma once

#include <string>
#include <iostream>
#include <sstream>

#ifdef USE_QUILL_LOGGING
#include <quill/LogMacros.h>
#include <quill/Logger.h>

#define LOG_INFO(msg) LOG_INFO(vp::Logger::getLogger(), "{}", msg)
#define LOG_WARNING(msg) LOG_WARNING(vp::Logger::getLogger(), "{}", msg)
#define LOG_ERROR(msg) LOG_ERROR(vp::Logger::getLogger(), "{}", msg)

#ifdef DEBUG_MODE
#define LOG_DEBUG(msg) LOG_DEBUG(vp::Logger::getLogger(), "{}", msg)
#define LOG_TRACE_VIDEO(msg) LOG_TRACE_L1(vp::Logger::getLogger(), "{}", msg)
#define LOG_TRACE_AUDIO(msg) LOG_TRACE_L1(vp::Logger::getLogger(), "{}", msg)
#define LOG_TRACE_EVENT(msg) LOG_TRACE_L1(vp::Logger::getLogger(), "{}", msg)
#define LOG_TRACE_LOOP(msg) LOG_TRACE_L1(vp::Logger::getLogger(), "{}", msg)
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

namespace vp {

class Logger {
public:
    static void init();
    static void shutdown();
    
    static void info(const std::string& msg);
    static void warning(const std::string& msg);
    static void error(const std::string& msg);
    static void debug(const std::string& msg);

#ifdef USE_QUILL_LOGGING
    static quill::Logger* getLogger();
#endif

private:
    static bool initialized_;
};

}
