#pragma once
//--------------------------------------------------------------
// Standard cpp library
//--------------------------------------------------------------
#include <iostream>
#include <mutex>
#include <chrono>
#include <functional>
//--------------------------------------------------------------
#if __cpp_lib_format
    #include <format>
#else
    #include <fmt/core.h>
    #include <fmt/chrono.h>
    #include <fmt/color.h>
#endif
//--------------------------------------------------------------
namespace Logger {
    //--------------------------------------------------------------
    
    //--------------------------------------------------------------
    class Logger {
        //--------------------------------------------------------------
        protected:
            //--------------------------------------------------------------
            enum class LogLevel : uint8_t {
                DEBUG   = 1 << 0,
                ERROR   = 1 << 1,
                WARNING = 1 << 2,
                NORMAL  = 1 << 3
            }; // end enum class LogLevel : uint8_t
            //--------------------------------------------------------------
        public:
            //--------------------------------------------------------------
            static Logger& instance(void);
            //--------------------------
            template<typename... Args>
            void debug(std::string_view format, Args&&... args) {
                log(LogLevel::DEBUG, format, std::forward<Args>(args)...);
            }// end void debug(std::string_view format, Args&&... args)
            //--------------------------
            template<typename... Args>
            void error(std::string_view format, Args&&... args) {
                log(LogLevel::ERROR, format, std::forward<Args>(args)...);
            }// end void error(std::string_view format, Args&&... args)
            //--------------------------
            template<typename... Args>
            void warning(std::string_view format, Args&&... args) {
                log(LogLevel::WARNING, format, std::forward<Args>(args)...);
            } // end void warning(std::string_view format, Args&&... args)
            //--------------------------
            template<typename... Args>
            void normal(std::string_view format, Args&&... args) {
                log(LogLevel::NORMAL, format, std::forward<Args>(args)...);
            } // end void normal(std::string_view format, Args&&... args)
            //--------------------------------------------------------------
        protected:
            //--------------------------------------------------------------
            template<typename... Args>
            void log(const LogLevel& level, std::string_view format, Args&&... args) {
                //--------------------------
                std::lock_guard<std::mutex> lock(m_mutex);
                auto now = std::chrono::system_clock::now();
                //--------------------------
#if __cpp_lib_format
                std::string message = std::vformat(format, std::make_format_args(args...));
#else
                std::string message = fmt::format(fmt::runtime(format), std::forward<Args>(args)...);
#endif
                //--------------------------
                std::string _formatted_message = format_message(level, message, now);
                //--------------------------
                level_message(level, _formatted_message);
                //--------------------------
            }// end void log(LogLevel level, std::string_view format, Args&&... args)
            //--------------------------
            void level_message(const LogLevel& level, std::string_view message) const;
            //--------------------------
            std::string level_print(const LogLevel& level) const;
            //--------------------------
            std::string format_message(const LogLevel& level, std::string_view message, const std::chrono::system_clock::time_point& now) const;
            //--------------------------
            void log_file(const std::string& filename, std::string_view message) const;
            //--------------------------------------------------------------
        private:
            //--------------------------------------------------------------
            Logger(void)                        = default;
            ~Logger(void)                       = default;
            //--------------------------
            Logger(const Logger&)               = delete;
            Logger& operator=(const Logger&)    = delete;
            Logger(Logger&&)                    = delete;
            Logger& operator=(Logger&&)         = delete;
            //--------------------------
            std::mutex m_mutex;
        //--------------------------------------------------------------
    }; // end class Logger
    //--------------------------------------------------------------
} // end namespace Logger
//--------------------------------------------------------------
#define LOG_DEBUG(msg, ...) Logger::Logger::instance().debug(msg, ##__VA_ARGS__)
#define LOG_ERROR(msg, ...) Logger::Logger::instance().error(msg, ##__VA_ARGS__)
#define LOG_WARNING(msg, ...) Logger::Logger::instance().warning(msg, ##__VA_ARGS__)
#define LOG_NORMAL(msg, ...) Logger::Logger::instance().normal(msg, ##__VA_ARGS__)
//--------------------------------------------------------------
#ifdef DEBUG
    #define DEBUG_PRINT(msg, ...) LOG_DEBUG(msg, __VA_ARGS__)
#else
    #define DEBUG_PRINT(msg, ...) 
#endif
//--------------------------------------------------------------