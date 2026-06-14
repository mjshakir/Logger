//--------------------------------------------------------------
// Main Header 
//--------------------------------------------------------------
#include "LogRecord.hpp"
//--------------------------------------------------------------
Logger::LogRecord::LogRecord(void) :    level(LogLevel::INFO),
                                        message(""),
                                        formatted_message(""),
                                        now(std::nullopt),
                                        file(std::nullopt),
                                        function_name(std::nullopt),
                                        line(std::nullopt) {
    //--------------------------
}// end Logger::LogRecord::LogRecord(void)
//--------------------------------------------------------------
Logger::LogRecord::LogRecord(   const LogLevel& level_,
                                std::string_view message_,
                                std::string_view formatted_message_,
                                const std::optional<std::chrono::system_clock::time_point>& now_,
                                std::optional<std::string_view> file_,
                                std::optional<std::string_view> function_name_,
                                std::optional<std::size_t> line_) noexcept
        :   level(level_),
            message(message_.data() ? message_ : std::string_view{""}),
            formatted_message(formatted_message_.data() ? formatted_message_ : std::string_view{""}),
            now(now_),
            file(std::move(file_)),
            function_name(std::move(function_name_)),
            line(std::move(line_)) {
    //--------------------------
    if (formatted_message.empty()) {
        formatted_message = message;
    }// end if (formatted_message.empty())
    //--------------------------
    if (function_name.has_value() and (!function_name->data() or function_name->empty())) {
        function_name.reset();
    }// end if (function_name.has_value() and (!function_name->data() or function_name->empty()))
    //--------------------------
    if (file.has_value() and (!file->data() or file->empty())) {
        file.reset();
    }// end if (file.has_value() and (!file->data() or file->empty()))
    //--------------------------
}// end LogRecord::LogRecord(...)
//--------------------------------------------------------------
