//--------------------------------------------------------------
// Main Header 
//--------------------------------------------------------------
#include "Logger.hpp"
//--------------------------------------------------------------
// Standard cpp library
//--------------------------------------------------------------
#include <chrono>
#include <fstream>
#include <iomanip>
#include <ctime>
//--------------------------------------------------------------
// Format library
//--------------------------------------------------------------
#if __cpp_lib_print
    #include <print>
#endif
#if !__cpp_lib_format
    #include <fmt/chrono.h>
    #include <fmt/color.h>
    #include <fmt/compile.h>
#endif
//--------------------------------------------------------------
// User Defined library
//--------------------------------------------------------------
#if HAS_LOGGER_SQL == 1
    #include "SQLogger.hpp"
#endif
//--------------------------------------------------------------
// Definitions
//--------------------------------------------------------------
#if __cpp_lib_format
    #define ANSI_COLOR_RED              "\x1b[31m"
    #define ANSI_COLOR_YELLOW           "\x1b[33m"
    #define ANSI_COLOR_RESET            "\x1b[0m"
#endif 
//--------------------------------------------------------------
Logger::Logger& Logger::Logger::instance(void) {
    static Logger instance;
    return instance;
} // end Logger& Logger::instance(void)
//--------------------------------------------------------------
void Logger::Logger::level_message(const LogLevel& level, std::string_view message, const std::optional<std::chrono::system_clock::time_point>& now) const {
    //--------------------------
    switch (level) {
        case LogLevel::DEBUG:
#ifdef DEBUG
            print_file(message);
#endif
            break;
        case LogLevel::ERROR:
#if __cpp_lib_format
            std::println(ANSI_COLOR_RED "{}" ANSI_COLOR_RESET, message);
#else
            fmt::print(fmt::fg(fmt::color::red), "{}\n", message);
#endif
            logs(level, message, now);
            break;
        case LogLevel::WARNING:
#if __cpp_lib_format
            std::println(ANSI_COLOR_YELLOW "{}" ANSI_COLOR_RESET, message);
#else
            fmt::print(fmt::fg(fmt::color::yellow), "{}\n", message);
#endif
            logs(level, message, now);
            break;
        case LogLevel::INFO:
            print_file(message);
            break;
    } // end switch(level)
}// end void Logger::Logger::level_message(LogLevel level, std::string_view message)
//--------------------------------------------------------------
constexpr std::string_view Logger::Logger::level_name(const LogLevel& level) const {
    //--------------------------
    switch (level) {
        case LogLevel::DEBUG:
            return "DEBUG";
        case LogLevel::ERROR:
            return "ERROR";
        case LogLevel::WARNING:
            return "WARNING";
        case LogLevel::INFO:
            return "INFO";
        default:
            return "UNKNOWN";
    } // end switch(level)
    //--------------------------
}// end std::string Logger::Logger::level_name(const LogLevel& level)
//--------------------------------------------------------------
constexpr std::string_view Logger::Logger::level_print(const LogLevel& level) const {
    //--------------------------
    switch (level) {
        case LogLevel::DEBUG:
            return " [DEBUG]: ";
        case LogLevel::ERROR:
            return " [ERROR]: ";
        case LogLevel::WARNING:
            return " [WARNING]: ";
        case LogLevel::INFO:
            return " [INFO]: ";
        default:
            return " [UNKNOWN]: ";
    } // end switch(level)
    //--------------------------
}// end std::string Logger::Logger::level_print(const LogLevel& level)
//--------------------------------------------------------------
constexpr std::string_view Logger::Logger::level_log(const LogLevel& level) const {
    //--------------------------
    switch (level) {
        case LogLevel::DEBUG:
            return "debug_log.txt";
        case LogLevel::ERROR:
            return "error_log.txt";
        case LogLevel::WARNING:
            return "warning_log.txt";
        case LogLevel::INFO:
            return "info_log.txt";
        default:
            return "UNKNOWN.txt";
    } // end switch(level)
    //--------------------------
}// end std::string Logger::Logger::level_log(const LogLevel& level)
//--------------------------------------------------------------
std::string Logger::Logger::format_message(const LogLevel& level, std::string_view message, const std::chrono::system_clock::time_point& now) const {
    //--------------------------
    std::tm _timeinfo;
    get_time(&_timeinfo, now);
    //--------------------------
#if __cpp_lib_format
    return std::format("{:%Y-%m-%d %H:%M:%S}{}{}", _timeinfo, level_print(level), message);
#else
    return fmt::format(FMT_COMPILE("{:%Y-%m-%d %H:%M:%S}{}{}"), _timeinfo, level_print(level), message);
#endif
    //--------------------------
}// end std::string Logger::Logger::format_message(const LogLevel& level, std::string_view message, const std::chrono::system_clock::time_point& now) const
//--------------------------------------------------------------
std::string Logger::Logger::format_message(const LogLevel& level, std::string_view function_name, std::string_view message, const std::chrono::system_clock::time_point& now) const {
    //--------------------------
    std::tm _timeinfo;
    get_time(&_timeinfo, now);
    //--------------------------
#if __cpp_lib_format
    return std::format("{:%Y-%m-%d %H:%M:%S}{} [{}]: {}", _timeinfo, 
                        level_print(level), format_function_name(function_name), message);
#else
    return fmt::format(FMT_COMPILE("{:%Y-%m-%d %H:%M:%S}{}[{}]: {}"), _timeinfo, 
                        level_print(level), format_function_name(function_name), message);
#endif
    //--------------------------
}// end std::string Logger::Logger::format_message(const LogLevel& level, std::string_view function_name, std::string_view message, const std::chrono::system_clock::time_point& now) const
//--------------------------------------------------------------
void Logger::Logger::log_file(std::string_view filename, std::string_view message, const std::optional<std::chrono::system_clock::time_point>& now) const {
    //--------------------------
    std::ofstream _log_file(filename.data(), std::ios_base::app);
    //--------------------------
    if (_log_file.is_open()) {
        if (!_log_file.tellp()) { // Check if the file is empty
            //--------------------------
            if (now.has_value()) { // Only format time if provided
                std::tm _timeinfo;
                get_time(&_timeinfo, now);
#if __cpp_lib_format 
                _log_file << std::format("Log file created at: {:%Y-%m-%d %H:%M:%S}\n", _timeinfo);
#else
                _log_file << fmt::format("Log file created at: {:%Y-%m-%d %H:%M:%S}\n", _timeinfo);
#endif
            }// end if (now)
        } // end if (!_log_file.tellp())
        //--------------------------
        _log_file << message << std::endl;
        //--------------------------
    }// end if (_log_file.is_open())
    //--------------------------
}// end void log_file(const std::string& filename, std::string_view message) const
//--------------------------------------------------------------
constexpr std::string_view Logger::Logger::format_function_name(std::string_view function_name) const {
    //--------------------------
    // Find the last "::" occurrence and return only the function name
    const size_t _pos = function_name.rfind("::");
    return (_pos != std::string_view::npos) ? function_name.substr(_pos + 2) : function_name;
    //--------------------------
}// end std::string Logger::Logger::format_function_name(std::string_view function_name) const
//--------------------------------------------------------------
void Logger::Logger::print_file(std::string_view message) const {
#if __cpp_lib_format
            std::println("{}", message);
#else
            fmt::print("{}\n", message);
#endif
}// end void Logger::Logger::print_file(std::string_view message) const
//--------------------------------------------------------------
void Logger::Logger::get_time(std::tm* timeinfo, const std::optional<std::chrono::system_clock::time_point>& now) const {
    //--------------------------
    const std::time_t _localtime = (!now.has_value()) ?  std::time(nullptr) : std::chrono::system_clock::to_time_t(now.value());
    //--------------------------
#if defined(_WIN32)
        localtime_s(timeinfo, &_localtime);
#else
        localtime_r(&_localtime, timeinfo);
#endif  
    //--------------------------
}// end void Logger::Logger::get_time(std::tm* timeinfo, const std::optional<std::chrono::system_clock::time_point>& now) const
//--------------------------------------------------------------
void Logger::Logger::logs(const LogLevel& level, std::string_view message, const std::optional<std::chrono::system_clock::time_point>& now) const {
    //--------------------------
#if HAS_LOGGER_SQL == 1
    static SQLogger& s_sq_logger = SQLogger::instance();
    //--------------------------
    if(!s_sq_logger.log(level_name(level), message, now)) {
        log_file(level_log(level), message, now);
    }// end if(!s_sq_logger.log(message, now))
    //--------------------------
#else
    log_file(level_log(level), message, now);
#endif
}// end void Logger::Logger::logs(std::string_view message, const std::optional<std::chrono::system_clock::time_point>& now) const
//--------------------------------------------------------------