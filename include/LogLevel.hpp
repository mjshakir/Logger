#pragma once
//--------------------------------------------------------------
// Standard cpp library
//--------------------------------------------------------------
#include <cstdint>
#include <string_view>
//--------------------------------------------------------------
namespace Logger {
    //--------------------------------------------------------------
    enum class LogLevel : uint8_t {
        DEBUG   = 1 << 0,
        ERROR   = 1 << 1,
        WARNING = 1 << 2,
        INFO    = 1 << 3
    }; // end enum class LogLevel : uint8_t
    //--------------------------------------------------------------
    constexpr std::string_view level_name(LogLevel level) noexcept {
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
        }// end switch (level)
    }// end constexpr std::string_view level_name(LogLevel level) noexcept
    //--------------------------------------------------------------
} // namespace Logger
//--------------------------------------------------------------

