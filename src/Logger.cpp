//--------------------------------------------------------------
// Main Header 
//--------------------------------------------------------------
#include "Logger.hpp"
//--------------------------------------------------------------
#include "TimeStamp.hpp"
//--------------------------------------------------------------
// Standard cpp library
//--------------------------------------------------------------
#include <chrono>
#include <cstdio>
#include <fstream>
#include <iostream>
//--------------------------------------------------------------
// fmt library (only when std::format isn't used)
//--------------------------------------------------------------
#if !LOGGER_HAS_STD_FORMAT
    #include <fmt/color.h>
    #include <fmt/compile.h>
#endif
//--------------------------------------------------------------
// Format library
//--------------------------------------------------------------
#if LOGGER_HAS_STD_PRINT
    #include <print>
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
#if LOGGER_HAS_STD_FORMAT
    constexpr std::string_view ANSI_COLOR_RED    = "\x1b[31m";
    constexpr std::string_view ANSI_COLOR_YELLOW = "\x1b[33m";
    constexpr std::string_view ANSI_COLOR_GREY   = "\x1b[90m";
    constexpr std::string_view ANSI_COLOR_RESET  = "\x1b[0m";
#endif
//--------------------------------------------------------------
// Public
//--------------------------------------------------------------
Logger::Logger& Logger::Logger::instance(void) {
    static Logger instance;
    return instance;
} // end Logger& Logger::instance(void)
//--------------------------------------------------------------
// Protected
//--------------------------------------------------------------
void Logger::Logger::level_message(const LogRecord& record) const {
    //--------------------------
    switch (record.level) {
        case LogLevel::DEBUG:
#ifdef LOGGER_DEBUG
    #if LOGGER_HAS_STD_PRINT
            std::println("{}{}{}", ANSI_COLOR_GREY, record.formatted_message, ANSI_COLOR_RESET);
    #elif !LOGGER_HAS_STD_FORMAT
            fmt::print(fmt::fg(fmt::color::gray), "{}\n", record.formatted_message);
    #else
            sync_line(std::cout, ANSI_COLOR_GREY, record.formatted_message, ANSI_COLOR_RESET);
    #endif
#endif
            break;
        case LogLevel::ERROR:
#if LOGGER_HAS_STD_PRINT
            // std::println(stderr, ANSI_COLOR_RED "{}" ANSI_COLOR_RESET, message);
            std::println(stderr, "{}{}{}", ANSI_COLOR_RED, record.formatted_message, ANSI_COLOR_RESET);
#elif !LOGGER_HAS_STD_FORMAT
            fmt::print(fmt::fg(fmt::color::red), "{}\n", record.formatted_message);
#else
            sync_line(std::cerr, ANSI_COLOR_RED, record.formatted_message, ANSI_COLOR_RESET);
#endif
            logs(record);
            break;
        case LogLevel::WARNING:
#if LOGGER_HAS_STD_PRINT
            // std::println(stderr, ANSI_COLOR_YELLOW "{}" ANSI_COLOR_RESET, message);
            std::println(stderr, "{}{}{}", ANSI_COLOR_YELLOW, record.formatted_message, ANSI_COLOR_RESET);
#elif !LOGGER_HAS_STD_FORMAT
            fmt::print(fmt::fg(fmt::color::yellow), "{}\n", record.formatted_message);
#else
            sync_line(std::cerr, ANSI_COLOR_YELLOW, record.formatted_message, ANSI_COLOR_RESET);
#endif
            logs(record);
            break;
        case LogLevel::INFO:
        default:
            print_file(record.formatted_message);
            break;
    } // end switch(level)
}// end void Logger::Logger::level_message(const LogRecord& record) const
//--------------------------------------------------------------
void Logger::Logger::sync_line(std::ostream& stream, std::string_view prefix, std::string_view message, std::string_view suffix) const {
    //--------------------------
    thread_local std::string _line;
    _line.clear();
    _line.reserve(prefix.size() + message.size() + suffix.size() + 1UL);
    _line.append(prefix);
    _line.append(message);
    _line.append(suffix);
    _line.push_back('\n');
    //--------------------------
    stream << _line;
    //--------------------------
}// end void Logger::Logger::sync_line(std::ostream& stream, ...)
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
    std::string_view _timestamp;
    TimeStamp::format_timestamp(now, _timestamp);
    //--------------------------
#if LOGGER_HAS_STD_FORMAT
    return std::format("{}{}{}", _timestamp, level_print(level), message);
#else
    return fmt::format(FMT_COMPILE("{}{}{}"), _timestamp, level_print(level), message);
#endif
    //--------------------------
}// end std::string Logger::Logger::format_message(const LogLevel& level, std::string_view message, const std::chrono::system_clock::time_point& now) const
//--------------------------------------------------------------
std::string Logger::Logger::format_message(const LogLevel& level, std::string_view file, std::string_view function_name, std::size_t line, std::string_view message, const std::chrono::system_clock::time_point& now) const {
    //--------------------------
    const std::string_view _function = format_function_name(function_name);
    const std::string_view _file = format_file_name(file);
    //--------------------------
    std::string_view _timestamp;
    TimeStamp::format_timestamp(now, _timestamp);
    //--------------------------
#if LOGGER_HAS_STD_FORMAT
    return std::format("{}{}[{}::{}:{}]: {}", _timestamp, level_print(level), _file, _function, line, message);
#else
    return fmt::format(FMT_COMPILE("{}{}[{}::{}:{}]: {}"), _timestamp, level_print(level), _file, _function, line, message);
#endif
    //--------------------------
}// end std::string Logger::Logger::format_message(const LogLevel& level, std::string_view file, std::string_view function_name, std::size_t line, std::string_view message, const std::chrono::system_clock::time_point& now) const
//--------------------------------------------------------------
void Logger::Logger::log_file(std::string_view filename, std::string_view message, const std::optional<std::chrono::system_clock::time_point>& now) const {
    //--------------------------
    std::ofstream _log_file(filename.data(), std::ios_base::app);
    //--------------------------
    if (!_log_file.is_open()) {
        return;
    }// end if (!_log_file.is_open())
    //--------------------------
    std::string _line;
    //--------------------------
    if (!_log_file.tellp() and now.has_value()) { // empty file: prepend the one-time header
        //--------------------------
        std::string_view _timestamp;
        TimeStamp::format_timestamp(now.value(), _timestamp);
        //--------------------------
        _line.reserve(21UL + _timestamp.size() + message.size() + 2UL);
        _line.append("Log file created at: ");
        _line.append(_timestamp);
        _line.push_back('\n');
        //--------------------------
    } else {
        _line.reserve(message.size() + 1UL);
    } // end if (!_log_file.tellp() and now.has_value())
    //--------------------------
    _line.append(message);
    _line.push_back('\n');
    //--------------------------
    _log_file.write(_line.data(), static_cast<std::streamsize>(_line.size()));
    //--------------------------
}// end void log_file(std::string_view filename, std::string_view message, const std::optional<std::chrono::system_clock::time_point>& now) const
//--------------------------------------------------------------
constexpr std::string_view Logger::Logger::format_function_name(std::string_view function_name) const {
    //--------------------------
    // Find the last "::" occurrence and return only the function name
    const size_t _pos = function_name.rfind("::");
    return (_pos != std::string_view::npos) ? function_name.substr(_pos + 2) : function_name;
    //--------------------------
}// end std::string Logger::Logger::format_function_name(std::string_view function_name) const
//--------------------------------------------------------------
constexpr std::string_view Logger::Logger::format_file_name(std::string_view file_name) const {
    //--------------------------
    // Strip the directory path and return only the base file name (handles both '/' and '\\')
    const size_t _pos = file_name.find_last_of("/\\");
    return (_pos != std::string_view::npos) ? file_name.substr(_pos + 1) : file_name;
    //--------------------------
}// end std::string Logger::Logger::format_file_name(std::string_view file_name) const
//--------------------------------------------------------------
void Logger::Logger::print_file(std::string_view message) const {
#if LOGGER_HAS_STD_PRINT
    std::println("{}", message);
#elif !LOGGER_HAS_STD_FORMAT
    fmt::print("{}\n", message);
#else
    sync_line(std::cout, std::string_view{}, message, std::string_view{});
#endif
}// end void Logger::Logger::print_file(std::string_view message) const
//--------------------------------------------------------------
void Logger::Logger::logs(const LogRecord& record) const {
    //--------------------------
#if HAS_LOGGER_SQL == 1
    static SQLogger& s_sq_logger = SQLogger::instance();
    //--------------------------
    const bool _logged = s_sq_logger.log(record);
    if(!_logged) {
        log_file(level_log(record.level), record.formatted_message, record.now);
    }// end if(!s_sq_logger.log(message, now))
    //--------------------------
#else
    log_file(level_log(record.level), record.formatted_message, record.now);
#endif
}// end void Logger::Logger::logs(const LogRecord& record) const
//--------------------------------------------------------------
