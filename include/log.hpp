#pragma once
//--------------------------------------------------------------
// Main Header
//--------------------------------------------------------------
#include "Logger.hpp"
//--------------------------------------------------------------
// Standard cpp library
//--------------------------------------------------------------
#include <mutex>
//--------------------------------------------------------------
// Macros for Automatically Capturing Function Name and Class**
//--------------------------------------------------------------
#ifdef __GNUC__  // GCC/Clang
    #define FUNC_NAME __PRETTY_FUNCTION__ // __func__ __FUNCTION__ __PRETTY_FUNCTION__
#elif defined(_MSC_VER)  // MSVC
    #define FUNC_NAME __FUNCTION__
#else
    #define FUNC_NAME "UnknownFunction"
#endif
//--------------------------------------------------------------
// Existing logging macros
//--------------------------------------------------------------
#define LOG_ERROR(msg, ...) Logger::Logger::instance().error(msg __VA_OPT__(,) __VA_ARGS__)
#define LOG_WARNING(msg, ...) Logger::Logger::instance().warning(msg __VA_OPT__(,) __VA_ARGS__)
#define LOG_INFO(msg, ...) Logger::Logger::instance().info(msg __VA_OPT__(,) __VA_ARGS__)
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
    #define LOG_DEBUG(msg, ...) Logger::Logger::instance().debug(msg __VA_OPT__(,) __VA_ARGS__)
    #define LOG_DEBUG_STREAM(msg, container) Logger::Logger::instance().debug_stream(msg, container)
    //--------------------------
    // **Newly added macros**
    #define LOG_WARNING_DEBUG(msg, ...) Logger::Logger::instance().warning(msg __VA_OPT__(,) __VA_ARGS__)
    #define LOG_WARNING_DEBUG_STREAM(msg, container) Logger::Logger::instance().warning_stream(msg, container)
    //--------------------------
    #define LOG_ERROR_DEBUG(msg, ...) Logger::Logger::instance().error(msg __VA_OPT__(,) __VA_ARGS__)
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
#define LOG_ONCE_IMPL(level_method, flag_var, ...) do { \
    static std::once_flag flag_var; \
    std::call_once(flag_var, [&]{ \
        Logger::Logger::instance().level_method(__VA_ARGS__); \
    }); \
} while(0)

#define LOG_ONCE(level_method, ...) LOG_ONCE_IMPL(level_method, UNIQUE_VAR(log_once_flag_), __VA_ARGS__)

#define LOG_ONCE_STREAM_IMPL(level_method, flag_var, msg, container) do { \
    static std::once_flag flag_var; \
    std::call_once(flag_var, [&]{ \
        Logger::Logger::instance().level_method(msg, container); \
    }); \
} while(0)

#define LOG_ONCE_STREAM(level_method, msg, container) \
    LOG_ONCE_STREAM_IMPL(level_method, UNIQUE_VAR(log_once_flag_), msg, container)

// **Define macros for each log level**
#define LOG_ERROR_ONCE(...) LOG_ONCE(error, __VA_ARGS__)
#define LOG_WARNING_ONCE(...) LOG_ONCE(warning, __VA_ARGS__)
#define LOG_INFO_ONCE(...) LOG_ONCE(info, __VA_ARGS__)

#define LOG_ERROR_ONCE_STREAM(msg, container) LOG_ONCE_STREAM(error_stream, msg, container)
#define LOG_WARNING_ONCE_STREAM(msg, container) LOG_ONCE_STREAM(warning_stream, msg, container)
#define LOG_INFO_ONCE_STREAM(msg, container) LOG_ONCE_STREAM(info_stream, msg, container)

#ifdef LOGGER_DEBUG
    #define LOG_DEBUG_ONCE(...) LOG_ONCE(debug, __VA_ARGS__)
    #define LOG_DEBUG_ONCE_STREAM(msg, container) LOG_ONCE_STREAM(debug_stream, msg, container)
#else
    #define LOG_DEBUG_ONCE(...)
    #define LOG_DEBUG_ONCE_STREAM(msg, container)
#endif
//--------------------------------------------------------------
// Logging With Function Name and Class
//--------------------------------------------------------------
#define LOG_ERROR_FUNCTION(msg, ...) Logger::Logger::instance().error_function(FUNC_NAME, msg __VA_OPT__(,) __VA_ARGS__)
#define LOG_WARNING_FUNCTION(msg, ...) Logger::Logger::instance().warning_function(FUNC_NAME, msg __VA_OPT__(,) __VA_ARGS__)
#define LOG_INFO_FUNCTION(msg, ...) Logger::Logger::instance().info_function(FUNC_NAME, msg __VA_OPT__(,) __VA_ARGS__)
#define LOG_ERROR_FUNCTION_STREAM(msg, container) Logger::Logger::instance().error_stream_function(FUNC_NAME, msg, container)
#define LOG_WARNING_FUNCTION_STREAM(msg, container) Logger::Logger::instance().warning_stream_function(FUNC_NAME, msg, container)
#define LOG_INFO_FUNCTION_STREAM(msg, container) Logger::Logger::instance().info_stream_function(FUNC_NAME, msg, container)
//--------------------------------------------------------------
// **Logging macros for conditional logging with DEBUG**
//--------------------------------------------------------------
#ifdef LOGGER_DEBUG
    #define LOG_DEBUG_FUNCTION(msg, ...) Logger::Logger::instance().debug_function(FUNC_NAME, msg __VA_OPT__(,) __VA_ARGS__)
    #define LOG_DEBUG_FUNCTION_STREAM(msg, container) Logger::Logger::instance().debug_stream_function(FUNC_NAME, msg, container)
    //--------------------------
    // **Newly added macros**
    #define LOG_WARNING_DEBUG_FUNCTION(msg, ...) Logger::Logger::instance().warning_function(FUNC_NAME, msg __VA_OPT__(,) __VA_ARGS__)
    #define LOG_WARNING_DEBUG_FUNCTION_STREAM(msg, container) Logger::Logger::instance().warning_stream_function(FUNC_NAME, msg, container)
    //--------------------------
    #define LOG_ERROR_DEBUG_FUNCTION(msg, ...) Logger::Logger::instance().error_function(FUNC_NAME, msg __VA_OPT__(,) __VA_ARGS__)
    #define LOG_ERROR_DEBUG_FUNCTION_STREAM(msg, container) Logger::Logger::instance().error_stream_function(FUNC_NAME, msg, container)
#else
    #define LOG_DEBUG_FUNCTION(msg, ...)
    #define LOG_DEBUG_FUNCTION_STREAM(msg, container)
    //--------------------------
    // **Ensure macros do nothing when DEBUG is not defined**
    #define LOG_WARNING_DEBUG_FUNCTION(msg, ...)
    #define LOG_WARNING_DEBUG_FUNCTION_STREAM(msg, container)
    //--------------------------
    #define LOG_ERROR_DEBUG_FUNCTION(msg, ...)
    #define LOG_ERROR_DEBUG_FUNCTION_STREAM(msg, container)
#endif
//--------------------------------------------------------------
// **Logging macros for logging messages only once**
//--------------------------------------------------------------
#define LOG_ONCE_FUNCTION_IMPL(level_method, flag_var, ...) do { \
    static std::once_flag flag_var; \
    std::call_once(flag_var, [&]{ \
        Logger::Logger::instance().level_method(FUNC_NAME, __VA_ARGS__); \
    }); \
} while(0)

#define LOG_ONCE_FUNCTION(level_method, ...) \
    LOG_ONCE_FUNCTION_IMPL(level_method, UNIQUE_VAR(log_once_flag_), __VA_ARGS__)

#define LOG_ONCE_FUNCTION_STREAM_IMPL(level_method, flag_var, msg, container) do { \
    static std::once_flag flag_var; \
    std::call_once(flag_var, [&]{ \
        Logger::Logger::instance().level_method(FUNC_NAME, msg, container); \
    }); \
} while(0)

#define LOG_ONCE_FUNCTION_STREAM(level_method, msg, container) \
    LOG_ONCE_FUNCTION_STREAM_IMPL(level_method, UNIQUE_VAR(log_once_flag_), msg, container)

// **Define macros for each log level**
#define LOG_ERROR_FUNCTION_ONCE(...) LOG_ONCE_FUNCTION(error_function, __VA_ARGS__)
#define LOG_WARNING_FUNCTION_ONCE(...) LOG_ONCE_FUNCTION(warning_function, __VA_ARGS__)
#define LOG_INFO_FUNCTION_ONCE(...) LOG_ONCE_FUNCTION(info_function, __VA_ARGS__)

#define LOG_ERROR_ONCE_FUNCTION_STREAM(msg, container) LOG_ONCE_FUNCTION_STREAM(error_stream_function, msg, container)
#define LOG_WARNING_ONCE_FUNCTION_STREAM(msg, container) LOG_ONCE_FUNCTION_STREAM(warning_stream_function, msg, container)
#define LOG_INFO_ONCE_FUNCTION_STREAM(msg, container) LOG_ONCE_FUNCTION_STREAM(info_stream_function, msg, container)

#ifdef LOGGER_DEBUG
    #define LOG_DEBUG_FUNCTION_ONCE(...) LOG_ONCE_FUNCTION(debug_function, __VA_ARGS__)
    #define LOG_DEBUG_ONCE_FUNCTION_STREAM(msg, container) LOG_ONCE_FUNCTION_STREAM(debug_stream_function, msg, container)
#else
    #define LOG_DEBUG_FUNCTION_ONCE(...)
    #define LOG_DEBUG_ONCE_FUNCTION_STREAM(msg, container)
#endif

