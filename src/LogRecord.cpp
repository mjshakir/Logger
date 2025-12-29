//--------------------------------------------------------------
// Main Header 
//--------------------------------------------------------------
#include "LogRecord.hpp"
//--------------------------------------------------------------
Logger::LogRecord::LogRecord(void) :    level(LogLevel::INFO),
                                        message(""),
                                        formatted_message(""),
                                        function_name(std::nullopt),
                                        now(std::nullopt) {
    //--------------------------
}// end Logger::LogRecord::LogRecord(void)
//--------------------------------------------------------------
Logger::LogRecord::LogRecord(   const LogLevel& level_,
                                std::string_view message_,
                                std::string_view formatted_message_,
                                std::optional<std::string_view> function_name_,
                                const std::optional<std::chrono::system_clock::time_point>& now_) noexcept
        :   level(level_),
            message(message_.data() ? message_ : std::string_view{""}),
            formatted_message(formatted_message_.data() ? formatted_message_ : std::string_view{""}),
            function_name(std::move(function_name_)),
            now(now_) {
    //--------------------------
    if (formatted_message.empty()) {
        formatted_message = message;
    }// end if (formatted_message.empty())
    //--------------------------
    if (function_name.has_value() and (!function_name->data() or function_name->empty())) {
        function_name.reset();
    }// end if (function_name.has_value() and (!function_name->data() or function_name->empty()))
    //--------------------------
}// end LogRecord::LogRecord(...)
//--------------------------------------------------------------
Logger::LogRecord::LogRecord( const LogLevel& level_,
                        std::string_view message_,
                        std::string_view formatted_message_,
                        std::string_view function_name_,
                        const std::optional<std::chrono::system_clock::time_point>& now_) noexcept
    : LogRecord(level_, message_, formatted_message_, std::optional<std::string_view>(function_name_), now_) {
    //--------------------------
}// end LogRecord::LogRecord(...)
//--------------------------------------------------------------
