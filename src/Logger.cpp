//--------------------------------------------------------------
// Main Header 
//--------------------------------------------------------------
#include "Logger.hpp"
//--------------------------------------------------------------
// Standard cpp library
//--------------------------------------------------------------
#include <array>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <ctime>
#include <iostream>
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
    // #define ANSI_COLOR_RED              "\x1b[31m"
    // #define ANSI_COLOR_YELLOW           "\x1b[33m"
    // #define ANSI_COLOR_RESET            "\x1b[0m"
    constexpr std::string_view ANSI_COLOR_RED    = "\x1b[31m";
    constexpr std::string_view ANSI_COLOR_YELLOW = "\x1b[33m";
    constexpr std::string_view ANSI_COLOR_RESET  = "\x1b[0m";
#endif
//--------------------------------------------------------------
Logger::Logger& Logger::Logger::instance(void) {
    static Logger instance;
    return instance;
} // end Logger& Logger::instance(void)
//--------------------------------------------------------------
void Logger::Logger::level_message(const LogLevel& level, std::string_view raw_message, std::string_view formatted_message, const std::optional<std::string_view>& function_name, const std::optional<std::chrono::system_clock::time_point>& now) const {
    //--------------------------
    switch (level) {
        case LogLevel::DEBUG:
#ifdef LOGGER_DEBUG
            print_file(formatted_message);
#endif
            break;
        case LogLevel::ERROR:
#if LOGGER_HAS_STD_PRINT
            // std::println(stderr, ANSI_COLOR_RED "{}" ANSI_COLOR_RESET, message);
            std::println(stderr, "{}{}{}", ANSI_COLOR_RED, formatted_message, ANSI_COLOR_RESET);
#elif !LOGGER_HAS_STD_FORMAT
            fmt::print(fmt::fg(fmt::color::red), "{}\n", formatted_message);
#else
            std::cerr << ANSI_COLOR_RED << formatted_message << ANSI_COLOR_RESET << '\n';
#endif
            logs(level, raw_message, formatted_message, function_name, now);
            break;
        case LogLevel::WARNING:
#if LOGGER_HAS_STD_PRINT
            // std::println(stderr, ANSI_COLOR_YELLOW "{}" ANSI_COLOR_RESET, message);
            std::println(stderr, "{}{}{}", ANSI_COLOR_YELLOW, formatted_message, ANSI_COLOR_RESET);
#elif !LOGGER_HAS_STD_FORMAT
            fmt::print(fmt::fg(fmt::color::yellow), "{}\n", formatted_message);
#else
            std::cerr << ANSI_COLOR_YELLOW << formatted_message << ANSI_COLOR_RESET << '\n';
#endif
            logs(level, raw_message, formatted_message, function_name, now);
            break;
        case LogLevel::INFO:
        default:
            print_file(formatted_message);
            break;
    } // end switch(level)
}// end void Logger::Logger::level_message(LogLevel level, std::string_view raw_message, std::string_view formatted_message)
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
std::string Logger::Logger::format_timestamp(const std::tm& timeinfo) const {
    //--------------------------
    std::array<char, 32> _buffer;
    //--------------------------
    const size_t _len = std::strftime(_buffer.data(), _buffer.size(), "%Y-%m-%d %H:%M:%S", &timeinfo);
    if (!_len) {
        return {};
    }// end if (!_len)
    //--------------------------
    return std::string(_buffer.data());
    //--------------------------
}// end std::string Logger::Logger::format_timestamp(const std::tm& timeinfo) const
//--------------------------------------------------------------
std::string Logger::Logger::format_message(const LogLevel& level, std::string_view message, const std::chrono::system_clock::time_point& now) const {
    //--------------------------
    std::tm _timeinfo;
    get_time(&_timeinfo, now);
    //--------------------------
#if LOGGER_HAS_STD_FORMAT
    return std::format("{}{}{}", format_timestamp(_timeinfo), level_print(level), message);
#else
    return fmt::format(FMT_COMPILE("{:%Y-%m-%d %H:%M:%S}{}{}"), _timeinfo, level_print(level), message);
#endif
    //--------------------------
}// end std::string Logger::Logger::format_message(const LogLevel& level, std::string_view message, const std::chrono::system_clock::time_point& now) const
//--------------------------------------------------------------
std::string Logger::Logger::format_message(const LogLevel& level, std::string_view function_name, std::string_view message, const std::chrono::system_clock::time_point& now) const {
    //--------------------------
    const std::string_view _function = format_function_name(function_name);
    //--------------------------
    std::tm _timeinfo;
    get_time(&_timeinfo, now);
    //--------------------------
#if LOGGER_HAS_STD_FORMAT
    return std::format("{}{}[{}]: {}", format_timestamp(_timeinfo), level_print(level), _function, message);
#else
    return fmt::format(FMT_COMPILE("{:%Y-%m-%d %H:%M:%S}{}[{}]: {}"), _timeinfo, level_print(level), _function, message);
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
                //--------------------------
                std::tm _timeinfo;
                get_time(&_timeinfo, now);
                //--------------------------
#if LOGGER_HAS_STD_FORMAT
                _log_file << "Log file created at: " << format_timestamp(_timeinfo) << '\n';
#else
                _log_file << fmt::format("Log file created at: {:%Y-%m-%d %H:%M:%S}\n", _timeinfo);
#endif
            }// end if (now)
        } // end if (!_log_file.tellp())
        //--------------------------
        _log_file << message << '\n';
        //--------------------------
    }// end if (_log_file.is_open())
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
void Logger::Logger::print_file(std::string_view message) const {
#if LOGGER_HAS_STD_PRINT
    std::println("{}", message);
#elif !LOGGER_HAS_STD_FORMAT
    fmt::print("{}\n", message);
#else
    std::cout << message << std::endl;
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
void Logger::Logger::logs(const LogLevel& level, [[maybe_unused]] std::string_view raw_message, std::string_view formatted_message, [[maybe_unused]] const std::optional<std::string_view>& function_name, const std::optional<std::chrono::system_clock::time_point>& now) const {
    //--------------------------
#if HAS_LOGGER_SQL == 1
    static SQLogger& s_sq_logger = SQLogger::instance();
    //--------------------------
    const bool _logged = function_name.has_value() ? s_sq_logger.log(level_name(level), raw_message, function_name.value(), now)
                                                   : s_sq_logger.log(level_name(level), raw_message, now);
    if(!_logged) {
        log_file(level_log(level), formatted_message, now);
    }// end if(!s_sq_logger.log(message, now))
    //--------------------------
#else
    log_file(level_log(level), formatted_message, now);
#endif
}// end void Logger::Logger::logs(std::string_view message, const std::optional<std::chrono::system_clock::time_point>& now) const
//--------------------------------------------------------------
