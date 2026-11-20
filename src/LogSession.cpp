//--------------------------------------------------------------
// Main Header
//--------------------------------------------------------------
#include "LogSession.hpp"
//--------------------------------------------------------------
// Standard cpp library
//--------------------------------------------------------------
#include <atomic>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <new>
#include <string>
#include <system_error>
#include <thread>
#if HAS_LOGGER_SQL == 0
    #include <fstream>
#endif
//--------------------------------------------------------------
// User Defined library
//--------------------------------------------------------------
#include "LogRecord.hpp"
#if HAS_LOGGER_SQL == 1
    #include "SQLogger.hpp"
#endif
//--------------------------------------------------------------
// Definitions
//--------------------------------------------------------------
namespace {
    //--------------------------------------------------------------
    // The two process-wide atomics of the protocol. They are defined here and in no header, so a
    // shared and a static build both have exactly one of each, as Logger::instance() has.
    // s_live is the Impl of the live session, or null. s_readers counts the messages inside
    // store().
    // Every operation on them is seq_cst: either a message sees the null pointer and uses nothing,
    // or its increment comes before the destructor's read of the count and the destructor waits.
    //--------------------------------------------------------------
    std::atomic<void*> s_live{nullptr};
    //--------------------------
    std::atomic<std::uint32_t> s_readers{0U};
    //--------------------------
    constexpr std::chrono::seconds C_WAIT{5};
    //--------------------------
    constexpr std::string_view C_DATABASE = "logs.db";
    //--------------------------------------------------------------
    // One message inside store(): counted in the constructor, released in the destructor, so that
    // every way out of store() gives the count back.
    //--------------------------------------------------------------
    class Reading {
        //--------------------------------------------------------------
        public:
            //--------------------------------------------------------------
            Reading(void) noexcept {
                s_readers.fetch_add(1U, std::memory_order_seq_cst);
            }// end Reading(void) noexcept
            //--------------------------
            Reading(const Reading&)             = delete;
            Reading& operator=(const Reading&)  = delete;
            Reading(Reading&&)                  = delete;
            Reading& operator=(Reading&&)       = delete;
            //--------------------------
            ~Reading(void) {
                s_readers.fetch_sub(1U, std::memory_order_seq_cst);
            }// end ~Reading(void)
        //--------------------------------------------------------------
    };// end class Reading
    //--------------------------------------------------------------
}// end namespace
//--------------------------------------------------------------
// The block of a live session: what a message uses.
//--------------------------------------------------------------
struct Logger::LogSession::Impl {
    //--------------------------------------------------------------
    Impl(std::string_view folder, std::string_view database, std::int64_t run_id) :
#if HAS_LOGGER_SQL == 1
            m_database(database),
#else
            m_folder(folder),
#endif
            m_run_id(run_id) {
        //--------------------------
#if HAS_LOGGER_SQL == 1
        (void)folder;
#else
        (void)database;
#endif
        //--------------------------
    }// end Impl(std::string_view folder, std::string_view database, std::int64_t run_id)
    //--------------------------
    Impl(const Impl&)               = delete;
    Impl& operator=(const Impl&)    = delete;
    Impl(Impl&&)                    = delete;
    Impl& operator=(Impl&&)         = delete;
    ~Impl(void)                     = default;
    //--------------------------
    bool usable(void) const noexcept {
#if HAS_LOGGER_SQL == 1
        return m_database.opened();
#else
        return true;
#endif
    }// end bool usable(void) const noexcept
    //--------------------------
    bool store(const LogRecord& record) const {
#if HAS_LOGGER_SQL == 1
        return m_database.log(record, m_run_id);
#else
        // A build without SQLite keeps the text files, in the session's folder.
        std::string _path;
        _path.reserve(m_folder.size() + 1UL + 16UL);
        _path.append(m_folder);
        _path.append(m_folder.empty() ? "" : "/");
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
    }// end bool store(const LogRecord& record) const
    //--------------------------
#if HAS_LOGGER_SQL == 1
    SQLogger m_database;
#else
    std::string m_folder; // "" is the working folder
#endif
    std::int64_t m_run_id;
    //--------------------------------------------------------------
};// end struct Logger::LogSession::Impl
//--------------------------------------------------------------
// Public
//--------------------------------------------------------------
Logger::LogSession::LogSession(void) : LogSession(std::string_view{}, 0, true) {
    //--------------------------
}// end Logger::LogSession::LogSession(void)
//--------------------------------------------------------------
Logger::LogSession::LogSession(std::string_view directory, std::int64_t run_id) :
        LogSession(directory, run_id, false) {
    //--------------------------
}// end Logger::LogSession::LogSession(std::string_view directory, std::int64_t run_id)
//--------------------------------------------------------------
Logger::LogSession::~LogSession(void) {
    //--------------------------
    if (!m_live) {
        return;
    }// end if (!m_live)
    //--------------------------
    // Withdrawn first: from here on a new message sees no session. Then the messages that got in
    // before are waited for. Both loops of the wait are bounded by the clock.
    (void)s_live.exchange(nullptr, std::memory_order_seq_cst);
    //--------------------------
    const auto _deadline = std::chrono::steady_clock::now() + C_WAIT;
    while (s_readers.load(std::memory_order_seq_cst) != 0U and
           std::chrono::steady_clock::now() < _deadline) {
        std::this_thread::yield();
    }// end while (a message is inside and the time is not up)
    //--------------------------
    if (s_readers.load(std::memory_order_seq_cst) != 0U) {
        // A message did not leave in time: the block is left to leak, never freed under it.
        std::cerr << "Logger: LogSession ended while a message was still inside; its block is "
                     "left allocated." << std::endl;
        (void)m_impl.release();
    }// end if (a message is still inside)
    //--------------------------
}// end Logger::LogSession::~LogSession(void)
//--------------------------------------------------------------
bool Logger::LogSession::live(void) const noexcept {
    //--------------------------
    return m_live;
    //--------------------------
}// end bool Logger::LogSession::live(void) const noexcept
//--------------------------------------------------------------
bool Logger::LogSession::store(const LogRecord& record) {
    //--------------------------
    const Reading _reading;
    //--------------------------
    const void* const p_live = s_live.load(std::memory_order_seq_cst);
    //--------------------------
    if (p_live == nullptr) {
        return false;
    }// end if (p_live == nullptr)
    //--------------------------
    return static_cast<const Impl*>(p_live)->store(record);
    //--------------------------
}// end bool Logger::LogSession::store(const LogRecord& record)
//--------------------------------------------------------------
// Private
//--------------------------------------------------------------
Logger::LogSession::LogSession(std::string_view directory, std::int64_t run_id, bool bare_names) :
        m_impl(open(directory, run_id, bare_names)),
        m_live(m_impl != nullptr) {
    //--------------------------
}// end Logger::LogSession::LogSession(directory, run_id, bare_names)
//--------------------------------------------------------------
std::unique_ptr<Logger::LogSession::Impl> Logger::LogSession::open(std::string_view directory,
                                                                    std::int64_t run_id,
                                                                    bool bare_names) {
    //--------------------------
    if (s_live.load(std::memory_order_seq_cst) != nullptr) {
        std::cerr << "Logger: LogSession is not live: another session is live, and it keeps the "
                     "messages." << std::endl;
        return nullptr;
    }// end if (another session is live)
    //--------------------------
    std::string _database;
    //--------------------------
    if (!bare_names) {
        //--------------------------
        const std::filesystem::path _path(directory);
        std::error_code _error;
        bool _usable = !directory.empty() and directory.find('\0') == std::string_view::npos;
        //--------------------------
        if (_usable) {
            std::filesystem::create_directories(_path, _error);
            _usable = !_error and std::filesystem::is_directory(_path, _error);
        }// end if (_usable)
        //--------------------------
        if (!_usable) {
            std::cerr << "Logger: LogSession is not live: cannot use the folder '" << directory
                      << "'; the messages go to the console only." << std::endl;
            return nullptr;
        }// end if (!_usable)
        //--------------------------
        _database.reserve(directory.size() + 1UL + C_DATABASE.size());
        _database.append(directory);
        _database.push_back('/');
    } else {
        _database.reserve(C_DATABASE.size());
    }// end if (!bare_names)
    _database.append(C_DATABASE);
    //--------------------------
    std::unique_ptr<Impl> _impl(new (std::nothrow) Impl(directory, _database, run_id));
    //--------------------------
    if (_impl == nullptr or !_impl->usable()) {
        std::cerr << "Logger: LogSession is not live: cannot open the database '" << _database
                  << "'; the messages go to the console only." << std::endl;
        return nullptr;
    }// end if (no block or no database)
    //--------------------------
    void* p_expected = nullptr;
    //--------------------------
    if (!s_live.compare_exchange_strong(p_expected, _impl.get(), std::memory_order_seq_cst)) {
        std::cerr << "Logger: LogSession is not live: another session is live, and it keeps the "
                     "messages." << std::endl;
        return nullptr;
    }// end if (a session took the place first)
    //--------------------------
    return _impl;
    //--------------------------
}// end std::unique_ptr<Impl> Logger::LogSession::open(...)
//--------------------------------------------------------------
