//--------------------------------------------------------------
// Main Header 
//--------------------------------------------------------------
#include "SQLogger.hpp"
//--------------------------------------------------------------
// Standard cpp library
//--------------------------------------------------------------
#include <iostream>
#include <sstream>
#include <chrono>
#include <string_view>
//--------------------------------------------------------------
// SQLite library
//--------------------------------------------------------------
#include <sqlite3.h>
//--------------------------------------------------------------
// Format library
//--------------------------------------------------------------
#if __cpp_lib_format
    #include <format>
#else
    #include <fmt/core.h>
    #include <fmt/format.h>
    #include <fmt/compile.h>
    #include <fmt/chrono.h>
#endif
//--------------------------------------------------------------
Logger::SQLogger& Logger::SQLogger::instance(void) {
    static SQLogger instance;
    return instance;
}// end Logger::SQLogger& Logger::SQLogger::instance(void)
//--------------------------------------------------------------
Logger::SQLogger::SQLogger(void) :  m_db(nullptr, &sqlite3_close),
                                    m_initialized(initialize()) {
    //--------------------------
}// end Logger::SQLogger::SQLogger(void)
//--------------------------------------------------------------
Logger::SQLogger::~SQLogger(void) {
    //--------------------------
}// end Logger::SQLogger::~SQLogger(void)
//--------------------------------------------------------------
bool Logger::SQLogger::log(std::string_view level, std::string_view message, const std::optional<std::chrono::system_clock::time_point>& now) const {
    //--------------------------
    if (!m_initialized) {
        std::cerr << "SQLogger was not initialized properly." << std::endl;
        return false;
    }// end if (!m_initialized)
    //--------------------------
    return log_message(level, message, now);
    //--------------------------
}// end bool Logger::SQLogger::log(std::string_view message, const std::optional<std::chrono::system_clock::time_point>& now = std::nullopt)
//--------------------------------------------------------------
bool Logger::SQLogger::initialize(void) {
    //--------------------------
    // std::lock_guard<std::mutex> lock(m_mutex);
    //--------------------------
    // Use a temporary raw pointer because sqlite3_open requires a sqlite3**
    sqlite3* db_raw = nullptr;
    if (sqlite3_open("logs.db", &db_raw) != SQLITE_OK) {
        //--------------------------
        std::cerr << "Error opening SQLite database: " << sqlite3_errmsg(db_raw) << std::endl;
        //--------------------------
        if (db_raw) {
            // Even if open fails, sometimes sqlite3_open sets db_raw
            sqlite3_close(db_raw);
        }// end if (db_raw)
        //--------------------------
        m_db.reset(nullptr);
        //--------------------------
        return false;
        //--------------------------
    }// end if (sqlite3_open("logs.db", &db_raw) != SQLITE_OK)
    //--------------------------
    // Store the raw pointer in the unique_ptr
    m_db.reset(db_raw);
    //--------------------------
    if (sqlite3_exec(m_db.get(), create_table_sql().data(), nullptr, nullptr, nullptr) != SQLITE_OK) {
        std::cerr << "Error creating logs table: " << sqlite3_errmsg(m_db.get()) << std::endl;
        return false;
    }// end if (sqlite3_exec(m_db.get(), create_table_sql.c_str(), nullptr, nullptr, nullptr) != SQLITE_OK)
    //--------------------------
    return true;
    //--------------------------
}// end void Logger::SQLogger::initialize(void)
//--------------------------------------------------------------
bool Logger::SQLogger::log_message(std::string_view level, std::string_view message, const std::optional<std::chrono::system_clock::time_point>& now) const {
    //--------------------------
    constexpr std::string_view _zero_time = "0000-00-00 00:00:00";
    //--------------------------
    // std::lock_guard<std::mutex> lock(m_mutex);
    //--------------------------
#if __cpp_lib_format
    // -----------------------------
    std::string sql_query;
    sql_query.reserve(4096UL);
    //--------------------------
    if (now.has_value()) {
        //--------------------------
        const std::time_t _localtime = std::chrono::system_clock::to_time_t(now.value());
        std::tm _timeinfo;
#if defined(_WIN32)
        localtime_s(&_timeinfo, &_localtime);
#else
        localtime_r(&_localtime, &_timeinfo);
#endif  
        //--------------------------
        // Build SQL query with std::format
        sql_query = std::format("INSERT INTO logs (timestamp, log_level, message) VALUES ('{}', '{}', '{}');",
                                 _timeinfo, level, message);
        //--------------------------
    } else {
        // If no time passed, use a placeholder
        sql_query = std::format("INSERT INTO logs (timestamp, log_level, message) VALUES ('{}', '{}', '{}');",
                                    _zero_time, level, message);
        //--------------------------
    }// end if (now)
#else
    //-----------------------------
    fmt::memory_buffer buffer;
    //-----------------------------
    if (now.has_value()) {
        //-----------------------------
        std::time_t _localtime = std::chrono::system_clock::to_time_t(*now);
        std::tm _timeinfo;
        //-----------------------------
#if defined(_WIN32)
        localtime_s(&_timeinfo, &_localtime);
#else
        localtime_r(&_localtime, &_timeinfo);
#endif
        //-----------------------------
        fmt::format_to( std::back_inserter(buffer),
                        FMT_COMPILE("INSERT INTO logs (timestamp, log_level, message) VALUES ('{:%Y-%m-%d %H:%M:%S}', '{}', '{}');"),
                                    _timeinfo, level, message);
        //-----------------------------

    } else {
        fmt::format_to(std::back_inserter(buffer),
            FMT_COMPILE("INSERT INTO logs (timestamp, log_level, message) VALUES ('{}', '{}', '{}');"),
                            _zero_time, level, message);
    }// end if (now)
    //-----------------------------
    // Convert memory_buffer -> std::string
    const std::string sql_query(buffer.data(), buffer.size());
    //-----------------------------
#endif
    //-----------------------------
    // Execute the SQL
    if (sqlite3_exec(m_db.get(), sql_query.c_str(), nullptr, nullptr, nullptr) != SQLITE_OK) {
        std::cerr << "Error inserting log: " << sqlite3_errmsg(m_db.get()) << std::endl;
        return false;
    }// end if (sqlite3_exec(m_db.get(), sql_query.c_str(), nullptr, nullptr, nullptr) != SQLITE_OK)
    //-----------------------------
    return true;
    //-----------------------------
}// end bool Logger::SQLogger::log_message(std::string_view message, const std::optional<std::chrono::system_clock::time_point>& now)
//--------------------------------------------------------------
constexpr std::string_view Logger::SQLogger::create_table_sql(void) const {
    //--------------------------
    // return R"(
    //     CREATE TABLE IF NOT EXISTS logs (
    //         id INTEGER PRIMARY KEY AUTOINCREMENT,
    //         timestamp TEXT NOT NULL,
    //         log_level  TEXT NOT NULL,
    //         function_name TEXT NOT NULL,
    //         message TEXT NOT NULL
    //     );
    // )";
    //--------------------------
    return R"(
        CREATE TABLE IF NOT EXISTS logs (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp TEXT NOT NULL,
            log_level  TEXT NOT NULL,
            message TEXT NOT NULL
        );
    )";
    //--------------------------
}// end constexpr std::string_view Logger::SQLogger::create_table_sql(void)
//--------------------------------------------------------------