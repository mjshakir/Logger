//--------------------------------------------------------------
// Main Header
//--------------------------------------------------------------
#include "UnfinishedRuns.hpp"
//--------------------------------------------------------------
// Standard cpp library
//--------------------------------------------------------------
#include <cstdint>
#include <utility>
//--------------------------------------------------------------
// SQLite library
//--------------------------------------------------------------
#if HAS_LOGGER_SQL == 1
    #include <sqlite3.h>
#endif
//--------------------------------------------------------------
// User Defined library
//--------------------------------------------------------------
#include "LogRun.hpp"
#include "LogStore.hpp"
//--------------------------------------------------------------
// Finalize
//--------------------------------------------------------------
void Logger::UnfinishedRuns::Finalize::operator()(sqlite3_stmt* statement) const noexcept {
    //--------------------------
#if HAS_LOGGER_SQL == 1
    if (statement != nullptr) {
        (void)sqlite3_finalize(statement);
    }// end if (statement != nullptr)
#else
    (void)statement;
#endif
    //--------------------------
}// end void Logger::UnfinishedRuns::Finalize::operator()(sqlite3_stmt* statement) const noexcept
//--------------------------------------------------------------
// Public
//--------------------------------------------------------------
Logger::UnfinishedRuns::UnfinishedRuns(const LogRun& run, std::size_t most) noexcept :
        m_statement(prepared(run, most)) {
    //--------------------------
}// end Logger::UnfinishedRuns::UnfinishedRuns(const LogRun& run, std::size_t most) noexcept
//--------------------------------------------------------------
std::optional<Logger::Unfinished> Logger::UnfinishedRuns::next(void) noexcept {
    //--------------------------
    if (!m_statement.has_value() or *m_statement == nullptr) {
        return std::nullopt;
    }// end if (refused, or already at the end)
    //--------------------------
#if HAS_LOGGER_SQL == 1
    const int _code = sqlite3_step(m_statement->get());
    //--------------------------
    if (_code == SQLITE_ROW) {
        return Unfinished(sqlite3_column_int64(m_statement->get(), 0), text(1),
                          RunIdentity(text(2), text(3), sqlite3_column_int64(m_statement->get(), 4)));
    }// end if (a row)
    //--------------------------
    if (_code == SQLITE_DONE) {
        // No read stays open once the list is read.
        m_statement->reset();
        return std::nullopt;
    }// end if (the end of the list)
    //--------------------------
    m_statement = std::unexpected(LogStore::refused(_code));
#endif
    return std::nullopt;
    //--------------------------
}// end std::optional<Unfinished> Logger::UnfinishedRuns::next(void) noexcept
//--------------------------------------------------------------
std::optional<Logger::LogError> Logger::UnfinishedRuns::error(void) const noexcept {
    //--------------------------
    if (m_statement.has_value()) {
        return std::nullopt;
    }// end if (no refusal)
    //--------------------------
    return m_statement.error();
    //--------------------------
}// end std::optional<LogError> Logger::UnfinishedRuns::error(void) const noexcept
//--------------------------------------------------------------
// Protected
//--------------------------------------------------------------
std::expected<std::unique_ptr<sqlite3_stmt, Logger::UnfinishedRuns::Finalize>, Logger::LogError>
Logger::UnfinishedRuns::prepared(const LogRun& run, std::size_t most) const noexcept {
    //--------------------------
    const std::expected<std::int64_t, LogError> _id = run.id();
    if (!_id.has_value()) {
        return std::unexpected(_id.error());
    }// end if (the run has no row to be older than)
    //--------------------------
    const std::expected<LogStore*, LogError> _store = run.session().place();
    if (!_store.has_value()) {
        return std::unexpected(_store.error());
    }// end if (the session is not live)
    //--------------------------
    return (*_store)->unfinished(*_id, most);
    //--------------------------
}// end std::expected<std::unique_ptr<sqlite3_stmt, Finalize>, LogError> Logger::UnfinishedRuns::prepared(run, most) const noexcept
//--------------------------------------------------------------
std::string_view Logger::UnfinishedRuns::text(int column) const noexcept {
    //--------------------------
#if HAS_LOGGER_SQL == 1
    const unsigned char* const p_text = sqlite3_column_text(m_statement->get(), column);
    //--------------------------
    if (p_text == nullptr) {
        return {};
    }// end if (NULL reads as an empty text)
    //--------------------------
    return {reinterpret_cast<const char*>(p_text), static_cast<std::size_t>(sqlite3_column_bytes(m_statement->get(), column))};
#else
    (void)column;
    return {};
#endif
    //--------------------------
}// end std::string_view Logger::UnfinishedRuns::text(int column) const noexcept
//--------------------------------------------------------------
