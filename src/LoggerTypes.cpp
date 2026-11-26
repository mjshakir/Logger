//--------------------------------------------------------------
// Main Header
//--------------------------------------------------------------
#include "LoggerTypes.hpp"
//--------------------------------------------------------------
// LogPlace
//--------------------------------------------------------------
Logger::LogPlace::LogPlace(std::optional<std::string_view> directory_, std::string_view database_) noexcept :
        directory(directory_),
        database(database_) {
    //--------------------------
}// end Logger::LogPlace::LogPlace(directory_, database_) noexcept
//--------------------------------------------------------------
// RunIdentity
//--------------------------------------------------------------
Logger::RunIdentity::RunIdentity(std::string_view name_, std::string_view program_, std::int64_t process_) noexcept :
        name(name_),
        program(program_),
        process(process_) {
    //--------------------------
}// end Logger::RunIdentity::RunIdentity(name_, program_, process_) noexcept
//--------------------------------------------------------------
// Unfinished
//--------------------------------------------------------------
Logger::Unfinished::Unfinished(std::int64_t run_, std::string_view started_, const RunIdentity& identity_) noexcept :
        run(run_),
        started(started_),
        identity(identity_) {
    //--------------------------
}// end Logger::Unfinished::Unfinished(run_, started_, identity_) noexcept
//--------------------------------------------------------------
