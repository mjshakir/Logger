#pragma once
//--------------------------------------------------------------
// Standard cpp library
//--------------------------------------------------------------
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
//--------------------------
extern "C" {
    int sqlite3_close_v2(sqlite3*);
}// end extern "C"
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
            static SQLogger& instance(void);
            //--------------------------
            bool log(const LogRecord& record) const;
            //--------------------------------------------------------------
        protected:
            //--------------------------------------------------------------
            bool initialize(void);
            //--------------------------
            std::string pragma_name(std::string_view table_name) const;
            //--------------------------
            bool has_column(sqlite3* db, std::string_view table_name, std::string_view column_name) const;
            //--------------------------
            constexpr std::string_view create_table_sql(void) const;
            //--------------------------
            bool insert_stmt(std::string_view time, const LogRecord& record) const;
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
            SQLogger(void);
            ~SQLogger(void);
            //--------------------------
            SQLogger(const SQLogger&)               = delete;
            SQLogger& operator=(const SQLogger&)    = delete;
            SQLogger(SQLogger&&)                    = delete;
            SQLogger& operator=(SQLogger&&)         = delete;
            //--------------------------
            std::unique_ptr<sqlite3, decltype(&sqlite3_close_v2)> m_db;
            //--------------------------
            const bool m_initialized;
        //--------------------------------------------------------------
    };// end class SQLogger
    //--------------------------------------------------------------
}// end namespace Logger
//--------------------------------------------------------------
