#pragma once
//--------------------------------------------------------------
// Standard cpp library
//--------------------------------------------------------------
#include <iostream>
#include <mutex>
#include <chrono>
#include <functional>
#include <type_traits>
#include <mutex>
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
    class Logger {
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
        protected:
            //--------------------------------------------------------------
            enum class LogLevel : uint8_t {
                DEBUG   = 1 << 0,
                ERROR   = 1 << 1,
                WARNING = 1 << 2,
                INFO    = 1 << 3
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
            void info(std::string_view format, Args&&... args) {
                log(LogLevel::INFO, format, std::forward<Args>(args)...);
            } // end void info(std::string_view format, Args&&... args)
            //--------------------------
            template<typename T>
            void debug_stream(const std::string& message, const T& container) {
                log_stream(LogLevel::DEBUG, message, container);
            } // end void debug_stream(const std::string& message, const T& container)
            //--------------------------
            template<typename T>
            void error_stream(const std::string& message, const T& container) {
                log_stream(LogLevel::ERROR, message, container);
            } // end void error_stream(const std::string& message, const T& container)
            //--------------------------
            template<typename T>
            void warning_stream(const std::string& message, const T& container) {
                log_stream(LogLevel::WARNING, message, container);
            } // end void warning_stream(const std::string& message, const T& container)
            //--------------------------
            template<typename T>
            void info_stream(const std::string& message, const T& container) {
                log_stream(LogLevel::INFO, message, container);
            } // end void info_stream(const std::string& message, const T& container)
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
            constexpr std::string_view level_print(const LogLevel& level) const;
            //--------------------------
            std::string format_message(const LogLevel& level, std::string_view message, const std::chrono::system_clock::time_point& now) const;
            //--------------------------
            void log_file(const std::string& filename, std::string_view message) const;
            //--------------------------
            template<typename T>
            void log_stream(const LogLevel& level, const std::string& message, const T& container) {
                //--------------------------
                std::lock_guard<std::mutex> lock(m_mutex);
                auto now = std::chrono::system_clock::now();
                //--------------------------
                std::string container_str = print_container(container);
                std::string combined_message = message + " " + container_str;
                std::string _formatted_message = format_message(level, combined_message, now);
                //--------------------------
                level_message(level, _formatted_message);
                //--------------------------
            }// end void log_stream(LogLevel level, const std::string& message, const T& container)
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
#if __cpp_lib_format
                std::string result = std::format("{{");
                for (const auto& [key, value] : container) {
                    result += std::format("{{{}: {}}}, ", key, value);
                } // end for(const auto& [key, value] : container)
#else
                std::string result = fmt::format("{{");
                for (const auto& [key, value] : container) {
                    result += fmt::format("{{{}: {}}}, ", key, value);
                } // end for(const auto& [key, value] : container)
#endif
                //--------------------------
                if (result.size() > 3) { // Remove trailing ", " if present
                    result.erase(result.size() - 2, 2);
                } // end if (result.size() > 3)
                //--------------------------
                result += "}";
                //--------------------------
                return result;
            } // end std::string print_map(const T& container)
            //--------------------------
            template<typename T>
            std::string print_general_container(const T& container) const {
#if __cpp_lib_format
                std::string result = std::format("[");
                for (const auto& element : container) {
                    result += std::format("{}, ", element);
                } // end for(const auto& element : container)
#else
                std::string result = fmt::format("[");
                for (const auto& element : container) {
                    result += fmt::format("{}, ", element);
                } // end for(const auto& element : container)
#endif
                //--------------------------
                if (result.size() > 2) { // Remove trailing ", " if present
                    result.erase(result.size() - 2, 2);
                } // end if (result.size() > 2)
                //--------------------------
                result += "]";
                //--------------------------
                return result;
            } // end std::string print_general_container(const T& container)
            //--------------------------
            template<typename T>
            std::string print_enum(const T& element) const {
#if __cpp_lib_format
                return std::format("{}", static_cast<std::underlying_type_t<T>>(element));
#else
                return fmt::format("{}", static_cast<std::underlying_type_t<T>>(element));
#endif
            } // end std::string print_enum(const T& element)
            //--------------------------
            template<typename T>
            std::string print_element(const T& element) const {
#if __cpp_lib_format
                return std::format("{}", element);
#else
                return fmt::format("{}", element);
#endif
            }
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
// Existing logging macros
//--------------------------------------------------------------
#define LOG_ERROR(msg, ...) Logger::Logger::instance().error(msg, ##__VA_ARGS__)
#define LOG_WARNING(msg, ...) Logger::Logger::instance().warning(msg, ##__VA_ARGS__)
#define LOG_INFO(msg, ...) Logger::Logger::instance().info(msg, ##__VA_ARGS__)
#define LOG_ERROR_STREAM(msg, container) Logger::Logger::instance().error_stream(msg, container)
#define LOG_WARNING_STREAM(msg, container) Logger::Logger::instance().warning_stream(msg, container)
#define LOG_INFO_STREAM(msg, container) Logger::Logger::instance().info_stream(msg, container)

//--------------------------------------------------------------
// **Helper macros for unique variable names using __LINE__**
//--------------------------------------------------------------
#define CONCATENATE_DETAIL(x, y) x##y
#define CONCATENATE(x, y) CONCATENATE_DETAIL(x, y)
#define UNIQUE_VAR(base) CONCATENATE(base, __LINE__)

//--------------------------------------------------------------
// **Logging macros for conditional logging with DEBUG**
//--------------------------------------------------------------
#ifdef LOGGER_DEBUG
    #define LOG_DEBUG(msg, ...) Logger::Logger::instance().debug(msg, ##__VA_ARGS__)
    #define LOG_DEBUG_STREAM(msg, container) Logger::Logger::instance().debug_stream(msg, container)
    //--------------------------
    // **Newly added macros**
    #define LOG_WARNING_DEBUG(msg, ...) Logger::Logger::instance().warning(msg, ##__VA_ARGS__)
    #define LOG_WARNING_DEBUG_STREAM(msg, container) Logger::Logger::instance().warning_stream(msg, container)
    //--------------------------
    #define LOG_ERROR_DEBUG(msg, ...) Logger::Logger::instance().error(msg, ##__VA_ARGS__)
    #define LOG_ERROR_DEBUG_STREAM(msg, container) Logger::Logger::instance().error_stream(msg, container)
#else
    #define LOG_DEBUG(msg, ...)
    #define LOG_DEBUG_STREAM(msg, container)
    //--------------------------
    // **Ensure macros do nothing when DEBUG is not defined**
    #define LOG_WARNING_DEBUG(msg, ...)
    #define LOG_WARNING_DEBUG_STREAM(msg, container)
    //--------------------------
    #define LOG_ERROR_DEBUG(msg, ...)
    #define LOG_ERROR_DEBUG_STREAM(msg, container)
#endif

//--------------------------------------------------------------
// **Logging macros for logging messages only once**
//--------------------------------------------------------------
#define LOG_ONCE(level_method, msg, ...) do { \
    static std::once_flag UNIQUE_VAR(log_once_flag_); \
    std::call_once(UNIQUE_VAR(log_once_flag_), [&]{ \
        Logger::Logger::instance().level_method(msg, ##__VA_ARGS__); \
    }); \
} while(0)

#define LOG_ONCE_STREAM(level_method, msg, container) do { \
    static std::once_flag UNIQUE_VAR(log_once_flag_); \
    std::call_once(UNIQUE_VAR(log_once_flag_), [&]{ \
        Logger::Logger::instance().level_method##_stream(msg, container); \
    }); \
} while(0)

// **Define macros for each log level**
#define LOG_ERROR_ONCE(msg, ...) LOG_ONCE(error, msg, ##__VA_ARGS__)
#define LOG_WARNING_ONCE(msg, ...) LOG_ONCE(warning, msg, ##__VA_ARGS__)
#define LOG_INFO_ONCE(msg, ...) LOG_ONCE(info, msg, ##__VA_ARGS__)

#define LOG_ERROR_ONCE_STREAM(msg, container) LOG_ONCE_STREAM(error, msg, container)
#define LOG_WARNING_ONCE_STREAM(msg, container) LOG_ONCE_STREAM(warning, msg, container)
#define LOG_INFO_ONCE_STREAM(msg, container) LOG_ONCE_STREAM(info, msg, container)

#ifdef LOGGER_DEBUG
    #define LOG_DEBUG_ONCE(msg, ...) LOG_ONCE(debug, msg, ##__VA_ARGS__)
    #define LOG_DEBUG_ONCE_STREAM(msg, container) LOG_ONCE_STREAM(debug, msg, container)
#else
    #define LOG_DEBUG_ONCE(msg, ...)
    #define LOG_DEBUG_ONCE_STREAM(msg, container)
#endif
//--------------------------------------------------------------