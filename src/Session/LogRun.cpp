//--------------------------------------------------------------
// Main Header
//--------------------------------------------------------------
#include "LogRun.hpp"
//--------------------------------------------------------------
// Standard cpp library
//--------------------------------------------------------------
#include <iostream>
//--------------------------------------------------------------
// User Defined library
//--------------------------------------------------------------
#include "LogStore.hpp"
//--------------------------------------------------------------
// Public
//--------------------------------------------------------------
Logger::LogRun::LogRun(const LogPlace& place, const RunIdentity& identity) :
        m_session(place),
        m_id(registered(identity)) {
    //--------------------------
    if (m_session.live() and !m_id.has_value()) {
        // Said once, by name. The session stays live: its messages are stored with no run.
        std::cerr << "Logger: the run's row was refused (" << name(m_id.error())
                  << "); the messages are stored with no run." << std::endl;
    }// end if (the run's row was refused)
    //--------------------------
}// end Logger::LogRun::LogRun(const LogPlace& place, const RunIdentity& identity)
//--------------------------------------------------------------
std::expected<std::int64_t, Logger::LogError> Logger::LogRun::id(void) const noexcept {
    //--------------------------
    return m_id;
    //--------------------------
}// end std::expected<std::int64_t, LogError> Logger::LogRun::id(void) const noexcept
//--------------------------------------------------------------
std::expected<Logger::Ended, Logger::LogError> Logger::LogRun::ended(std::string_view outcome) const noexcept {
    //--------------------------
    if (!m_id.has_value()) {
        return std::unexpected(m_id.error());
    }// end if (this run has no row)
    //--------------------------
    return ended(outcome, *m_id);
    //--------------------------
}// end std::expected<Ended, LogError> Logger::LogRun::ended(std::string_view outcome) const noexcept
//--------------------------------------------------------------
std::expected<Logger::Ended, Logger::LogError> Logger::LogRun::ended(std::string_view outcome, std::int64_t run) const noexcept {
    //--------------------------
    const std::expected<LogStore*, LogError> _store = m_session.place();
    if (!_store.has_value()) {
        return std::unexpected(_store.error());
    }// end if (the session is not live)
    //--------------------------
    return (*_store)->ended(outcome, run);
    //--------------------------
}// end std::expected<Ended, LogError> Logger::LogRun::ended(std::string_view outcome, std::int64_t run) const noexcept
//--------------------------------------------------------------
const Logger::LogSession& Logger::LogRun::session(void) const noexcept {
    //--------------------------
    return m_session;
    //--------------------------
}// end const LogSession& Logger::LogRun::session(void) const noexcept
//--------------------------------------------------------------
// Protected
//--------------------------------------------------------------
std::expected<std::int64_t, Logger::LogError> Logger::LogRun::registered(const RunIdentity& identity) const noexcept {
    //--------------------------
    const std::expected<LogStore*, LogError> _store = m_session.place();
    if (!_store.has_value()) {
        return std::unexpected(_store.error());
    }// end if (the session is not live)
    //--------------------------
    return (*_store)->registered(identity);
    //--------------------------
}// end std::expected<std::int64_t, LogError> Logger::LogRun::registered(const RunIdentity& identity) const noexcept
//--------------------------------------------------------------
