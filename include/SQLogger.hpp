#pragma once
//--------------------------------------------------------------
// Standard cpp library
//--------------------------------------------------------------
#include <optional>
#include <memory>
#include <mutex>
//--------------------------------------------------------------
// Forward declarations for SQLite
//--------------------------------------------------------------
struct sqlite3;
//--------------------------
extern "C" {
    int sqlite3_close(sqlite3*);
}// end extern "C"
//--------------------------------------------------------------
namespace Logger {
    //--------------------------------------------------------------
    class SQLogger {
        //--------------------------------------------------------------
        public:
            //--------------------------------------------------------------
            static SQLogger& instance(void);
            //--------------------------
            bool log(std::string_view level, std::string_view message, const std::optional<std::chrono::system_clock::time_point>& now = std::nullopt) const;
            //--------------------------------------------------------------
        protected:
            //--------------------------------------------------------------
            bool initialize(void);
            //--------------------------
            bool log_message(std::string_view level, std::string_view message, const std::optional<std::chrono::system_clock::time_point>& now) const;
            //--------------------------------------------------------------
        private:
            //--------------------------------------------------------------
            SQLogger(void);
            ~SQLogger(void);
            //--------------------------
            SQLogger(const SQLogger&)               = delete;
            SQLogger& operator=(const SQLogger&)    = delete;
            SQLogger(SQLogger&&)                    = delete;
            SQLogger& operator=(SQLogger&&)         = delete;
            //--------------------------
            std::unique_ptr<sqlite3, decltype(&sqlite3_close)> m_db;
            const bool m_initialized;
            //--------------------------
            // std::mutex m_mutex;
        //--------------------------------------------------------------
    };// end class SQLogger
    //--------------------------------------------------------------
}// end namespace Logger
//--------------------------------------------------------------