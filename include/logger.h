#pragma once

#include <string>

#define LOG_INFO(x) Logger::log(x, std::string(__FILE__), __LINE__, LogLevel::INFO)
#define LOG_WARNING(x) Logger::log(x, std::string(__FILE__), __LINE__, LogLevel::WARNING)
#define LOG_ERROR(x) Logger::log(x, std::string(__FILE__), __LINE__, LogLevel::ERR)
#define LOG_FATAL(x) Logger::log(x, std::string(__FILE__), __LINE__, LogLevel::FATAL)

enum class LogLevel
{
    INFO     = 0,
    WARNING  = 1,
    ERR	     = 2,
    FATAL    = 3
};

namespace Logger
{
    enum LogVerbosity
    {
        VERBOSITY_BASIC     = 0x00,
        VERBOSITY_TIMESTAMP = 0x01,
        VERBOSITY_LEVEL     = 0x02,
        VERBOSITY_FILE      = 0x04,
        VERBOSITY_LINE      = 0x08,
        VERBOSITY_ALL       = 0x0f
    };
    
    typedef void(*CustomStreamCallback)(std::string, LogLevel);
    
    extern void initialize();
    extern void set_verbosity(int flags);
    extern void open_file_stream();
    extern void open_console_stream();
    extern void open_custom_stream(CustomStreamCallback callback);
    extern void close_file_stream();
    extern void close_console_stream();
    extern void close_custom_stream();
	extern void enable_debug_mode();
	extern void disable_debug_mode();
    extern void log(std::string text, std::string file, int line, LogLevel level);
    
    // simplified api for scripting
    extern void log_info(std::string text);
    extern void log_error(std::string text);
    extern void log_warning(std::string text);
    extern void log_fatal(std::string text);
    
    extern void flush();
}
