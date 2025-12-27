#pragma once
//--------------------------------------------------------------
// Standard cpp library
//--------------------------------------------------------------
#include <optional>
#include <memory>
#include <string>
#include <string_view>
#include <chrono>
//--------------------------------------------------------------
// User Defined library
//--------------------------------------------------------------
#include "LoggerAPI.hpp"
//--------------------------------------------------------------
// Forward declarations for SQLite
//--------------------------------------------------------------
struct sqlite3;
//--------------------------
struct sqlite3_stmt;
//--------------------------
extern "C" {
    int sqlite3_close(sqlite3*);
}// end extern "C"
//--------------------------------------------------------------
namespace Logger {
    //--------------------------------------------------------------
    class LOGGER_LOCAL SQLogger {
        //--------------------------------------------------------------
        public:
            //--------------------------------------------------------------
            static SQLogger& instance(void);
            //--------------------------
            bool log(std::string_view level, std::string_view message, const std::optional<std::chrono::system_clock::time_point>& now = std::nullopt) const;
            //--------------------------------------------------------------
        protected:
            //--------------------------------------------------------------
            bool initialize(void);
            //--------------------------
            constexpr std::string_view create_table_sql(void) const;
            //--------------------------
            constexpr std::string_view default_time(void) const;
            //--------------------------
            std::string format_time(const std::optional<std::chrono::system_clock::time_point>& now) const;
            //--------------------------
            bool insert_stmt(std::string_view time, std::string_view level, std::string_view message) const;
            //--------------------------
            struct SQLiteStmtDeleter {
                void operator()(sqlite3_stmt* stmt) const;
            };// end struct SQLiteStmtDeleter
            //--------------------------------------------------------------
        private:
            //--------------------------------------------------------------
            SQLogger(void);
            ~SQLogger(void);
            //--------------------------
            SQLogger(const SQLogger&)               = delete;
            SQLogger& operator=(const SQLogger&)    = delete;
            SQLogger(SQLogger&&)                    = delete;
            SQLogger& operator=(SQLogger&&)         = delete;
            //--------------------------
            std::unique_ptr<sqlite3, decltype(&sqlite3_close)> m_db;
            std::unique_ptr<sqlite3_stmt, SQLiteStmtDeleter> m_insertStmt;
            //--------------------------
            const bool m_initialized;
        //--------------------------------------------------------------
    };// end class SQLogger
    //--------------------------------------------------------------
}// end namespace Logger
//--------------------------------------------------------------
