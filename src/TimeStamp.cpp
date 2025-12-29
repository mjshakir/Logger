//--------------------------------------------------------------
// Main Header
//--------------------------------------------------------------
#include "TimeStamp.hpp"
//--------------------------------------------------------------
#if LOGGER_HAS_STD_FORMAT
    #include <format>
#else
    #include <fmt/format.h>
#endif
//--------------------------------------------------------------
// Public
//--------------------------------------------------------------
bool Logger::TimeStamp::format_timestamp(const std::chrono::system_clock::time_point& now, std::string_view& out) noexcept {
    //--------------------------
    return format_cached(now, out);
    //--------------------------
}// end bool Logger::TimeStamp::format_timestamp(const std::chrono::system_clock::time_point& now, std::string_view& out) noexcept
//--------------------------------------------------------------
bool Logger::TimeStamp::format_timestamp(const std::optional<std::chrono::system_clock::time_point>& now, std::string_view& out) noexcept {
    //--------------------------
    if (!now.has_value()) {
        out = {};
        return false;
    }// end if (!now.has_value())
    //--------------------------
    return format_cached(*now, out);
    //--------------------------
}// end bool Logger::TimeStamp::format_timestamp(const std::optional<std::chrono::system_clock::time_point>& now, std::string_view& out) noexcept
//--------------------------------------------------------------
bool Logger::TimeStamp::format_timestamp_default(const std::optional<std::chrono::system_clock::time_point>& now, std::string_view& out) noexcept {
    //--------------------------
    if (format_timestamp(now, out)) {
        return true;
    }// end if (format_timestamp(now, out))
    //--------------------------
    out = default_value();
    return false;
    //--------------------------
}// end bool Logger::TimeStamp::format_timestamp_default(const std::optional<std::chrono::system_clock::time_point>& now, std::string_view& out) noexcept
//--------------------------------------------------------------
// Protected
//--------------------------------------------------------------
bool Logger::TimeStamp::local_time(std::time_t seconds, std::tm& out) noexcept {
    //--------------------------
#if defined(_WIN32)
    return localtime_s(&out, &seconds) == 0;
#else
    return localtime_r(&seconds, &out) != nullptr;
#endif
    //--------------------------
}// end bool Logger::TimeStamp::local_time(std::time_t seconds, std::tm& out) noexcept
//--------------------------------------------------------------
bool Logger::TimeStamp::format_time(const std::tm& timeinfo, Buffer& buffer) noexcept {
    //--------------------------
    const int64_t year  = static_cast<int64_t>(timeinfo.tm_year + 1900);
    const int64_t month = static_cast<int64_t>(timeinfo.tm_mon + 1);
    const int64_t day   = static_cast<int64_t>(timeinfo.tm_mday);
    const int64_t hour  = static_cast<int64_t>(timeinfo.tm_hour);
    const int64_t min   = static_cast<int64_t>(timeinfo.tm_min);
    const int64_t sec   = static_cast<int64_t>(timeinfo.tm_sec);
    //--------------------------
    try {
#if LOGGER_HAS_STD_FORMAT
        const auto res = std::format_to_n(buffer.data(),
            static_cast<std::ptrdiff_t>(C_FORMATTED_LENGTH),
            "{:04}-{:02}-{:02} {:02}:{:02}:{:02}",
            year, month, day, hour, min, sec);
#else
        const auto res = fmt::format_to_n(buffer.data(),
            C_FORMATTED_LENGTH, "{:04}-{:02}-{:02} {:02}:{:02}:{:02}",
            year, month, day, hour, min, sec);
#endif
        // A mismatch means truncation OR unexpected field widths (e.g. out-of-range tm values).
        if (static_cast<size_t>(res.size) != C_FORMATTED_LENGTH) {
            return false;
        }// end if (static_cast<size_t>(res.size) != C_FORMATTED_LENGTH)
    } catch (...) {
        return false;
    }// end catch (...)
    //--------------------------
    buffer.at(C_FORMATTED_LENGTH) = '\0';
    //--------------------------
    return true;
    //--------------------------
}// end bool Logger::TimeStamp::format_time(const std::tm& timeinfo, Buffer& buffer) noexcept
//--------------------------------------------------------------
bool Logger::TimeStamp::format_cached(const std::chrono::system_clock::time_point& now, std::string_view& out) noexcept {
    //--------------------------
    thread_local TimeCache _cache;
    //--------------------------
    const std::time_t _seconds = std::chrono::system_clock::to_time_t(now);
    if (_cache.has_last and _seconds == _cache.last_seconds) {
        out = std::string_view(_cache.buffer.data(), C_FORMATTED_LENGTH);
        return true;
    }// end if (_cache.has_last and _seconds == _cache.last_seconds)
    //--------------------------
    std::tm timeinfo;
    if (!local_time(_seconds, timeinfo)) {
        out = {};
        return false;
    }// end if (!local_time(_seconds, timeinfo))
    //--------------------------
    if (!format_time(timeinfo, _cache.buffer)) {
        out = {};
        return false;
    }// end if (!format_time(timeinfo, _cache.buffer))
    //--------------------------
    _cache.last_seconds = _seconds;
    _cache.has_last = true;
    out = std::string_view(_cache.buffer.data(), C_FORMATTED_LENGTH);
    return true;
    //--------------------------
}// end bool Logger::TimeStamp::format_cached(const std::chrono::system_clock::time_point& now, std::string_view& out) noexcept
//--------------------------------------------------------------
Logger::TimeStamp::TimeCache::TimeCache(void) : last_seconds(),
                                                buffer(),
                                                has_last(false) {
//--------------------------
}// end Logger::TimeStamp::TimeCache::TimeCache(void)


