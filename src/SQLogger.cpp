//--------------------------------------------------------------
// Main Header 
//--------------------------------------------------------------
#include "SQLogger.hpp"
//--------------------------------------------------------------
// Standard cpp library
//--------------------------------------------------------------
#include <iostream>
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
void Logger::SQLogger::SQLiteStmtDeleter::operator()(sqlite3_stmt* stmt) const {
    //--------------------------
    if (stmt) {
        sqlite3_finalize(stmt);
    }// end if (stmt)
    //--------------------------
}// end void Logger::SQLogger::SQLiteStmtDeleter::operator()(sqlite3_stmt* stmt) const
//--------------------------------------------------------------
Logger::SQLogger& Logger::SQLogger::instance(void) {
    static SQLogger instance;
    return instance;
}// end Logger::SQLogger& Logger::SQLogger::instance(void)
//--------------------------------------------------------------
Logger::SQLogger::SQLogger(void) :  m_db(nullptr, &sqlite3_close),
                                    m_insertStmt(nullptr),
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
    return insert_stmt(format_time(now), level, message);
    //--------------------------
}// end bool Logger::SQLogger::log(std::string_view message, const std::optional<std::chrono::system_clock::time_point>& now = std::nullopt)
//--------------------------------------------------------------
bool Logger::SQLogger::initialize(void) {
    //--------------------------
    // std::lock_guard<std::mutex> lock(m_mutex);
    //--------------------------
    // Use a temporary raw pointer because sqlite3_open requires a sqlite3**
    sqlite3* db_raw = nullptr;
    //--------------------------
    if (sqlite3_open("logs.db", &db_raw) != SQLITE_OK) {
        //--------------------------
        std::cerr << "Error opening SQLite database: " << sqlite3_errmsg(db_raw) << std::endl;
        //--------------------------
        if (db_raw) {
            sqlite3_close(db_raw);
        }// end if (db_raw)
        //--------------------------
        m_db.reset(nullptr);
        //--------------------------
        return false;
        //--------------------------
    }// end if (sqlite3_open("logs.db", &db_raw) != SQLITE_OK)
    //--------------------------
    m_db.reset(db_raw);
    //--------------------------
    // Optional: Improve performance by tweaking PRAGMA settings.
    //--------------------------
    if (sqlite3_exec(m_db.get(), "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr) != SQLITE_OK) {
        std::cerr << "Error setting journal_mode: " << sqlite3_errmsg(m_db.get()) << std::endl;
    }// end if (sqlite3_exec(m_db.get(), "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr) != SQLITE_OK)
    //--------------------------
    if (sqlite3_exec(m_db.get(), "PRAGMA synchronous=NORMAL;", nullptr, nullptr, nullptr) != SQLITE_OK) {
        std::cerr << "Error setting synchronous: " << sqlite3_errmsg(m_db.get()) << std::endl;
    }// end if (sqlite3_exec(m_db.get(), "PRAGMA synchronous=NORMAL;", nullptr, nullptr, nullptr) != SQLITE_OK)
    //--------------------------
    if (sqlite3_exec(m_db.get(), create_table_sql().data(), nullptr, nullptr, nullptr) != SQLITE_OK) {
        std::cerr << "Error creating logs table: " << sqlite3_errmsg(m_db.get()) << std::endl;
        return false;
    }// end if (sqlite3_exec(m_db.get(), create_table_sql().data(), nullptr, nullptr, nullptr) != SQLITE_OK)
    //--------------------------
    constexpr std::string_view insert_sql = "INSERT INTO logs (timestamp, log_level, message) VALUES (?, ?, ?);";
    sqlite3_stmt* stmt = nullptr;
    //--------------------------
    if (sqlite3_prepare_v2(m_db.get(), insert_sql.data(), -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Error preparing insert statement: " << sqlite3_errmsg(m_db.get()) << std::endl;
        return false;
    }// end if (sqlite3_prepare_v2(m_db.get(), insert_sql, -1, &stmt, nullptr) != SQLITE_OK)
    //--------------------------
    m_insertStmt.reset(stmt);
    //--------------------------
    return true;
    //--------------------------
}// end void Logger::SQLogger::initialize(void)
//--------------------------------------------------------------
constexpr std::string_view Logger::SQLogger::create_table_sql(void) const {
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
constexpr std::string_view Logger::SQLogger::default_time(void) const {
    //--------------------------
    return "0000-00-00 00:00:00";
    //--------------------------
}// end constexpr std::string_view Logger::SQLogger::default_time(void)
//--------------------------------------------------------------
std::string Logger::SQLogger::format_time(const std::optional<std::chrono::system_clock::time_point>& now) const {
    //--------------------------
    if (!now.has_value()) {
#if __cpp_lib_format
    return std::format("{}", default_time());
#else
    return fmt::format(FMT_COMPILE("{}"), default_time());
#endif
    }// end if (!now.has_value())
    //--------------------------
    const std::time_t _localtime = std::chrono::system_clock::to_time_t(now.value());
    std::tm _timeinfo;
    //--------------------------
#if defined(_WIN32)
    localtime_s(&_timeinfo, &_localtime);
#else
    localtime_r(&_localtime, &_timeinfo);
#endif
    //--------------------------
#if __cpp_lib_format
    return std::format("{:%Y-%m-%d %H:%M:%S}", _timeinfo);
#else     
    return fmt::format(FMT_COMPILE("{:%Y-%m-%d %H:%M:%S}"), _timeinfo);
#endif
    //--------------------------
}// end std::string std::string_view Logger::SQLogger::format_time(const std::optional<std::chrono::system_clock::time_point>& now)
//--------------------------------------------------------------
bool Logger::SQLogger::insert_stmt(std::string_view time, std::string_view level, std::string_view message) const {
    //--------------------------
    sqlite3_reset(m_insertStmt.get());
    sqlite3_clear_bindings(m_insertStmt.get());
    //--------------------------
    if (sqlite3_bind_text(m_insertStmt.get(), 1, time.data(), -1, SQLITE_TRANSIENT) != SQLITE_OK) {
        std::cerr << "Error binding timestamp: " << sqlite3_errmsg(m_db.get()) << std::endl;
        return false;
    }// end if (sqlite3_bind_text(m_insertStmt.get(), 1, time.data
    //--------------------------
    // Bind the log level (parameter index 2).
    if (sqlite3_bind_text(m_insertStmt.get(), 2, level.data(), static_cast<int>(level.size()), SQLITE_TRANSIENT) != SQLITE_OK) {
        std::cerr << "Error binding log level: " << sqlite3_errmsg(m_db.get()) << std::endl;
        return false;
    }// end if (sqlite3_bind_text(m_insertStmt.get(), 2, level.data(), static_cast<int>(level.size()), SQLITE_TRANSIENT) != SQLITE_OK)
    //--------------------------
    if (sqlite3_bind_text(m_insertStmt.get(), 3, message.data(), static_cast<int>(message.size()), SQLITE_TRANSIENT) != SQLITE_OK) {
        std::cerr << "Error binding message: " << sqlite3_errmsg(m_db.get()) << std::endl;
        return false;
    }// end if (sqlite3_bind_text(m_insertStmt.get(), 3, message.data(), static_cast<int>(message.size()), SQLITE_TRANSIENT) != SQLITE_OK)
    //--------------------------
    int rc = sqlite3_step(m_insertStmt.get());
    if (rc != SQLITE_DONE) {
        std::cerr << "Error executing insert statement: " << sqlite3_errmsg(m_db.get()) << std::endl;
        return false;
    }// end if (rc != SQLITE_DONE)
    //--------------------------
    return true;
    //-----------------------------
}// end bool Logger::SQLogger::insert_stmt(std::string_view time, std::string_view level, std::string_view message) const
//--------------------------------------------------------------
