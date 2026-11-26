//--------------------------------------------------------------
// Main Header
//--------------------------------------------------------------
#include "LogStore.hpp"
//--------------------------------------------------------------
// Standard cpp library
//--------------------------------------------------------------
#include <utility>
#if HAS_LOGGER_SQL == 0
    #include <fstream>
#endif
//--------------------------------------------------------------
// SQLite library
//--------------------------------------------------------------
#if HAS_LOGGER_SQL == 1
    #include <sqlite3.h>
#endif
//--------------------------------------------------------------
// User Defined library
//--------------------------------------------------------------
#include "LogRecord.hpp"
//--------------------------------------------------------------
// RETURNING (the one statement that says whether THIS call changed a row, when other threads write
// on the same database) came with SQLite 3.35. Without it the build would compile and count wrong.
#if HAS_LOGGER_SQL == 1
static_assert(SQLITE_VERSION_NUMBER >= 3035000, "SQLite 3.35 or newer is needed for RETURNING");
#endif
//--------------------------------------------------------------
// Public
//--------------------------------------------------------------
Logger::LogStore::LogStore(const LogPlace& place) :
        m_folder(place.directory.has_value() ? std::optional<std::string>(std::string(*place.directory)) : std::nullopt),
        m_database(joined(place)),
#if HAS_LOGGER_SQL == 1
        m_sql(m_database),
#endif
        m_run(0) {
    //--------------------------
}// end Logger::LogStore::LogStore(const LogPlace& place)
//--------------------------------------------------------------
bool Logger::LogStore::usable(void) const noexcept {
    //--------------------------
#if HAS_LOGGER_SQL == 1
    return m_sql.opened();
#else
    return true;
#endif
    //--------------------------
}// end bool Logger::LogStore::usable(void) const noexcept
//--------------------------------------------------------------
bool Logger::LogStore::stored(const LogRecord& record) const {
    //--------------------------
#if HAS_LOGGER_SQL == 1
    const std::int64_t _run = m_run.load(std::memory_order_relaxed);
    return m_sql.log(record, _run != 0 ? std::optional<std::int64_t>(_run) : std::nullopt);
#else
    // A build without SQLite keeps the text files, in the session's folder.
    std::string _path;
    _path.reserve((m_folder.has_value() ? m_folder->size() + 1UL : 0UL) + 16UL);
    if (m_folder.has_value()) {
        _path.append(*m_folder);
        _path.push_back('/');
    }// end if (the session has a folder)
    switch (record.level) {
        case LogLevel::DEBUG:
            _path.append("debug_log.txt");
            break;
        case LogLevel::ERROR:
            _path.append("error_log.txt");
            break;
        case LogLevel::WARNING:
            _path.append("warning_log.txt");
            break;
        case LogLevel::INFO:
            _path.append("info_log.txt");
            break;
    }// end switch (record.level)
    std::ofstream _file(_path, std::ios_base::app);
    if (!_file.is_open()) {
        return false;
    }// end if (!_file.is_open())
    _file << record.formatted_message << '\n';
    return static_cast<bool>(_file);
#endif
    //--------------------------
}// end bool Logger::LogStore::stored(const LogRecord& record) const
//--------------------------------------------------------------
std::optional<std::string_view> Logger::LogStore::folder(void) const noexcept {
    //--------------------------
    if (!m_folder.has_value()) {
        return std::nullopt;
    }// end if (no folder)
    //--------------------------
    return std::string_view{*m_folder};
    //--------------------------
}// end std::optional<std::string_view> Logger::LogStore::folder(void) const noexcept
//--------------------------------------------------------------
std::optional<std::string_view> Logger::LogStore::database(void) const noexcept {
    //--------------------------
#if HAS_LOGGER_SQL == 1
    return std::string_view{m_database};
#else
    return std::nullopt;
#endif
    //--------------------------
}// end std::optional<std::string_view> Logger::LogStore::database(void) const noexcept
//--------------------------------------------------------------
std::expected<std::int64_t, Logger::LogError> Logger::LogStore::registered(const RunIdentity& identity) noexcept {
    //--------------------------
#if HAS_LOGGER_SQL == 1
    const std::expected<sqlite3*, LogError> _control = m_sql.control();
    if (!_control.has_value()) {
        return std::unexpected(_control.error());
    }// end if (no bookkeeping connection)
    //--------------------------
    // BEGIN IMMEDIATE takes the write lock now and honors the busy wait, so two programs that start
    // together queue here and each gets its own id.
    const int _begin = sqlite3_exec(*_control, "BEGIN IMMEDIATE;", nullptr, nullptr, nullptr);
    if (_begin != SQLITE_OK) {
        return std::unexpected(refused(_begin));
    }// end if (_begin != SQLITE_OK)
    //--------------------------
    const std::expected<std::int64_t, LogError> _id = inserted(identity);
    const int _end = sqlite3_exec(*_control, _id.has_value() ? "COMMIT;" : "ROLLBACK;", nullptr, nullptr, nullptr);
    //--------------------------
    if (_id.has_value() and _end != SQLITE_OK) {
        // The commit was refused (full disk, a lock): nothing of the run stays.
        (void)sqlite3_exec(*_control, "ROLLBACK;", nullptr, nullptr, nullptr);
        return std::unexpected(refused(_end));
    }// end if (_id.has_value() and _end != SQLITE_OK)
    //--------------------------
    if (_id.has_value()) {
        attached(*_id);
    }// end if (the run is committed)
    //--------------------------
    return _id;
#else
    (void)identity;
    return std::unexpected(LogError::NO_SQL);
#endif
    //--------------------------
}// end std::expected<std::int64_t, LogError> Logger::LogStore::registered(const RunIdentity& identity) noexcept
//--------------------------------------------------------------
std::expected<Logger::Ended, Logger::LogError> Logger::LogStore::ended(std::string_view outcome, std::int64_t run) const noexcept {
    //--------------------------
#if HAS_LOGGER_SQL == 1
    const std::expected<sqlite3*, LogError> _control = m_sql.control();
    if (!_control.has_value()) {
        return std::unexpected(_control.error());
    }// end if (no bookkeeping connection)
    //--------------------------
    // RETURNING answers a row only for the call that changed one: the count of the connection
    // (sqlite3_changes) could be another thread's.
    sqlite3_stmt* _raw = nullptr;
    const int _prepared = sqlite3_prepare_v2(*_control,
        "UPDATE run_state SET ended = datetime('now', 'localtime'), outcome = ?1 "
        "WHERE run_id = ?2 AND ended IS NULL RETURNING run_id;", -1, &_raw, nullptr);
    const std::unique_ptr<sqlite3_stmt, UnfinishedRuns::Finalize> _statement(_raw);
    if (_prepared != SQLITE_OK or !_statement) {
        return std::unexpected(refused(_prepared));
    }// end if (_prepared != SQLITE_OK or !_statement)
    //--------------------------
    if (sqlite3_bind_text(_statement.get(), 1, outcome.data(), static_cast<int>(outcome.size()), SQLITE_STATIC) != SQLITE_OK or
        sqlite3_bind_int64(_statement.get(), 2, static_cast<sqlite3_int64>(run)) != SQLITE_OK) {
        return std::unexpected(LogError::REFUSED);
    }// end if (a bind was refused)
    //--------------------------
    const int _first = sqlite3_step(_statement.get());
    if (_first != SQLITE_ROW and _first != SQLITE_DONE) {
        return std::unexpected(refused(_first));
    }// end if (the update was refused)
    //--------------------------
    // The statement is stepped to its end: only then is the change committed.
    const int _last = (_first == SQLITE_ROW) ? sqlite3_step(_statement.get()) : _first;
    if (_last != SQLITE_DONE) {
        return std::unexpected(refused(_last));
    }// end if (the end of the statement was refused)
    //--------------------------
    return (_first == SQLITE_ROW) ? Ended::WRITTEN : Ended::ALREADY;
#else
    (void)outcome;
    (void)run;
    return std::unexpected(LogError::NO_SQL);
#endif
    //--------------------------
}// end std::expected<Ended, LogError> Logger::LogStore::ended(std::string_view outcome, std::int64_t run) const noexcept
//--------------------------------------------------------------
std::expected<std::unique_ptr<sqlite3_stmt, Logger::UnfinishedRuns::Finalize>, Logger::LogError>
Logger::LogStore::unfinished(std::int64_t before, std::size_t most) const noexcept {
    //--------------------------
#if HAS_LOGGER_SQL == 1
    const std::expected<sqlite3*, LogError> _control = m_sql.control();
    if (!_control.has_value()) {
        return std::unexpected(_control.error());
    }// end if (no bookkeeping connection)
    //--------------------------
    sqlite3_stmt* _raw = nullptr;
    const int _prepared = sqlite3_prepare_v2(*_control,
        "SELECT id, started, name, program, pid FROM unfinished WHERE id < ?1 ORDER BY id LIMIT ?2;",
        -1, &_raw, nullptr);
    std::unique_ptr<sqlite3_stmt, UnfinishedRuns::Finalize> _statement(_raw);
    if (_prepared != SQLITE_OK or !_statement) {
        return std::unexpected(refused(_prepared));
    }// end if (_prepared != SQLITE_OK or !_statement)
    //--------------------------
    if (sqlite3_bind_int64(_statement.get(), 1, static_cast<sqlite3_int64>(before)) != SQLITE_OK or
        sqlite3_bind_int64(_statement.get(), 2, static_cast<sqlite3_int64>(most)) != SQLITE_OK) {
        return std::unexpected(LogError::REFUSED);
    }// end if (a bind was refused)
    //--------------------------
    return std::expected<std::unique_ptr<sqlite3_stmt, UnfinishedRuns::Finalize>, LogError>(std::move(_statement));
#else
    (void)before;
    (void)most;
    return std::unexpected(LogError::NO_SQL);
#endif
    //--------------------------
}// end std::expected<std::unique_ptr<sqlite3_stmt, UnfinishedRuns::Finalize>, LogError> Logger::LogStore::unfinished(before, most) const noexcept
//--------------------------------------------------------------
Logger::LogError Logger::LogStore::refused(int code) noexcept {
    //--------------------------
#if HAS_LOGGER_SQL == 1
    switch (code & 0xFF) { // the primary result code
        case SQLITE_BUSY:
        case SQLITE_LOCKED:
            return LogError::LOCKED;
        case SQLITE_NOMEM:
            return LogError::NO_MEMORY;
        default:
            return LogError::REFUSED;
    }// end switch (code & 0xFF)
#else
    (void)code;
    return LogError::NO_SQL;
#endif
    //--------------------------
}// end Logger::LogError Logger::LogStore::refused(int code) noexcept
//--------------------------------------------------------------
// Protected
//--------------------------------------------------------------
std::string Logger::LogStore::joined(const LogPlace& place) const {
    //--------------------------
    std::string _path;
    //--------------------------
    if (place.directory.has_value()) {
        _path.reserve(place.directory->size() + 1UL + place.database.size());
        _path.append(*place.directory);
        _path.push_back('/');
    } else {
        _path.reserve(place.database.size());
    }// end if (the database is in a folder)
    _path.append(place.database);
    //--------------------------
    return _path;
    //--------------------------
}// end std::string Logger::LogStore::joined(const LogPlace& place) const
//--------------------------------------------------------------
std::expected<std::int64_t, Logger::LogError> Logger::LogStore::inserted(const RunIdentity& identity) const noexcept {
    //--------------------------
#if HAS_LOGGER_SQL == 1
    const std::expected<sqlite3*, LogError> _control = m_sql.control();
    if (!_control.has_value()) {
        return std::unexpected(_control.error());
    }// end if (no bookkeeping connection)
    //--------------------------
    sqlite3_stmt* _raw_run = nullptr;
    const int _prepared_run = sqlite3_prepare_v2(*_control, "INSERT INTO run (name, program) VALUES (?1, ?2) RETURNING id;", -1, &_raw_run, nullptr);
    const std::unique_ptr<sqlite3_stmt, UnfinishedRuns::Finalize> _run(_raw_run);
    if (_prepared_run != SQLITE_OK or !_run) {
        return std::unexpected(refused(_prepared_run));
    }// end if (the run's insert was refused)
    //--------------------------
    // A text with no characters is the empty string and not NULL (a run with no name has none; the
    // columns are NOT NULL): SQLite binds NULL for a null pointer, so an empty view is given an
    // address of its own.
    const char* const _p_name    = identity.name.empty() ? "" : identity.name.data();
    const char* const _p_program = identity.program.empty() ? "" : identity.program.data();
    if (sqlite3_bind_text(_run.get(), 1, _p_name, static_cast<int>(identity.name.size()), SQLITE_STATIC) != SQLITE_OK or
        sqlite3_bind_text(_run.get(), 2, _p_program, static_cast<int>(identity.program.size()), SQLITE_STATIC) != SQLITE_OK) {
        return std::unexpected(LogError::REFUSED);
    }// end if (a bind was refused)
    //--------------------------
    const int _stepped = sqlite3_step(_run.get());
    if (_stepped != SQLITE_ROW) {
        return std::unexpected(refused(_stepped));
    }// end if (no row came back)
    const std::int64_t _id = sqlite3_column_int64(_run.get(), 0);
    if (const int _done = sqlite3_step(_run.get()); _done != SQLITE_DONE) {
        return std::unexpected(refused(_done));
    }// end if (the statement did not end)
    //--------------------------
    sqlite3_stmt* _raw_state = nullptr;
    const int _prepared_state = sqlite3_prepare_v2(*_control, "INSERT INTO run_state (run_id, pid) VALUES (?1, ?2);", -1, &_raw_state, nullptr);
    const std::unique_ptr<sqlite3_stmt, UnfinishedRuns::Finalize> _state(_raw_state);
    if (_prepared_state != SQLITE_OK or !_state) {
        return std::unexpected(refused(_prepared_state));
    }// end if (the state's insert was refused)
    //--------------------------
    if (sqlite3_bind_int64(_state.get(), 1, static_cast<sqlite3_int64>(_id)) != SQLITE_OK or
        sqlite3_bind_int64(_state.get(), 2, static_cast<sqlite3_int64>(identity.process)) != SQLITE_OK) {
        return std::unexpected(LogError::REFUSED);
    }// end if (a bind was refused)
    //--------------------------
    if (const int _state_done = sqlite3_step(_state.get()); _state_done != SQLITE_DONE) {
        return std::unexpected(refused(_state_done));
    }// end if (the state row was refused)
    //--------------------------
    return _id;
#else
    (void)identity;
    return std::unexpected(LogError::NO_SQL);
#endif
    //--------------------------
}// end std::expected<std::int64_t, LogError> Logger::LogStore::inserted(const RunIdentity& identity) const noexcept
//--------------------------------------------------------------
void Logger::LogStore::attached(std::int64_t run) noexcept {
    //--------------------------
    std::int64_t _none = 0;
    (void)m_run.compare_exchange_strong(_none, run, std::memory_order_relaxed);
    //--------------------------
}// end void Logger::LogStore::attached(std::int64_t run) noexcept
//--------------------------------------------------------------
