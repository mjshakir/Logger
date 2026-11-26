//--------------------------------------------------------------
// Main Header
//--------------------------------------------------------------
#include "LogSession.hpp"
//--------------------------------------------------------------
// Standard cpp library
//--------------------------------------------------------------
#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <new>
#include <string>
#include <system_error>
#include <thread>
//--------------------------------------------------------------
// User Defined library
//--------------------------------------------------------------
#include "LogRecord.hpp"
#include "LogStore.hpp"
//--------------------------------------------------------------
// Definitions
//--------------------------------------------------------------
namespace {
    //--------------------------------------------------------------
    // The two process-wide atomics of the protocol. They are defined here and in no header, so a
    // shared and a static build both have exactly one of each, as Logger::instance() has.
    // s_live is the LogStore of the live session, or null. s_readers counts the messages inside
    // store().
    // Every operation on them is seq_cst: either a message sees the null pointer and uses nothing,
    // or its increment comes before the destructor's read of the count and the destructor waits.
    //--------------------------------------------------------------
    std::atomic<void*> s_live{nullptr};
    //--------------------------
    std::atomic<std::uint32_t> s_readers{0U};
    //--------------------------
    constexpr std::chrono::seconds C_WAIT{5};
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
// Public
//--------------------------------------------------------------
Logger::LogSession::LogSession(void) : LogSession(LogPlace(std::nullopt, C_DATABASE)) {
    //--------------------------
}// end Logger::LogSession::LogSession(void)
//--------------------------------------------------------------
Logger::LogSession::LogSession(const LogPlace& place) :
        m_store(made(place)),
        m_live(m_store != nullptr and published(*m_store)) {
    //--------------------------
}// end Logger::LogSession::LogSession(const LogPlace& place)
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
        (void)m_store.release();
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
    return static_cast<const LogStore*>(p_live)->stored(record);
    //--------------------------
}// end bool Logger::LogSession::store(const LogRecord& record)
//--------------------------------------------------------------
std::optional<std::string_view> Logger::LogSession::folder(void) const noexcept {
    //--------------------------
    if (!m_live) {
        return std::nullopt;
    }// end if (not live)
    //--------------------------
    return m_store->folder();
    //--------------------------
}// end std::optional<std::string_view> Logger::LogSession::folder(void) const noexcept
//--------------------------------------------------------------
std::optional<std::string_view> Logger::LogSession::database(void) const noexcept {
    //--------------------------
    if (!m_live) {
        return std::nullopt;
    }// end if (not live)
    //--------------------------
    return m_store->database();
    //--------------------------
}// end std::optional<std::string_view> Logger::LogSession::database(void) const noexcept
//--------------------------------------------------------------
std::expected<Logger::LogStore*, Logger::LogError> Logger::LogSession::place(void) const noexcept {
    //--------------------------
    if (!m_live) {
        return std::unexpected(LogError::NO_SESSION);
    }// end if (not live)
    //--------------------------
    return m_store.get();
    //--------------------------
}// end std::expected<LogStore*, LogError> Logger::LogSession::place(void) const noexcept
//--------------------------------------------------------------
// Protected
//--------------------------------------------------------------
bool Logger::LogSession::placed(const LogPlace& place) const {
    //--------------------------
    if (s_live.load(std::memory_order_seq_cst) != nullptr) {
        std::cerr << "Logger: LogSession is not live: another session is live, and it keeps the "
                     "messages." << std::endl;
        return false;
    }// end if (another session is live)
    //--------------------------
    if (place.database.empty() or place.database.find('\0') != std::string_view::npos) {
        std::cerr << "Logger: LogSession is not live: the database needs a name; the messages go "
                     "to the console only." << std::endl;
        return false;
    }// end if (no database name)
    //--------------------------
    if (!place.directory.has_value()) {
        return true;
    }// end if (the working folder)
    //--------------------------
    std::error_code _error;
    bool _usable = !place.directory->empty() and place.directory->find('\0') == std::string_view::npos;
    //--------------------------
    if (_usable) {
        const std::filesystem::path _directory(*place.directory);
        std::filesystem::create_directories(_directory, _error);
        _usable = !_error and std::filesystem::is_directory(_directory, _error);
    }// end if (_usable)
    //--------------------------
    if (!_usable) {
        std::cerr << "Logger: LogSession is not live: cannot use the folder '" << *place.directory
                  << "'; the messages go to the console only." << std::endl;
        return false;
    }// end if (!_usable)
    //--------------------------
    return true;
    //--------------------------
}// end bool Logger::LogSession::placed(const LogPlace& place) const
//--------------------------------------------------------------
std::unique_ptr<Logger::LogStore> Logger::LogSession::made(const LogPlace& place) const noexcept {
    //--------------------------
    std::unique_ptr<LogStore> _store;
    //--------------------------
    // The only throw left here is the standard library's own refusal of memory for a text; it is
    // caught and said, so a refused allocation is one more way of not being live.
    try {
        if (!placed(place)) {
            return nullptr;
        }// end if (the session cannot be placed)
        //--------------------------
        _store.reset(new (std::nothrow) LogStore(place));
    } catch (const std::bad_alloc&) {
        std::cerr << "Logger: LogSession is not live: memory was refused; the messages go to the "
                     "console only." << std::endl;
        return nullptr;
    }// end try (the texts and the store)
    //--------------------------
    if (_store == nullptr) {
        std::cerr << "Logger: LogSession is not live: memory was refused; the messages go to the "
                     "console only." << std::endl;
        return nullptr;
    }// end if (no store)
    //--------------------------
    if (!_store->usable()) {
        std::cerr << "Logger: LogSession is not live: cannot open the database '"
                  << _store->database().value_or(place.database) << "'; the messages go to the "
                     "console only." << std::endl;
        return nullptr;
    }// end if (no database)
    //--------------------------
    return _store;
    //--------------------------
}// end std::unique_ptr<LogStore> Logger::LogSession::made(const LogPlace& place) const noexcept
//--------------------------------------------------------------
bool Logger::LogSession::published(LogStore& store) const noexcept {
    //--------------------------
    void* p_expected = nullptr;
    //--------------------------
    if (!s_live.compare_exchange_strong(p_expected, &store, std::memory_order_seq_cst)) {
        std::cerr << "Logger: LogSession is not live: another session is live, and it keeps the "
                     "messages." << std::endl;
        return false;
    }// end if (a session took the place first)
    //--------------------------
    return true;
    //--------------------------
}// end bool Logger::LogSession::published(LogStore& store) const noexcept
//--------------------------------------------------------------
