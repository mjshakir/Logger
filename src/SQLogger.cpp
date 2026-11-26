//--------------------------------------------------------------
// Main Header
//--------------------------------------------------------------
#include "SQLogger.hpp"
//--------------------------------------------------------------
// Standard cpp library
//--------------------------------------------------------------
#include <array>
#include <iostream>
#include <utility>
//--------------------------------------------------------------
// SQLite library
//--------------------------------------------------------------
#include <sqlite3.h>
//--------------------------------------------------------------
// User Defined library
//--------------------------------------------------------------
#include "LogRecord.hpp"
#include "LoggerBuild.hpp"
#include "TimeStamp.hpp"
//--------------------------------------------------------------
// Public
//--------------------------------------------------------------
Logger::SQLogger::SQLogger(const std::string& database_path) :  m_db(nullptr),
                                    m_control(nullptr),
                                    m_initialized(initialize(database_path)) {
    //--------------------------
}// end Logger::SQLogger::SQLogger(const std::string& database_path)
//--------------------------------------------------------------
Logger::SQLogger::~SQLogger(void) = default;
//--------------------------------------------------------------
bool Logger::SQLogger::opened(void) const noexcept {
    //--------------------------
    return m_initialized;
    //--------------------------
}// end bool Logger::SQLogger::opened(void) const noexcept
//--------------------------------------------------------------
bool Logger::SQLogger::log(const LogRecord& record, std::optional<std::int64_t> run_id) const {
    //--------------------------
    if (!m_initialized) {
        std::cerr << "SQLogger was not initialized properly." << std::endl;
        return false;
    }// end if (!m_initialized)
    //--------------------------
    std::string_view time;
    TimeStamp::format_timestamp_default(record.now, time);
    //--------------------------
    return insert_stmt(time, record, run_id);
    //--------------------------
}// end bool Logger::SQLogger::log(const LogRecord& record, std::optional<std::int64_t> run_id) const
//--------------------------------------------------------------
std::expected<sqlite3*, Logger::LogError> Logger::SQLogger::control(void) const noexcept {
    //--------------------------
    if (!m_initialized or !m_control) {
        return std::unexpected(LogError::NO_SESSION);
    }// end if (!m_initialized or !m_control)
    //--------------------------
    return m_control.get();
    //--------------------------
}// end std::expected<sqlite3*, LogError> Logger::SQLogger::control(void) const noexcept
//--------------------------------------------------------------
// Protected
//--------------------------------------------------------------
bool Logger::SQLogger::initialize(const std::string& database_path) {
    //--------------------------
    if (!sqlite3_threadsafe()) {
        std::cerr << "Error opening SQLite database: the SQLite library has no thread support" << std::endl;
        m_db.reset(nullptr);
        return false;
    }// end if (!sqlite3_threadsafe())
    //--------------------------
    sqlite3* db_raw = nullptr;
    //--------------------------
    if (sqlite3_open_v2(database_path.c_str(), &db_raw, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, nullptr) != SQLITE_OK) {
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
    }// end if (sqlite3_open_v2(database_path.c_str(), &db_raw, ...) != SQLITE_OK)
    //--------------------------
    m_db.reset(db_raw);
    //--------------------------
    // Set first: every statement below queues behind another program that makes the same file.
    sqlite3_busy_timeout(m_db.get(), C_BUSY_WAIT_MILLISECONDS);
    //--------------------------
    // Optional: Improve performance by tweaking PRAGMA settings.
    //--------------------------
    // SQLite answers BUSY at once, with no wait, when two programs switch a new file to WAL
    // together; the one that loses prints this line and finds the file already switched.
    if (sqlite3_exec(m_db.get(), "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr) != SQLITE_OK) {
        std::cerr << "Error setting journal_mode: " << sqlite3_errmsg(m_db.get()) << std::endl;
    }// end if (sqlite3_exec(m_db.get(), "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr) != SQLITE_OK)
    //--------------------------
    if (sqlite3_exec(m_db.get(), "PRAGMA synchronous=NORMAL;", nullptr, nullptr, nullptr) != SQLITE_OK) {
        std::cerr << "Error setting synchronous: " << sqlite3_errmsg(m_db.get()) << std::endl;
    }// end if (sqlite3_exec(m_db.get(), "PRAGMA synchronous=NORMAL;", nullptr, nullptr, nullptr) != SQLITE_OK)
    //--------------------------
    // WAL with synchronous=NORMAL: a commit does not sync, so a row costs a write and no fdatasync.
    // The file stays consistent after a crash of the program; a power cut may lose the last rows,
    // never the database. The checkpoint is the part that syncs (about 125 ms on an SD card): it
    // runs every 8192 pages here instead of every 1000, so a run of a few rows never reaches it.
    if (sqlite3_exec(m_db.get(), "PRAGMA wal_autocheckpoint=8192;", nullptr, nullptr, nullptr) != SQLITE_OK) {
        std::cerr << "Error setting wal_autocheckpoint: " << sqlite3_errmsg(m_db.get()) << std::endl;
    }// end if (sqlite3_exec(m_db.get(), "PRAGMA wal_autocheckpoint=8192;", nullptr, nullptr, nullptr) != SQLITE_OK)
    //--------------------------
    // The whole setup is one transaction: two programs that start together queue on its write lock
    // under the busy wait, and the one that comes second finds the tables made.
    if (sqlite3_exec(m_db.get(), "BEGIN IMMEDIATE;", nullptr, nullptr, nullptr) != SQLITE_OK) {
        std::cerr << "Error starting the setup: " << sqlite3_errmsg(m_db.get()) << std::endl;
        return false;
    }// end if (the setup transaction was refused)
    //--------------------------
    // A table made by an older library lacks the later columns: they are added, each once, before
    // the schema, whose index names one of them.
    // (column, statement that adds it)
    const std::array<std::pair<std::string_view, std::string_view>, 4> _migrations{{
        {"function_name", "ALTER TABLE logs ADD COLUMN function_name TEXT;"},
        {"line_number", "ALTER TABLE logs ADD COLUMN line_number INTEGER;"},
        {"file_name", "ALTER TABLE logs ADD COLUMN file_name TEXT;"},
        {"run_id", "ALTER TABLE logs ADD COLUMN run_id INTEGER;"}}};
    const bool _older = has_column(m_db.get(), "logs", "id");
    bool       _setup = true;
    for (const std::pair<std::string_view, std::string_view>& _migration : _migrations) {
        if (_older and !has_column(m_db.get(), "logs", _migration.first) and !applied(_migration.second)) {
            std::cerr << "Error migrating logs table: " << sqlite3_errmsg(m_db.get()) << std::endl;
            _setup = false;
            break;
        }// end if (the column is missing and could not be added)
    }// end for (_migration)
    //--------------------------
    if (_setup and !applied(C_SCHEMA)) {
        std::cerr << "Error applying the schema: " << sqlite3_errmsg(m_db.get()) << std::endl;
        _setup = false;
    }// end if (the schema was refused)
    //--------------------------
    const int _end = sqlite3_exec(m_db.get(), _setup ? "COMMIT;" : "ROLLBACK;", nullptr, nullptr, nullptr);
    if (!_setup) {
        return false;
    }// end if (the setup was refused: nothing of it stays)
    //--------------------------
    if (_end != SQLITE_OK) {
        std::cerr << "Error committing the setup: " << sqlite3_errmsg(m_db.get()) << std::endl;
        (void)sqlite3_exec(m_db.get(), "ROLLBACK;", nullptr, nullptr, nullptr);
        return false;
    }// end if (the commit was refused)
    //--------------------------
    // The run's own connection: the run's end must not queue behind a message that is stuck in the
    // store, so it has a connection that no message uses.
    sqlite3* control_raw = nullptr;
    if (sqlite3_open_v2(database_path.c_str(), &control_raw, SQLITE_OPEN_READWRITE | SQLITE_OPEN_FULLMUTEX, nullptr) == SQLITE_OK) {
        m_control.reset(control_raw);
        sqlite3_busy_timeout(m_control.get(), C_BUSY_WAIT_MILLISECONDS);
    } else {
        // The messages still work; the run's bookkeeping answers NO_SESSION.
        std::cerr << "Error opening the run's SQLite connection: " << sqlite3_errmsg(control_raw) << std::endl;
        if (control_raw) {
            sqlite3_close(control_raw);
        }// end if (control_raw)
    }// end if (the run's connection opened)
    //--------------------------
    return true;
    //--------------------------
}// end bool Logger::SQLogger::initialize(const std::string& database_path)
//--------------------------------------------------------------
bool Logger::SQLogger::applied(std::string_view sql) const noexcept {
    //--------------------------
    const char*       p_next = sql.data();
    const char* const p_end  = sql.data() + sql.size();
    //--------------------------
    // Bounded by the text: every pass moves p_next on, or stops.
    while (p_next < p_end) {
        sqlite3_stmt* _raw  = nullptr;
        const char*   p_tail = p_next;
        const int     _prepared = sqlite3_prepare_v2(m_db.get(), p_next, static_cast<int>(p_end - p_next), &_raw, &p_tail);
        const std::unique_ptr<sqlite3_stmt, SQLiteStmtDeleter> _statement(_raw);
        if (_prepared != SQLITE_OK or p_tail <= p_next) {
            return false;
        }// end if (the statement was refused, or the text did not move on)
        //--------------------------
        // Text with no statement in it (a comment, a blank) gives no statement to step.
        if (_statement and sqlite3_step(_statement.get()) != SQLITE_DONE) {
            return false;
        }// end if (the statement was refused)
        p_next = p_tail;
    }// end while (text is left)
    //--------------------------
    return true;
    //--------------------------
}// end bool Logger::SQLogger::applied(std::string_view sql) const noexcept
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
bool Logger::SQLogger::insert_stmt(std::string_view time, const LogRecord& record, std::optional<std::int64_t> run_id) const {
    //--------------------------
    // The statement lives for this one message: it is finalized before the function returns, so a
    // closed session leaves nothing behind in any thread.
    const std::unique_ptr<sqlite3_stmt, SQLiteStmtDeleter> owned = prepare_insert();
    sqlite3_stmt* stmt = owned.get();
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
    if (record.line.has_value()) {
        if (sqlite3_bind_int64(stmt, 5, static_cast<sqlite3_int64>(record.line.value())) != SQLITE_OK) {
            std::cerr << "Error binding line number: " << sqlite3_errmsg(m_db.get()) << std::endl;
            return false;
        }// end if (sqlite3_bind_int64(stmt, 5, static_cast<sqlite3_int64>(record.line.value())) != SQLITE_OK)
    } else {
        if (sqlite3_bind_null(stmt, 5) != SQLITE_OK) {
            std::cerr << "Error binding line number: " << sqlite3_errmsg(m_db.get()) << std::endl;
            return false;
        }// end if (sqlite3_bind_null(stmt, 5) != SQLITE_OK)
    }// end if (record.line.has_value())
    //--------------------------
    if (record.file.has_value() and !record.file->empty()) {
        if (sqlite3_bind_text(stmt, 6, record.file->data(), static_cast<int>(record.file->size()), SQLITE_STATIC) != SQLITE_OK) {
            std::cerr << "Error binding file name: " << sqlite3_errmsg(m_db.get()) << std::endl;
            return false;
        }// end if (sqlite3_bind_text(...))
    } else {
        if (sqlite3_bind_null(stmt, 6) != SQLITE_OK) {
            std::cerr << "Error binding file name: " << sqlite3_errmsg(m_db.get()) << std::endl;
            return false;
        }// end if (sqlite3_bind_null(stmt, 6) != SQLITE_OK)
    }// end if (record.file.has_value() and !record.file->empty())
    //--------------------------
    if (run_id.has_value()) {
        if (sqlite3_bind_int64(stmt, 7, static_cast<sqlite3_int64>(*run_id)) != SQLITE_OK) {
            std::cerr << "Error binding run id: " << sqlite3_errmsg(m_db.get()) << std::endl;
            return false;
        }// end if (sqlite3_bind_int64(stmt, 7, static_cast<sqlite3_int64>(run_id)) != SQLITE_OK)
    } else {
        if (sqlite3_bind_null(stmt, 7) != SQLITE_OK) {
            std::cerr << "Error binding run id: " << sqlite3_errmsg(m_db.get()) << std::endl;
            return false;
        }// end if (sqlite3_bind_null(stmt, 7) != SQLITE_OK)
    }// end if (run_id.has_value())
    //--------------------------
    int rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        std::cerr << "Error executing insert statement: " << sqlite3_errmsg(m_db.get()) << std::endl;
        return false;
    }// end if (rc != SQLITE_DONE)
    //--------------------------
    return true;
    //-----------------------------
}// end bool Logger::SQLogger::insert_stmt(std::string_view time, const LogRecord& record, std::optional<std::int64_t> run_id) const
//--------------------------------------------------------------
void Logger::SQLogger::SQLiteDbDeleter::operator()(sqlite3* db) const {
    //--------------------------
    sqlite3_close_v2(db);
    //--------------------------
}// end void Logger::SQLogger::SQLiteDbDeleter::operator()(sqlite3* db) const
//--------------------------------------------------------------
void Logger::SQLogger::SQLiteStmtDeleter::operator()(sqlite3_stmt* stmt) const {
    //--------------------------
    if (stmt) {
        sqlite3_finalize(stmt);
    }// end if (stmt)
    //--------------------------
}// end void Logger::SQLogger::SQLiteStmtDeleter::operator()(sqlite3_stmt* stmt) const
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
std::unique_ptr<sqlite3_stmt, Logger::SQLogger::SQLiteStmtDeleter> Logger::SQLogger::prepare_insert(void) const {
    //--------------------------
    using StmtPtr = std::unique_ptr<sqlite3_stmt, SQLiteStmtDeleter>;
    //--------------------------
    if (!m_initialized or !m_db) { // guard against a failed / not-yet-opened connection
        return StmtPtr(nullptr);
    }// end if (!m_initialized or !m_db)
    //--------------------------
    constexpr std::string_view insert_sql = "INSERT INTO logs (timestamp, log_level, function_name, message, line_number, file_name, run_id) VALUES (?, ?, ?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt = nullptr;
    //--------------------------
    if (sqlite3_prepare_v2(m_db.get(), insert_sql.data(), -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Error preparing insert statement: " << sqlite3_errmsg(m_db.get()) << std::endl;
        return StmtPtr(nullptr);
    }// end if (sqlite3_prepare_v2(...) != SQLITE_OK)
    //--------------------------
    return StmtPtr(stmt);
    //--------------------------
}// end std::unique_ptr<sqlite3_stmt, SQLiteStmtDeleter> Logger::SQLogger::prepare_insert(void) const
