#pragma once

#if defined(LOGGER_STATIC)
    #define LOGGER_API
    #define LOGGER_LOCAL
#elif defined(_WIN32) || defined(__CYGWIN__)
    #if defined(LOGGER_BUILDING_LIBRARY)
        #define LOGGER_API __declspec(dllexport)
    #else
        #define LOGGER_API __declspec(dllimport)
    #endif
    #define LOGGER_LOCAL
#else
    #if defined(__GNUC__) && (__GNUC__ >= 4)
        #define LOGGER_API __attribute__((visibility("default")))
        #define LOGGER_LOCAL __attribute__((visibility("hidden")))
    #else
        #define LOGGER_API
        #define LOGGER_LOCAL
    #endif
#endif


#ifndef LOGGER_HAS_STD_FORMAT
    #if defined(__cpp_lib_format) && (__cpp_lib_format >= 201907L)
        #define LOGGER_HAS_STD_FORMAT 1
    #else
        #define LOGGER_HAS_STD_FORMAT 0
    #endif
#endif

#ifndef LOGGER_USE_STD_FORMAT_STRING
    #define LOGGER_USE_STD_FORMAT_STRING 0
#endif

#ifndef LOGGER_HAS_STD_PRINT
    #if defined(__cpp_lib_print) && (__cpp_lib_print >= 202207L)
        #define LOGGER_HAS_STD_PRINT 1
    #else
        #define LOGGER_HAS_STD_PRINT 0
    #endif
#endif

#if LOGGER_HAS_STD_PRINT && !LOGGER_HAS_STD_FORMAT
    #undef LOGGER_HAS_STD_PRINT
    #define LOGGER_HAS_STD_PRINT 0
#endif
