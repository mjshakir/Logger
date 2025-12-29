#pragma once
//--------------------------------------------------------------
// Standard cpp library
//--------------------------------------------------------------
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <version>
//--------------------------------------------------------------
// User Defined library
//--------------------------------------------------------------
#include "LoggerAPI.hpp"
#include "LogRecord.hpp"
//--------------------------------------------------------------
// Format library
//--------------------------------------------------------------
#if LOGGER_HAS_STD_FORMAT
    #include <format>
#else
    #include <fmt/core.h>
    #include <fmt/format.h>
#endif
//--------------------------------------------------------------
namespace Logger {
    //--------------------------------------------------------------
    class LOGGER_API Logger {
        //--------------------------------------------------------------
        private:
            //--------------------------------------------------------------
            template <typename T, typename = void>
            struct is_container : std::false_type {};
            //--------------------------
            template <typename T>
            struct is_container<T, std::void_t<
                decltype(std::declval<T>().begin()),
                decltype(std::declval<T>().end()),
                typename T::value_type
            >> : std::true_type {};
            //--------------------------
            template <typename T>
            static constexpr bool is_container_v = is_container<T>::value;
            //--------------------------
            template <typename T, typename = void>
            struct is_map : std::false_type {};
            //--------------------------
            template <typename T>
            struct is_map<T, std::void_t<
                typename T::key_type,
                typename T::mapped_type
            >> : std::true_type {};
            //--------------------------
            template <typename T>
            static constexpr bool is_map_v = is_map<T>::value;
            //--------------------------------------------------------------
        public:
            //--------------------------------------------------------------
            static Logger& instance(void);
            //--------------------------------------------------------------
            // Logging functions
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
            void info(std::string_view format, Args&&... args) {
                log(LogLevel::INFO, format, std::forward<Args>(args)...);
            } // end void info(std::string_view format, Args&&... args)
            //--------------------------
            template<typename T>
            void debug_stream(std::string_view message, const T& container) {
                log_stream(LogLevel::DEBUG, message, container);
            } // end void debug_stream(std::string_view message, const T& container)
            //--------------------------
            template<typename T>
            void error_stream(std::string_view message, const T& container) {
                log_stream(LogLevel::ERROR, message, container);
            } // end void error_stream(std::string_view message, const T& container)
            //--------------------------
            template<typename T>
            void warning_stream(std::string_view message, const T& container) {
                log_stream(LogLevel::WARNING, message, container);
            } // end void warning_stream(std::string_view message, const T& container)
            //--------------------------
            template<typename T>
            void info_stream(std::string_view message, const T& container) {
                log_stream(LogLevel::INFO, message, container);
            } // end void info_stream(std::string_view message, const T& container)
            //--------------------------------------------------------------
            // Function to log messages with function name
            //--------------------------
            template<typename... Args>
            void debug_function(std::string_view function_name, std::string_view format, Args&&... args) {
                log(LogLevel::DEBUG, function_name, format, std::forward<Args>(args)...);
            }// end void debug(std::string_view format, Args&&... args)
            //--------------------------
            template<typename... Args>
            void error_function(std::string_view function_name, std::string_view format, Args&&... args) {
                log(LogLevel::ERROR, function_name, format, std::forward<Args>(args)...);
            }// end void error(std::string_view format, Args&&... args)
            //--------------------------
            template<typename... Args>
            void warning_function(std::string_view function_name, std::string_view format, Args&&... args) {
                log(LogLevel::WARNING, function_name, format, std::forward<Args>(args)...);
            } // end void warning(std::string_view format, Args&&... args)
            //--------------------------
            template<typename... Args>
            void info_function(std::string_view function_name, std::string_view format, Args&&... args) {
                log(LogLevel::INFO, function_name, format, std::forward<Args>(args)...);
            } // end void info(std::string_view format, Args&&... args)
            //--------------------------
            template<typename T>
            void debug_stream_function(std::string_view function_name, std::string_view message, const T& container) {
                log_stream(LogLevel::DEBUG, function_name, message, container);
            } // end void debug_stream(std::string_view message, const T& container)
            //--------------------------
            template<typename T>
            void error_stream_function(std::string_view function_name, std::string_view message, const T& container) {
                log_stream(LogLevel::ERROR, function_name, message, container);
            } // end void error_stream(std::string_view message, const T& container)
            //--------------------------
            template<typename T>
            void warning_stream_function(std::string_view function_name, std::string_view message, const T& container) {
                log_stream(LogLevel::WARNING, function_name, message, container);
            } // end void warning_stream(std::string_view message, const T& container)
            //--------------------------
            template<typename T>
            void info_stream_function(std::string_view function_name, std::string_view message, const T& container) {
                log_stream(LogLevel::INFO, function_name, message, container);
            } // end void info_stream(std::string_view message, const T& container)
            //--------------------------------------------------------------
        protected:
            //--------------------------------------------------------------
            template<typename... Args>
            void log(const LogLevel& level, std::string_view format, Args&&... args) {
                //--------------------------
                const auto now = std::chrono::system_clock::now();
                //--------------------------
#if LOGGER_HAS_STD_FORMAT
                const std::string message = std::vformat(format, std::make_format_args(args...));
#else
                const std::string message = fmt::format(fmt::runtime(format), std::forward<Args>(args)...);
#endif
                //--------------------------
                const std::string _formatted_message = format_message(level, message, now);
                //--------------------------
                { // protect the log_file call
                    std::lock_guard<std::mutex> lock(m_mutex);
                    level_message({level, message, _formatted_message, std::nullopt, now});
                } // end protect the log_file call
                //--------------------------
            }// end void log(LogLevel level, std::string_view format, Args&&... args)
            //--------------------------
            template<typename... Args>
            void log(const LogLevel& level, std::string_view function_name, std::string_view format, Args&&... args) {
                //--------------------------
                const auto now = std::chrono::system_clock::now();
                //--------------------------
#if LOGGER_HAS_STD_FORMAT
                const std::string message = std::vformat(format, std::make_format_args(args...));
#else
                const std::string message = fmt::format(fmt::runtime(format), std::forward<Args>(args)...);
#endif
                //--------------------------
                const std::string _formatted_message = format_message(level, function_name, message, now);
                //--------------------------
                { // protect the log_file call
                    std::lock_guard<std::mutex> lock(m_mutex);
                    level_message({level, message, _formatted_message, function_name, now});
                } // end protect the log_file call
                //--------------------------
            }// end void log(LogLevel level, std::string_view function_name, std::string_view format, Args&&... args)
            //--------------------------
            void level_message(const LogRecord& record) const;
            //--------------------------
            constexpr std::string_view level_print(const LogLevel& level) const;
            //--------------------------
            constexpr std::string_view level_log(const LogLevel& level) const;
            //--------------------------
            std::string format_message(const LogLevel& level, std::string_view message, const std::chrono::system_clock::time_point& now) const;
            //--------------------------
            std::string format_message(const LogLevel& level, std::string_view function_name, std::string_view message, const std::chrono::system_clock::time_point& now) const;
            //--------------------------
            void log_file(std::string_view filename, std::string_view message, const std::optional<std::chrono::system_clock::time_point>& now = std::nullopt) const;
            //--------------------------
            constexpr std::string_view format_function_name(std::string_view function_name) const;
            //--------------------------
            template<typename T>
            void log_stream(const LogLevel& level, std::string_view message, const T& container) {
                //--------------------------
                const auto now = std::chrono::system_clock::now();
                //--------------------------
                std::string _message;
#if LOGGER_HAS_STD_FORMAT
                //--------------------------
                const std::string _container = print_container(container);
                _message.reserve(message.size() + 1 + _container.size());
                std::format_to(std::back_inserter(_message), "{} {}", message, _container);
                //--------------------------
                const std::string _formatted_message  = format_message(level, _message, now);
                
#else
                //--------------------------
                const std::string _container = print_container(container);
                fmt::memory_buffer _buffer;
                fmt::format_to(std::back_inserter(_buffer), "{} {}", message, _container);
                _message.assign(_buffer.data(), _buffer.size());
                //--------------------------
                const std::string _formatted_message  = format_message(level, _message, now);
#endif
                //--------------------------
                { // protect the log_file call
                    std::lock_guard<std::mutex> lock(m_mutex);
                    level_message({level, _message, _formatted_message, std::nullopt, now});
                } // end protect the log_file call
                //--------------------------
            }// end void log_stream(LogLevel level, std::string_view message, const T& container)
            //--------------------------
            template<typename T>
            void log_stream(const LogLevel& level, std::string_view function_name, std::string_view message, const T& container) {
                //--------------------------
                const auto now = std::chrono::system_clock::now();
                //--------------------------
                std::string _message;
#if LOGGER_HAS_STD_FORMAT
                //--------------------------
                const std::string _container = print_container(container);
                _message.reserve(message.size() + 1 + _container.size());
                std::format_to(std::back_inserter(_message), "{} {}", message, _container);
                //--------------------------
                const std::string _formatted_message  = format_message(level, function_name, _message, now);
#else
                //--------------------------
                const std::string _container = print_container(container);
                fmt::memory_buffer _buffer;
                fmt::format_to(std::back_inserter(_buffer), "{} {}", message, _container);
                _message.assign(_buffer.data(), _buffer.size());
                //--------------------------
                const std::string _formatted_message  = format_message(level, function_name, _message, now);
                //--------------------------
#endif
                { // protect the log_file call
                    std::lock_guard<std::mutex> lock(m_mutex);
                    level_message({level, _message, _formatted_message, function_name, now});
                } // end protect the log_file call
            }// end void log_stream(LogLevel level, std::string_view message, const T& container)
            //--------------------------
            template<typename T>
            std::string print_container(const T& container) const {
                if constexpr (is_map_v<T>) {
                    return print_map(container);
                } else if constexpr (is_container_v<T>) {
                    return print_general_container(container);
                } else if constexpr (std::is_enum_v<T>) {
                    return print_enum(container);
                } else {
                    return print_element(container);
                } // end if constexpr (is_map_v<T>)
            }// end std::string print_container(const T& container)
            //--------------------------
            template<typename T>
            std::string print_map(const T& container) const {
#if LOGGER_HAS_STD_FORMAT
                //--------------------------
                std::string _result;
                _result.reserve(container.size() * 20UL); // Rough estimation for capacity
                _result += "{"; // Start with "{" instead of "{{" for std::format
                //--------------------------
                for (const auto& [key, value] : container) {
                    std::format_to(std::back_inserter(_result), "{{{}: {}}}, ", key, value);
                } // end for(const auto& [key, value] : container)
                //--------------------------
#else
                //--------------------------
                fmt::memory_buffer _buffer;
                fmt::format_to(std::back_inserter(_buffer), "{{");
                //--------------------------
                for (const auto& [key, value] : container) {
                    fmt::format_to(std::back_inserter(_buffer), "{{{}: {}}}, ", key, value);
                } // end for(const auto& [key, value] : container)
                //--------------------------
                std::string _result = std::string(_buffer.data(), _buffer.size());
                //--------------------------
#endif
                //--------------------------
                if (_result.size() > 3) { // Remove trailing ", " if present
                    _result.erase(_result.size() - 2, 2);
                } // end if (_result.size() > 3)
                //--------------------------
                _result += "}";
                //--------------------------
                return _result;
                //--------------------------
            } // end std::string print_map(const T& container)
            //--------------------------
            template<typename T>
            std::string print_general_container(const T& container) const {
#if LOGGER_HAS_STD_FORMAT
                //--------------------------
                std::string _result;
                _result.reserve(container.size() * 20UL); // Rough estimation
                _result += "["; // Start with "[" instead of "{{" for std::format
                //--------------------------
                for (const auto& element : container) {
                    std::format_to(std::back_inserter(_result), "{}, ", element);
                } // end for(const auto& element : container)
                //--------------------------
#else
                //--------------------------
                fmt::memory_buffer _buffer;
                fmt::format_to(std::back_inserter(_buffer), "[");
                //--------------------------
                for (const auto& element : container) {
                    fmt::format_to(std::back_inserter(_buffer), "{}, ", element);
                } // end for(const auto& element : container)
                //--------------------------
                std::string _result = std::string(_buffer.data(), _buffer.size());
                //--------------------------
#endif
                //--------------------------
                if (_result.size() > 2) { // Remove trailing ", " if present
                    _result.erase(_result.size() - 2, 2);
                } // end if (_result.size() > 2)
                //--------------------------
                _result += "]";
                //--------------------------
                return _result;
            } // end std::string print_general_container(const T& container)
            //--------------------------
            template<typename T>
            std::string print_enum(const T& element) const {
#if LOGGER_HAS_STD_FORMAT
                return std::format("{}", static_cast<std::underlying_type_t<T>>(element));
#else
                return fmt::format("{}", static_cast<std::underlying_type_t<T>>(element));
#endif
            } // end std::string print_enum(const T& element)
            //--------------------------
            template<typename T>
            std::string print_element(const T& element) const {
#if LOGGER_HAS_STD_FORMAT
                return std::format("{}", element);
#else
                return fmt::format("{}", element);
#endif
            }// end std::string print_element(const T& element)
            //--------------------------
            void print_file(std::string_view message) const;
            //--------------------------
            void logs(const LogRecord& record) const;
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
// Logging macros are defined in `log.hpp`.
