#pragma once
//--------------------------------------------------------------
// Standard cpp library
//--------------------------------------------------------------
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
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
//--------------------------------------------------------------
namespace Logger {
    //--------------------------------------------------------------
    // Forward declarations LogRecord
    //--------------------------------------------------------------
    struct LogRecord;
    //--------------------------------------------------------------
    class LOGGER_LOCAL SQLogger {
        //--------------------------------------------------------------
        public:
            //--------------------------------------------------------------
            // A database of its own: opened once here at database_path, closed in the destructor.
            // Made by a LogSession, which owns it; there is no instance() any more.
            explicit SQLogger(std::string_view database_path);
            ~SQLogger(void);
            //--------------------------
            SQLogger(const SQLogger&)               = delete;
            SQLogger& operator=(const SQLogger&)    = delete;
            SQLogger(SQLogger&&)                    = delete;
            SQLogger& operator=(SQLogger&&)         = delete;
            //--------------------------
            // True when the database is open and the table is there: a row can be stored.
            bool opened(void) const noexcept;
            //--------------------------
            // run_id 1 or more is stored in the row's run_id column; 0 or less stores NULL.
            bool log(const LogRecord& record, std::int64_t run_id) const;
            //--------------------------------------------------------------
        protected:
            //--------------------------------------------------------------
            bool initialize(std::string_view database_path);
            //--------------------------
            std::string pragma_name(std::string_view table_name) const;
            //--------------------------
            bool has_column(sqlite3* db, std::string_view table_name, std::string_view column_name) const;
            //--------------------------
            constexpr std::string_view create_table_sql(void) const;
            //--------------------------
            bool insert_stmt(std::string_view time, const LogRecord& record, std::int64_t run_id) const;
            //--------------------------
            struct SQLiteDbDeleter {
                void operator()(sqlite3* db) const;
            };// end struct SQLiteDbDeleter
            //--------------------------
            struct SQLiteStmtDeleter {
                void operator()(sqlite3_stmt* stmt) const;
            };// end struct SQLiteStmtDeleter
            //--------------------------
            std::unique_ptr<sqlite3_stmt, SQLiteStmtDeleter> prepare_insert(void) const;
            //--------------------------
            struct Cleanup {
                //--------------------------
                Cleanup(void) = default;
                Cleanup(sqlite3_stmt* stmt_);
                ~Cleanup(void);
                //--------------------------
                sqlite3_stmt* stmt;
            };// end struct Cleanup
            //--------------------------------------------------------------
        private:
            //--------------------------------------------------------------
            std::unique_ptr<sqlite3, SQLiteDbDeleter> m_db;
            //--------------------------
            const bool m_initialized;
        //--------------------------------------------------------------
    };// end class SQLogger
    //--------------------------------------------------------------
}// end namespace Logger
//--------------------------------------------------------------
