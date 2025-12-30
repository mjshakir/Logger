#pragma once
//--------------------------------------------------------------
// Standard cpp library
//--------------------------------------------------------------
#include <array>
#include <chrono>
#include <cstddef>
#include <ctime>
#include <optional>
#include <string_view>
//--------------------------------------------------------------
// User Defined library
//--------------------------------------------------------------
#include "LoggerAPI.hpp"
//--------------------------------------------------------------
namespace Logger {
    //--------------------------------------------------------------
    class LOGGER_LOCAL TimeStamp {
        //--------------------------------------------------------------
        private:
            //--------------------------------------------------------------
            // "%Y-%m-%d %H:%M:%S" -> "YYYY-MM-DD HH:MM:SS" (local time)
            //--------------------------------------------------------------
            static constexpr std::string_view C_FORMAT  = "%Y-%m-%d %H:%M:%S";
            static constexpr std::string_view C_DEFAULT = "0000-00-00 00:00:00";
            static constexpr size_t C_FORMATTED_LENGTH  = 19UL;                     // "YYYY-MM-DD HH:MM:SS"
            static constexpr size_t C_BUFFER_SIZE       = C_FORMATTED_LENGTH + 1UL; // + '\0'
            //--------------------------
            static_assert(C_DEFAULT.size() == C_FORMATTED_LENGTH, "TimeStamp::C_DEFAULT must match TimeStamp::C_FORMATTED_LENGTH.");
            static_assert(C_FORMAT == std::string_view{"%Y-%m-%d %H:%M:%S"}, "TimeStamp::C_FORMAT must be \"%Y-%m-%d %H:%M:%S\".");
            //--------------------------
            using Buffer = std::array<char, C_BUFFER_SIZE>;
            //--------------------------------------------------------------
        public:
            //--------------------------------------------------------------
            TimeStamp(void)     = delete;
            ~TimeStamp(void)    = delete;
            //--------------------------------------------------------------
            // Fast path (thread-local cache, 1-second resolution).
            // `out` points to internal storage and is valid until the next call on the same thread.
            //--------------------------------------------------------------
            static bool format_timestamp(const std::chrono::system_clock::time_point& now, std::string_view& out) noexcept;
            //--------------------------
            static bool format_timestamp(const std::optional<std::chrono::system_clock::time_point>& now, std::string_view& out) noexcept;
            //--------------------------
            static bool format_timestamp_default(const std::optional<std::chrono::system_clock::time_point>& now, std::string_view& out) noexcept;
            //--------------------------
            static consteval std::string_view default_value(void) noexcept {
                return C_DEFAULT;
            }// end static constexpr std::string_view default_value(void) noexcept
            //--------------------------------------------------------------
        protected:
            //--------------------------------------------------------------
            static bool local_time(std::time_t seconds, std::tm& out) noexcept;
            //--------------------------
            static bool format_time(const std::tm& timeinfo, Buffer& buffer) noexcept;
            //--------------------------
            static bool format_cached(const std::chrono::system_clock::time_point& now, std::string_view& out) noexcept;
            //--------------------------
            struct TimeCache {
                //--------------------------
                TimeCache(void);
                ~TimeCache(void) = default;
                //--------------------------
                std::time_t last_seconds;
                Buffer buffer;
                bool has_last;
                //--------------------------
            };// end struct TimeCache
        //--------------------------------------------------------------
    };// end class TimeStamp
    //--------------------------------------------------------------
} // namespace Logger
//--------------------------------------------------------------
