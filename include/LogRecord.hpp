#pragma once
//--------------------------------------------------------------
// Standard cpp library
//--------------------------------------------------------------
#include <chrono>
#include <cstddef>
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
                        const std::optional<std::chrono::system_clock::time_point>& now_ = std::nullopt,
                        std::optional<std::string_view> file_ = std::nullopt,
                        std::optional<std::string_view> function_name_ = std::nullopt,
                        std::optional<std::size_t> line_ = std::nullopt) noexcept;
            //--------------------------
            LogLevel level;
            std::string_view message, formatted_message;
            std::optional<std::chrono::system_clock::time_point> now;
            std::optional<std::string_view> file, function_name;
            std::optional<std::size_t> line;
        //--------------------------------------------------------------
    }; // end struct LogRecord
    //--------------------------------------------------------------
} // namespace Logger
//--------------------------------------------------------------
