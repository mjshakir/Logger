//--------------------------------------------------------------
// Main Header 
//--------------------------------------------------------------
#include "Logger.hpp"
//--------------------------------------------------------------
// Standard cpp library
//--------------------------------------------------------------
// #include <iostream>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <ctime>
#if !__cpp_lib_format
    #include <fmt/chrono.h>
    #include <fmt/color.h>
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
void Logger::Logger::level_message(const LogLevel& level, std::string_view message) const {
    //--------------------------
    switch (level) {
        case LogLevel::DEBUG:
#ifdef DEBUG
#if __cpp_lib_format
            std::print("{}\n", message);
#else
            fmt::print("{}\n", message);
#endif
#endif
            break;
        case LogLevel::ERROR:
#if __cpp_lib_format
            std::print(ANSI_COLOR_RED "{}" ANSI_COLOR_RESET "\n", message);
#else
            fmt::print(fmt::fg(fmt::color::red), "{}\n", message);
#endif
            log_file("error_log.txt", message);
            break;
        case LogLevel::WARNING:
#if __cpp_lib_format
            std::print(ANSI_COLOR_YELLOW "{}" ANSI_COLOR_RESET "\n", message);
#else
            fmt::print(fmt::fg(fmt::color::yellow), "{}\n", message);
#endif
            log_file("warning_log.txt", message);
            break;
        case LogLevel::INFO:
#if __cpp_lib_format
            std::print("{}\n", message);
#else
            fmt::print("{}\n", message);
#endif
            break;
    } // end switch(level)
}// end void Logger::Logger::level_message(LogLevel level, std::string_view message)
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
std::string Logger::Logger::format_message(const LogLevel& level, std::string_view message, const std::chrono::system_clock::time_point& now) const {
    //--------------------------
    const auto _localtime = std::chrono::system_clock::to_time_t(now);
    //--------------------------
#if __cpp_lib_format
    return std::format("{:%Y-%m-%d %H:%M:%S}{}{}", *std::localtime(&_localtime), 
                        level_print(level), message);
#else
    return fmt::format("{:%Y-%m-%d %H:%M:%S}{}{}", fmt::localtime(_localtime), 
                        level_print(level), message);
#endif
    //--------------------------
}// end std::string Logger::Logger::format_message(const LogLevel& level, std::string_view message, const std::chrono::system_clock::time_point& now) const
//--------------------------------------------------------------
std::string Logger::Logger::format_message(const LogLevel& level, std::string_view function_name, std::string_view message, const std::chrono::system_clock::time_point& now) const {
    //--------------------------
    const auto _localtime = std::chrono::system_clock::to_time_t(now);
    //--------------------------
#if __cpp_lib_format
    return std::format("{:%Y-%m-%d %H:%M:%S}{}{}: {}", *std::localtime(&_localtime), 
                        level_print(level), format_function_name(function_name), message);
#else
    return fmt::format("{:%Y-%m-%d %H:%M:%S}{}{}: {}", fmt::localtime(_localtime), 
                        level_print(level), format_function_name(function_name), message);
#endif
    //--------------------------
}// end std::string Logger::Logger::format_message(const LogLevel& level, std::string_view function_name, std::string_view message, const std::chrono::system_clock::time_point& now) const
//--------------------------------------------------------------
void Logger::Logger::log_file(const std::string& filename, std::string_view message) const {
    //--------------------------
    std::ofstream _log_file(filename, std::ios_base::app);
    //--------------------------
    if (_log_file.is_open()) {
        if (!_log_file.tellp()) { // Check if the file is empty
            //--------------------------
            const auto now          = std::chrono::system_clock::now();
            const auto localtime    = std::chrono::system_clock::to_time_t(now);
            //--------------------------
#if __cpp_lib_format
            _log_file << std::format("Log file created at: {:%Y-%m-%d %H:%M:%S}\n", *std::localtime(&localtime));
#else
            _log_file << fmt::format("Log file created at: {:%Y-%m-%d %H:%M:%S}\n", fmt::localtime(localtime));
#endif
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
    const size_t pos = function_name.rfind("::");
    return (pos != std::string_view::npos) ? function_name.substr(pos + 2) : function_name;
    //--------------------------
}// end std::string Logger::Logger::format_function_name(std::string_view function_name) const
//--------------------------------------------------------------