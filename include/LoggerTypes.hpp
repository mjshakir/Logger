#pragma once
//--------------------------------------------------------------
// Standard cpp library
//--------------------------------------------------------------
#include <concepts>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
//--------------------------------------------------------------
// User Defined library
//--------------------------------------------------------------
#include "LoggerAPI.hpp"
//--------------------------------------------------------------
namespace Logger {
    //--------------------------------------------------------------
    /// Why a run's bookkeeping was refused. Every answer that can fail carries one.
    //--------------------------------------------------------------
    enum class LogError : std::uint8_t {
        NO_SQL     = 0U, // the library was built without SQLite
        NO_SESSION = 1U, // the session is not live
        NO_MEMORY  = 2U, // memory was refused
        LOCKED     = 3U, // another writer still held the database after the busy wait
        REFUSED    = 4U  // the database refused: full, not a database, or its file is not usable
    };// end enum class LogError
    //--------------------------------------------------------------
    constexpr std::string_view name(const LogError error) noexcept {
        switch (error) {
            case LogError::NO_SQL:
                return "NO_SQL";
            case LogError::NO_SESSION:
                return "NO_SESSION";
            case LogError::NO_MEMORY:
                return "NO_MEMORY";
            case LogError::LOCKED:
                return "LOCKED";
            case LogError::REFUSED:
                return "REFUSED";
        }// end switch (error)
        return "UNNAMED";
    }// end constexpr std::string_view name(const LogError error) noexcept
    //--------------------------------------------------------------
    /// What ended() answers when the database took the statement.
    //--------------------------------------------------------------
    enum class Ended : bool {
        ALREADY = false, // no open end: another call wrote it first, or there is no such run
        WRITTEN = true   // this call's UPDATE changed the row
    };// end enum class Ended
    //--------------------------------------------------------------
    constexpr std::string_view name(const Ended ended) noexcept {
        switch (ended) {
            case Ended::ALREADY:
                return "ALREADY";
            case Ended::WRITTEN:
                return "WRITTEN";
        }// end switch (ended)
        return "UNNAMED";
    }// end constexpr std::string_view name(const Ended ended) noexcept
    //--------------------------------------------------------------
    /// True for an argument that is a temporary std::string: a view of it would dangle at the end
    /// of the statement. The structs below delete the constructor that would take one.
    //--------------------------------------------------------------
    template <typename T>
    concept TemporaryString = std::same_as<std::remove_cvref_t<T>, std::string> and
                              not std::is_lvalue_reference_v<T>;
    //--------------------------------------------------------------
    /// Where the messages are stored. The texts must live as long as the place is used.
    //--------------------------------------------------------------
    struct LOGGER_API LogPlace {
        public:
            //--------------------------------------------------------------
            LogPlace(void)                          = delete;
            //--------------------------
            /// `directory` is made when missing; nothing means the working folder. `database` is
            /// the file name in it. The folder comes first, then the file.
            LogPlace(std::optional<std::string_view> directory_, std::string_view database_) noexcept;
            //--------------------------
            // A temporary string would dangle.
            template <typename Directory, typename Database>
            requires (TemporaryString<Directory> or TemporaryString<Database>)
            LogPlace(Directory&& directory_, Database&& database_) = delete;
            //--------------------------
            LogPlace(const LogPlace&)               = default;
            LogPlace& operator=(const LogPlace&)    = default;
            LogPlace(LogPlace&&)                    = default;
            LogPlace& operator=(LogPlace&&)         = default;
            ~LogPlace(void)                         = default;
            //--------------------------
            std::optional<std::string_view> directory;
            std::string_view                database;
        //--------------------------------------------------------------
    };// end struct LogPlace
    //--------------------------------------------------------------
    /// Who a run is. The texts must live as long as the identity is used.
    //--------------------------------------------------------------
    struct LOGGER_API RunIdentity {
        public:
            //--------------------------------------------------------------
            RunIdentity(void)                           = delete;
            //--------------------------
            /// The order is name, program, process: the name and the program are both texts, so
            /// keep it. The name may be empty; the process is the system's id of the process.
            RunIdentity(std::string_view name_, std::string_view program_, std::int64_t process_) noexcept;
            //--------------------------
            // A temporary string would dangle.
            template <typename Name, typename Program>
            requires (TemporaryString<Name> or TemporaryString<Program>)
            RunIdentity(Name&& name_, Program&& program_, std::int64_t process_) = delete;
            //--------------------------
            RunIdentity(const RunIdentity&)             = default;
            RunIdentity& operator=(const RunIdentity&)  = default;
            RunIdentity(RunIdentity&&)                  = default;
            RunIdentity& operator=(RunIdentity&&)       = default;
            ~RunIdentity(void)                          = default;
            //--------------------------
            std::string_view name;
            std::string_view program;
            std::int64_t     process;
        //--------------------------------------------------------------
    };// end struct RunIdentity
    //--------------------------------------------------------------
    /// An earlier run with no end. The texts are views into the row the cursor stands on: they
    /// live until the next call of next().
    //--------------------------------------------------------------
    struct LOGGER_API Unfinished {
        public:
            //--------------------------------------------------------------
            Unfinished(void)                            = delete;
            //--------------------------
            /// The run's id, when it began (as the database writes it), and who it is.
            Unfinished(std::int64_t run_, std::string_view started_, const RunIdentity& identity_) noexcept;
            //--------------------------
            // A temporary string would dangle.
            template <typename Started>
            requires TemporaryString<Started>
            Unfinished(std::int64_t run_, Started&& started_, const RunIdentity& identity_) = delete;
            //--------------------------
            Unfinished(const Unfinished&)               = default;
            Unfinished& operator=(const Unfinished&)    = default;
            Unfinished(Unfinished&&)                    = default;
            Unfinished& operator=(Unfinished&&)         = default;
            ~Unfinished(void)                           = default;
            //--------------------------
            std::int64_t     run;
            std::string_view started;
            RunIdentity      identity;
        //--------------------------------------------------------------
    };// end struct Unfinished
    //--------------------------------------------------------------
}// end namespace Logger
//--------------------------------------------------------------
