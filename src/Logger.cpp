//--------------------------------------------------------------
// Main Header 
//--------------------------------------------------------------
#include "Logger.hpp"
//--------------------------------------------------------------
#include "LogSession.hpp"
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
            logs(record);
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
// The opening partner of the bracket that ends `text`, found by depth from the right; 0 when there is none.
constexpr std::size_t Logger::Logger::opened(std::string_view text, const char open, const char close) const {
    //--------------------------
    std::size_t _depth = 0;
    for (std::size_t _left = text.size(); _left > 0; --_left) {
        const char _c = text[_left - 1];
        _depth += (_c == close) ? 1 : 0;
        if (_c == open) {
            --_depth;
            if (_depth == 0) {
                return _left - 1;
            }// end if (_depth == 0)
        }// end if (_c == open)
    }// end for (_left)
    return 0;
    //--------------------------
}// end constexpr std::size_t Logger::Logger::opened(std::string_view text, const char open, const char close) const
//--------------------------------------------------------------
constexpr std::string_view Logger::Logger::trimmed(std::string_view text) const {
    //--------------------------
    while (!text.empty() and text.back() == ' ') {
        text.remove_suffix(1);
    }// end while (!text.empty() and text.back() == ' ')
    return text;
    //--------------------------
}// end constexpr std::string_view Logger::Logger::trimmed(std::string_view text) const
//--------------------------------------------------------------
// The operator that ends `text` ("operator>", "operator()", "operator new") whole, or an empty view when the
// name is not an operator's; its symbol may hold "<", ">" and "::"-free punctuation that a cut by depth would take
// for brackets.
constexpr std::string_view Logger::Logger::operator_name(std::string_view text) const {
    //--------------------------
    const std::size_t _pos = text.rfind("operator");
    if (_pos == std::string_view::npos) {
        return std::string_view{};
    }// end if (no operator)
    const bool _word = _pos > 0 and (text[_pos - 1] == '_' or (text[_pos - 1] >= '0' and text[_pos - 1] <= '9') or
                                     (text[_pos - 1] >= 'a' and text[_pos - 1] <= 'z') or
                                     (text[_pos - 1] >= 'A' and text[_pos - 1] <= 'Z'));
    const char _next = (_pos + 8 < text.size()) ? text[_pos + 8] : ' ';
    const bool _longer = _next == '_' or (_next >= '0' and _next <= '9') or (_next >= 'a' and _next <= 'z') or
                         (_next >= 'A' and _next <= 'Z');
    return (_word or _longer) ? std::string_view{} : text.substr(_pos);
    //--------------------------
}// end constexpr std::string_view Logger::Logger::operator_name(std::string_view text) const
//--------------------------------------------------------------
constexpr std::string_view Logger::Logger::format_function_name(std::string_view function_name) const {
    //--------------------------
    // GCC and Clang give the whole signature: the return type, the qualified name, the parameters, the
    // qualifiers and a "[with T = ...]" or "[T = ...]" note, and the parameters and the note hold "::" too.
    // So the note, a trailing return type, a GCC lambda's "<lambda(int)>", the parameters and the function's own
    // template arguments are cut by depth first, and only then is the last "::" found. MSVC's __FUNCTION__ is the
    // qualified name alone and passes through.
    std::string_view _name = trimmed(function_name);
    if (!_name.empty() and _name.back() == ']') {
        _name = trimmed(_name.substr(0, opened(_name, '[', ']')));
    }// end if (a template note ends it)
    const std::size_t _arrow = _name.rfind(") -> ");
    if (_arrow != std::string_view::npos) {
        _name = _name.substr(0, _arrow + 1);
    }// end if (a trailing return type)
    if (!_name.empty() and _name.back() == '>') {
        _name = _name.substr(0, opened(_name, '<', '>'));
    }// end if (a GCC lambda ends it)
    while (_name.ends_with("::")) {
        _name.remove_suffix(2);
    }// end while (_name.ends_with("::"))
    const std::size_t _close = _name.rfind(')');
    if (_close != std::string_view::npos) {
        _name = _name.substr(0, opened(_name.substr(0, _close + 1), '(', ')'));
    }// end if (a parameter list)
    const std::string_view _operator = operator_name(_name);
    if (!_operator.empty()) {
        return _operator;
    }// end if (an operator)
    if (!_name.empty() and _name.back() == '>') {
        _name = _name.substr(0, opened(_name, '<', '>'));
    }// end if (the function's own template arguments)
    const std::size_t _pos = _name.rfind("::");
    _name = (_pos != std::string_view::npos) ? _name.substr(_pos + 2) : _name;
    const std::size_t _space = _name.rfind(' ');
    return (_space != std::string_view::npos) ? _name.substr(_space + 1) : _name;
    //--------------------------
}// end constexpr std::string_view Logger::Logger::format_function_name(std::string_view function_name) const
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
    // The text lines carry the file's and the function's short names; so do the rows.
    const LogRecord _row{record.level, record.message, record.formatted_message, record.now,
                         record.file.has_value() ? std::optional<std::string_view>{format_file_name(*record.file)} : std::nullopt,
                         record.function_name.has_value() ? std::optional<std::string_view>{format_function_name(*record.function_name)} : std::nullopt,
                         record.line};
    //--------------------------
    // The live LogSession, if any, stores the row. With none alive the console has the line
    // already and nothing else is written: no database and no file is opened.
    LogSession::store(_row);
    //--------------------------
}// end void Logger::Logger::logs(const LogRecord& record) const
//--------------------------------------------------------------
