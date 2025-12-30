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
// User Defined library
//--------------------------------------------------------------
#include "LogRecord.hpp"
#include "TimeStamp.hpp"
//--------------------------------------------------------------
// Statement Deleter
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
bool Logger::SQLogger::log(const LogRecord& record) const {
    //--------------------------
    if (!m_initialized) {
        std::cerr << "SQLogger was not initialized properly." << std::endl;
        return false;
    }// end if (!m_initialized)
    //--------------------------
    std::string_view time;
    TimeStamp::format_timestamp_default(record.now, time);
    //--------------------------
    return insert_stmt(time, record);
    //--------------------------
}// end bool Logger::SQLogger::log(const LogRecord& record) const
//--------------------------------------------------------------
bool Logger::SQLogger::initialize(void) {
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
    if (!has_column(m_db.get(), "logs", "function_name")) {
        if (sqlite3_exec(m_db.get(), "ALTER TABLE logs ADD COLUMN function_name TEXT;", nullptr, nullptr, nullptr) != SQLITE_OK) {
            std::cerr << "Error migrating logs table: " << sqlite3_errmsg(m_db.get()) << std::endl;
            return false;
        }// end if (sqlite3_exec(...))
    }// end if (!has_column(m_db.get(), "logs", "function_name"))
    //--------------------------
    constexpr std::string_view insert_sql = "INSERT INTO logs (timestamp, log_level, function_name, message) VALUES (?, ?, ?, ?);";
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
std::string Logger::SQLogger::pragma_name(std::string_view table_name) const {
    //--------------------------
    std::string _pragma;
    _pragma.reserve(32 + table_name.size());
    _pragma.append("PRAGMA table_info(");
    _pragma.append(table_name);
    _pragma.append(");");
    //--------------------------
    return _pragma;
    //--------------------------
}// end constexpr std::string_view Logger::SQLogger::pragma_name(std::string_view table_name) const
//--------------------------------------------------------------
bool Logger::SQLogger::has_column(sqlite3* db, std::string_view table_name, std::string_view column_name) const {
    //--------------------------
    if (!db) {
        return false;
    }// end if (!db)
    //--------------------------
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, pragma_name(table_name).c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }// end if (sqlite3_prepare_v2(db, _pragma.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
    //--------------------------
    bool found = false;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char* name = sqlite3_column_text(stmt, 1);
        if (name and column_name == reinterpret_cast<const char*>(name)) {
            found = true;
            break;
        }// end if (name and column_name == reinterpret_cast<const char*>(name))
    }// end while (sqlite3_step(stmt) == SQLITE_ROW)
    //--------------------------
    sqlite3_finalize(stmt);
    return found;
    //--------------------------
}// end bool has_column(sqlite3* db, std::string_view table_name, std::string_view column_name)
//--------------------------------------------------------------
constexpr std::string_view Logger::SQLogger::create_table_sql(void) const {
    //--------------------------
    return R"(
        CREATE TABLE IF NOT EXISTS logs (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp TEXT NOT NULL,
            log_level  TEXT NOT NULL,
            function_name TEXT,
            message TEXT NOT NULL
        );
    )";
    //--------------------------
}// end constexpr std::string_view Logger::SQLogger::create_table_sql(void)
//--------------------------------------------------------------
bool Logger::SQLogger::insert_stmt(std::string_view time, const LogRecord& record) const {
    //--------------------------
    sqlite3_stmt* stmt = m_insertStmt.get();
    if (!stmt) {
        std::cerr << "Insert statement is not initialized." << std::endl;
        return false;
    }// end if (!stmt)
    //--------------------------
    // When using SQLITE_STATIC, make sure we don't keep references to caller memory.
    Cleanup _cleanup(stmt);
    //--------------------------
    const std::string_view level = level_name(record.level);
    //--------------------------
    if (sqlite3_bind_text(stmt, 1, time.data(), static_cast<int>(time.size()), SQLITE_STATIC) != SQLITE_OK) {
        std::cerr << "Error binding timestamp: " << sqlite3_errmsg(m_db.get()) << std::endl;
        return false;
    }// end if (sqlite3_bind_text(m_insertStmt.get(), 1, time.data
    //--------------------------
    // Bind the log level (parameter index 2).
    if (sqlite3_bind_text(stmt, 2, level.data(), static_cast<int>(level.size()), SQLITE_STATIC) != SQLITE_OK) {
        std::cerr << "Error binding log level: " << sqlite3_errmsg(m_db.get()) << std::endl;
        return false;
    }// end if (sqlite3_bind_text(m_insertStmt.get(), 2, level.data(), static_cast<int>(level.size()), SQLITE_TRANSIENT) != SQLITE_OK)
    //--------------------------
    if (record.function_name.has_value() and !record.function_name->empty()) {
        if (sqlite3_bind_text(stmt, 3, record.function_name->data(), static_cast<int>(record.function_name->size()), SQLITE_STATIC) != SQLITE_OK) {
            std::cerr << "Error binding function name: " << sqlite3_errmsg(m_db.get()) << std::endl;
            return false;
        }// end if (sqlite3_bind_text(...))
    } else {
        if (sqlite3_bind_null(stmt, 3) != SQLITE_OK) {
            std::cerr << "Error binding function name: " << sqlite3_errmsg(m_db.get()) << std::endl;
            return false;
        }// end  if (sqlite3_bind_null(m_insertStmt.get(), 3) != SQLITE_OK)
    }// end if (record.function_name.has_value())
    //--------------------------
    const char* message_ptr = record.message.data();
    const int message_len = message_ptr ? static_cast<int>(record.message.size()) : 0;
    if (sqlite3_bind_text(stmt, 4, message_ptr ? message_ptr : "", message_len, SQLITE_STATIC) != SQLITE_OK) {
        std::cerr << "Error binding message: " << sqlite3_errmsg(m_db.get()) << std::endl;
        return false;
    }// end if (sqlite3_bind_text(stmt, 4, message_ptr ? message_ptr : "", message_len, SQLITE_STATIC) != SQLITE_OK)
    //--------------------------
    int rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        std::cerr << "Error executing insert statement: " << sqlite3_errmsg(m_db.get()) << std::endl;
        return false;
    }// end if (rc != SQLITE_DONE)
    //--------------------------
    return true;
    //-----------------------------
}// end bool Logger::SQLogger::insert_stmt(std::string_view time, const LogRecord& record) const
//--------------------------------------------------------------
// Cleanup struct
//--------------------------------------------------------------
Logger::SQLogger::Cleanup::Cleanup(sqlite3_stmt* stmt_) : stmt(stmt_){
    //-----------------------------
}// end Logger::SQLogger::Cleanup::Cleanup(sqlite3_stmt* stmt_)
//--------------------------------------------------------------
Logger::SQLogger::Cleanup::~Cleanup(void) {
    if (stmt) {
        sqlite3_clear_bindings(stmt);
        sqlite3_reset(stmt);
    }// end if (stmt)
}// end Logger::SQLogger::Cleanup::~Cleanup(void)
//--------------------------------------------------------------