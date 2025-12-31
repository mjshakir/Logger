#pragma once
//--------------------------------------------------------------
// Standard cpp library
//--------------------------------------------------------------
#include <chrono>
#include <optional>
#include <string>
#include <string_view>
//--------------------------------------------------------------
// User Defined library
//--------------------------------------------------------------
#include "LoggerAPI.hpp"
#include "LogLevel.hpp"
//--------------------------------------------------------------
namespace Logger {
    //--------------------------------------------------------------
    struct LOGGER_API LogRecord {
        public:
            //--------------------------------------------------------------
            LogRecord(void);
            ~LogRecord(void) = default;
            //--------------------------
            LogRecord(  const LogLevel& level_ = LogLevel::INFO,
                        std::string_view message_ = "",
                        std::string_view formatted_message_ = "",
                        std::optional<std::string_view> function_name_ = std::nullopt,
                        const std::optional<std::chrono::system_clock::time_point>& now_ = std::nullopt) noexcept;
            //--------------------------
            LogRecord(  const LogLevel& level_,
                        std::string_view message_,
                        std::string_view formatted_message_,
                        std::string_view function_name_,
                        const std::optional<std::chrono::system_clock::time_point>& now_ = std::nullopt) noexcept;
            //--------------------------
            LogLevel level;
            std::string_view message;
            std::string_view formatted_message;
            std::optional<std::string_view> function_name;
            std::optional<std::chrono::system_clock::time_point> now;
        //--------------------------------------------------------------
    }; // end struct LogRecord
    //--------------------------------------------------------------
} // namespace Logger
//--------------------------------------------------------------
