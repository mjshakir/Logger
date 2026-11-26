#pragma once
//--------------------------------------------------------------
// Standard cpp library
//--------------------------------------------------------------
#include <cstdint>
#include <expected>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
//--------------------------------------------------------------
// User Defined library
//--------------------------------------------------------------
#include "LoggerAPI.hpp"
#include "LoggerTypes.hpp"
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
            // Made by a LogStore, which owns it; there is no instance() any more. The library's
            // schema is applied here, in one transaction.
            explicit SQLogger(const std::string& database_path);
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
            // The run, when there is one, is stored in the row's run_id column; none stores NULL.
            bool log(const LogRecord& record, std::optional<std::int64_t> run_id) const;
            //--------------------------
            // The second connection, for the run's rows: an open cursor on it never holds a read
            // snapshot on the messages' connection. NO_SESSION when it did not open.
            std::expected<sqlite3*, LogError> control(void) const noexcept;
            //--------------------------------------------------------------
        protected:
            //--------------------------------------------------------------
            bool initialize(const std::string& database_path);
            //--------------------------
            // Steps every statement of `sql` on the messages' connection, one after the other; false
            // at the first refusal.
            bool applied(std::string_view sql) const noexcept;
            //--------------------------
            std::string pragma_name(std::string_view table_name) const;
            //--------------------------
            bool has_column(sqlite3* db, std::string_view table_name, std::string_view column_name) const;
            //--------------------------
            bool insert_stmt(std::string_view time, const LogRecord& record, std::optional<std::int64_t> run_id) const;
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
            std::unique_ptr<sqlite3, SQLiteDbDeleter> m_db;      // the messages' connection
            std::unique_ptr<sqlite3, SQLiteDbDeleter> m_control; // the run's: row, end, unfinished
            //--------------------------
            const bool m_initialized;
        //--------------------------------------------------------------
    };// end class SQLogger
    //--------------------------------------------------------------
}// end namespace Logger
//--------------------------------------------------------------
