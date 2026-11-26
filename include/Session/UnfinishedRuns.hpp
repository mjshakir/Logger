#pragma once
//--------------------------------------------------------------
// Standard cpp library
//--------------------------------------------------------------
#include <cstddef>
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
// Forward declaration of SQLite's statement type; its header is never included here
//--------------------------------------------------------------
struct sqlite3_stmt;
//--------------------------------------------------------------
namespace Logger {
    //--------------------------------------------------------------
    // Forward declarations
    //--------------------------------------------------------------
    class LogRun;
    //--------------------------------------------------------------
    /// The runs older than `run` that have no end: at most `most`, oldest first, one row at a
    /// time. A refusal is the cursor's state: next() answers nothing and error() says why.
    /// Move only. End it before the run ends.
    //--------------------------------------------------------------
    class LOGGER_API UnfinishedRuns {
        //--------------------------------------------------------------
        public:
            //--------------------------------------------------------------
            /// Releases a statement: the deleter of the cursor's one statement.
            struct Finalize {
                void operator()(sqlite3_stmt* statement) const noexcept;
            };// end struct Finalize
            //--------------------------
            /// Prepares the statement. The loop that reads it is bounded by `most`.
            UnfinishedRuns(const LogRun& run, std::size_t most) noexcept;
            //--------------------------
            UnfinishedRuns(const UnfinishedRuns&)               = delete;
            UnfinishedRuns& operator=(const UnfinishedRuns&)    = delete;
            UnfinishedRuns(UnfinishedRuns&&)                    = default;
            UnfinishedRuns& operator=(UnfinishedRuns&&)         = default;
            ~UnfinishedRuns(void)                               = default;
            //--------------------------
            /// The next row, or nothing at the end or after a refusal. The texts are valid until
            /// the next call or the end of the cursor; copy what you keep. The last row releases
            /// the statement.
            std::optional<Unfinished> next(void) noexcept;
            //--------------------------
            /// Why the list ended early or never began; nothing when it ended by its last row.
            std::optional<LogError> error(void) const noexcept;
            //--------------------------------------------------------------
        protected:
            //--------------------------------------------------------------
            // The statement for the runs below `run`'s id, or the cause.
            std::expected<std::unique_ptr<sqlite3_stmt, Finalize>, LogError>
            prepared(const LogRun& run, std::size_t most) const noexcept;
            //--------------------------
            // The text of a column of the row the statement stands on; empty for NULL.
            std::string_view text(int column) const noexcept;
            //--------------------------------------------------------------
        private:
            //--------------------------------------------------------------
            // The statement, or the refusal: at the end the statement is released (null), after
            // a refused read the cause replaces it.
            std::expected<std::unique_ptr<sqlite3_stmt, Finalize>, LogError> m_statement;
        //--------------------------------------------------------------
    };// end class UnfinishedRuns
    //--------------------------------------------------------------
}// end namespace Logger
//--------------------------------------------------------------
