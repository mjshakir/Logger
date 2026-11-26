#pragma once
//--------------------------------------------------------------
// Standard cpp library
//--------------------------------------------------------------
#include <cstdint>
#include <expected>
#include <string_view>
//--------------------------------------------------------------
// User Defined library
//--------------------------------------------------------------
#include "LoggerAPI.hpp"
#include "LoggerTypes.hpp"
#include "LogSession.hpp"
//--------------------------------------------------------------
namespace Logger {
    //--------------------------------------------------------------
    /// One run: its session and its row, from where the messages go and who the run is. The
    /// constructor makes the session, then the run's row and its state row in one transaction,
    /// and every message after it carries the run's id. A refusal is kept: id() says why. The
    /// destructor writes no end: a run never told ended() is found by the next start, which is
    /// how a crash is seen.
    ///
    /// Example:
    /// @code
    /// Logger::LogRun run(Logger::LogPlace(folder, "logs.db"), Logger::RunIdentity("a", "b", pid));
    /// for (Logger::UnfinishedRuns old(run, 16uz); const auto row = old.next();) {
    ///     run.ended("LOST", row->run);
    /// }
    /// run.ended("FINISHED");
    /// @endcode
    //--------------------------------------------------------------
    class LOGGER_API LogRun {
        //--------------------------------------------------------------
        public:
            //--------------------------------------------------------------
            /// The messages go to `place`; the run is `identity` (name, then program, then process).
            LogRun(const LogPlace& place, const RunIdentity& identity);
            //--------------------------
            LogRun(const LogRun&)               = delete;
            LogRun& operator=(const LogRun&)    = delete;
            LogRun(LogRun&&)                    = delete;
            LogRun& operator=(LogRun&&)         = delete;
            ~LogRun(void)                       = default;
            //--------------------------
            /// The run's id, or why there is none.
            std::expected<std::int64_t, LogError> id(void) const noexcept;
            //--------------------------
            /// Writes this run's end once. LOCKED when the database stayed locked past the
            /// library's busy wait: the run stays open and the next start finds it.
            std::expected<Ended, LogError> ended(std::string_view outcome) const noexcept;
            //--------------------------
            /// The same for an older run, such as one the cursor listed.
            std::expected<Ended, LogError> ended(std::string_view outcome, std::int64_t run) const noexcept;
            //--------------------------
            /// The run's session, for its own queries: live(), folder(), database().
            const LogSession& session(void) const noexcept;
            //--------------------------------------------------------------
        protected:
            //--------------------------------------------------------------
            // The run's rows through the session's store; the cause when there is none.
            std::expected<std::int64_t, LogError> registered(const RunIdentity& identity) const noexcept;
            //--------------------------------------------------------------
        private:
            //--------------------------------------------------------------
            LogSession                            m_session; // first: made before the run, ended after it
            std::expected<std::int64_t, LogError> m_id;
        //--------------------------------------------------------------
    };// end class LogRun
    //--------------------------------------------------------------
}// end namespace Logger
//--------------------------------------------------------------
