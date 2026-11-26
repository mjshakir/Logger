#pragma once
//--------------------------------------------------------------
// Standard cpp library
//--------------------------------------------------------------
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
//--------------------------------------------------------------
// User Defined library
//--------------------------------------------------------------
#include "LoggerTypes.hpp"
#include "UnfinishedRuns.hpp" // UnfinishedRuns::Finalize
#if HAS_LOGGER_SQL == 1
    #include "SQLogger.hpp"
#endif
//--------------------------------------------------------------
namespace Logger {
    //--------------------------------------------------------------
    // Forward declarations
    //--------------------------------------------------------------
    struct LogRecord;
    //--------------------------------------------------------------
    /// The place a message is written, and the owner of every statement of the database: the
    /// database, or the text files in a build without SQLite. The session allocates it once and
    /// publishes it to the logging threads.
    //--------------------------------------------------------------
    class LogStore {
        //--------------------------------------------------------------
        public:
            //--------------------------------------------------------------
            /// The folder is kept; the database opens at the folder joined to the file name.
            explicit LogStore(const LogPlace& place);
            //--------------------------
            LogStore(const LogStore&)               = delete;
            LogStore& operator=(const LogStore&)    = delete;
            LogStore(LogStore&&)                    = delete;
            LogStore& operator=(LogStore&&)         = delete;
            ~LogStore(void)                         = default;
            //--------------------------
            /// True when a row can be stored.
            bool usable(void) const noexcept;
            //--------------------------
            /// Stores one row with the run's id once there is one.
            bool stored(const LogRecord& record) const;
            //--------------------------
            std::optional<std::string_view> folder(void) const noexcept;
            //--------------------------
            std::optional<std::string_view> database(void) const noexcept;
            //--------------------------
            /// A run's row and its state row in one BEGIN IMMEDIATE transaction; then later
            /// messages carry its id.
            std::expected<std::int64_t, LogError> registered(const RunIdentity& identity) noexcept;
            //--------------------------
            /// Writes a run's end once; WRITTEN only for the call whose UPDATE changed the row.
            std::expected<Ended, LogError> ended(std::string_view outcome, std::int64_t run) const noexcept;
            //--------------------------
            /// The statement for the runs below `before` with no end, at most `most`, ready to step.
            std::expected<std::unique_ptr<sqlite3_stmt, UnfinishedRuns::Finalize>, LogError>
            unfinished(std::int64_t before, std::size_t most) const noexcept;
            //--------------------------
            /// SQLite's result code as a named cause; the cursor reads its rows with the same one.
            static LogError refused(int code) noexcept;
            //--------------------------------------------------------------
        protected:
            //--------------------------------------------------------------
            // The database's path: the folder, a separator and the file name; the bare name when
            // there is no folder.
            std::string joined(const LogPlace& place) const;
            //--------------------------
            // The two inserts of the transaction, on the bookkeeping connection.
            std::expected<std::int64_t, LogError> inserted(const RunIdentity& identity) const noexcept;
            //--------------------------
            // One compare_exchange of the run atomic, 0 to `run`.
            void attached(std::int64_t run) noexcept;
            //--------------------------------------------------------------
        private:
            //--------------------------------------------------------------
            std::optional<std::string> m_folder;   // nothing: the working folder
            std::string                m_database; // the path, as the database opens it
#if HAS_LOGGER_SQL == 1
            SQLogger                   m_sql;
#endif
            // Written once, read by every message with no lock. SQLite ids start at 1, so 0
            // means "no run yet"; it is never returned.
            std::atomic<std::int64_t>  m_run;
        //--------------------------------------------------------------
    };// end class LogStore
    //--------------------------------------------------------------
}// end namespace Logger
//--------------------------------------------------------------
