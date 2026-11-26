#pragma once
//--------------------------------------------------------------
// Standard cpp library
//--------------------------------------------------------------
#include <expected>
#include <memory>
#include <optional>
#include <string_view>
//--------------------------------------------------------------
// User Defined library
//--------------------------------------------------------------
#include "LoggerAPI.hpp"
#include "LoggerTypes.hpp"
//--------------------------------------------------------------
namespace Logger {
    //--------------------------------------------------------------
    // Forward declarations
    //--------------------------------------------------------------
    struct LogRecord;
    class LogStore; // what a message is written into; its header is private and not installed
    //--------------------------------------------------------------
    /// Where the messages are stored. Use it alone for a program with no run; a LogRun makes
    /// its own. One is live at a time; a refused or second one is not live, its messages go to
    /// the console only, and the refusal is named on stderr. The library's schema is applied
    /// when it opens.
    //--------------------------------------------------------------
    class LOGGER_API LogSession {
        //--------------------------------------------------------------
        public:
            //--------------------------------------------------------------
            /// The library's default database, by its bare name in the working folder.
            LogSession(void);
            //--------------------------
            /// `place.directory` is made when missing; nothing means the working folder, empty is
            /// refused. `place.database` is the file name in it.
            explicit LogSession(const LogPlace& place);
            //--------------------------
            LogSession(const LogSession&)               = delete;
            LogSession& operator=(const LogSession&)    = delete;
            LogSession(LogSession&&)                    = delete;
            LogSession& operator=(LogSession&&)         = delete;
            //--------------------------
            /// Withdraws the session and waits for the messages in flight; the wait is bounded
            /// (5 seconds) and a store left behind by a message that never left is leaked.
            ~LogSession(void);
            //--------------------------
            /// True when this is the session the messages reach.
            bool live(void) const noexcept;
            //--------------------------
            /// Called by Logger::logs(): stores one row. False when no session is live.
            static bool store(const LogRecord& record);
            //--------------------------
            /// The folder; nothing when not live or none. The view lives as long as the session.
            std::optional<std::string_view> folder(void) const noexcept;
            //--------------------------
            /// The database's path as opened; nothing when not live or the build has no SQLite.
            std::optional<std::string_view> database(void) const noexcept;
            //--------------------------
            /// The library's handle to the store: LogStore is not installed, so a caller outside
            /// the library can do nothing with it. NO_SESSION when not live.
            std::expected<LogStore*, LogError> place(void) const noexcept;
            //--------------------------------------------------------------
        protected:
            //--------------------------------------------------------------
            // No other session lives, the name is usable, the folder exists or was made.
            bool placed(const LogPlace& place) const;
            //--------------------------
            // The store, or nothing when placed() refused, memory was refused or the database
            // did not open. A refused text is caught here and said.
            std::unique_ptr<LogStore> made(const LogPlace& place) const noexcept;
            //--------------------------
            // Takes the one place of the live session; false when another took it first.
            bool published(LogStore& store) const noexcept;
            //--------------------------------------------------------------
        private:
            //--------------------------------------------------------------
            static constexpr std::string_view C_DATABASE = "logs.db"; // the default session's
            //--------------------------
            std::unique_ptr<LogStore> m_store; // made once; the only allocation
            //--------------------------
            bool m_live;
        //--------------------------------------------------------------
    };// end class LogSession
    //--------------------------------------------------------------
}// end namespace Logger
//--------------------------------------------------------------
