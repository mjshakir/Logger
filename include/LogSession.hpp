#pragma once
//--------------------------------------------------------------
// Standard cpp library
//--------------------------------------------------------------
#include <cstdint>
#include <memory>
#include <string_view>
//--------------------------------------------------------------
// User Defined library
//--------------------------------------------------------------
#include "LoggerAPI.hpp"
//--------------------------------------------------------------
namespace Logger {
    //--------------------------------------------------------------
    // Forward declarations LogRecord
    //--------------------------------------------------------------
    struct LogRecord;
    //--------------------------------------------------------------
    // Where the messages are stored. A program or a run makes one LogSession; the Logger and every
    // LOG_ macro reach it by themselves.
    //
    // Contract:
    //  - Everything is given once, here in the constructor: the folder and the run the rows belong
    //    to. Nothing sets them afterwards. The destructor ends the session: it withdraws it, waits
    //    for the messages in flight and closes the database, so the folder can be removed after it.
    //  - With no live session a message goes to the console only. No file and no database is
    //    opened or made.
    //  - A refused folder or database (empty, cannot be made, cannot be opened) says so with one
    //    line on stderr, and the session is not live: the messages go to the console only. No file
    //    is made in its place.
    //  - One session is live at a time. A second one made while another lives is not live and
    //    does not disturb the first.
    //  - The session is made before the threads that log and is destroyed after they stop.
    //  - A message takes no lock of the library's: it announces itself in an atomic reader count,
    //    reads the one atomic pointer of the live session, stores and leaves. The destructor
    //    withdraws the pointer, then waits (bounded: 5 seconds) until no message is inside.
    //  - If that wait runs out, which is a violation of the line before, the session's block is
    //    left to leak and is never freed under a message.
    //  - Not copyable and not movable.
    //--------------------------------------------------------------
    class LOGGER_API LogSession {
        //--------------------------------------------------------------
        public:
            //--------------------------------------------------------------
            // The database logs.db by its bare name in the working folder: what the library did
            // before there were sessions. The run of its rows is none (NULL).
            LogSession(void);
            //--------------------------
            // The folder of the logs (made when missing, parents too) and the run the rows belong
            // to (1 or more; 0 or less stores NULL). The database logs.db is made in the folder.
            // Everything is done here, once.
            explicit LogSession(std::string_view directory, std::int64_t run_id = 0);
            //--------------------------
            LogSession(const LogSession&)               = delete;
            LogSession& operator=(const LogSession&)    = delete;
            LogSession(LogSession&&)                    = delete;
            LogSession& operator=(LogSession&&)         = delete;
            //--------------------------
            ~LogSession(void);
            //--------------------------
            // True when the folder and the database are usable and this is the session the messages
            // reach.
            bool live(void) const noexcept;
            //--------------------------
            // Called by Logger::logs(): stores one row (and in a build without SQLite one text
            // line) in the live session. False when there is no live session or the store failed.
            static bool store(const LogRecord& record);
            //--------------------------------------------------------------
        private:
            //--------------------------------------------------------------
            struct Impl; // the folder, the run and the database; defined in LogSession.cpp
            //--------------------------
            LogSession(std::string_view directory, std::int64_t run_id, bool bare_names);
            //--------------------------
            static std::unique_ptr<Impl> open(std::string_view directory, std::int64_t run_id,
                                              bool bare_names);
            //--------------------------
            std::unique_ptr<Impl> m_impl; // made once in the constructor, the only allocation
            //--------------------------
            bool m_live;
        //--------------------------------------------------------------
    };// end class LogSession
    //--------------------------------------------------------------
}// end namespace Logger
//--------------------------------------------------------------
