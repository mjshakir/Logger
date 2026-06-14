cmake_minimum_required(VERSION 3.15 FATAL_ERROR)
#------------------------------------------------------------------------------------------
# Determine if Logger is built as a standalone project or included by other projects
set(LOGGER_STANDALONE_PROJECT OFF)
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(LOGGER_STANDALONE_PROJECT ON)
endif()
#------------------------------------------------------------------------------------------
# Get the name of the folder and use it as the project name
get_filename_component(PARENT_DIR_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
string(REPLACE " " "_" PARENT_DIR_NAME ${PARENT_DIR_NAME})
project(${PARENT_DIR_NAME} VERSION 2.3.0 DESCRIPTION "A Logger function library" LANGUAGES CXX)
#------------------------------------------------------------------------------------------
# Formatting backend
option(LOGGER_FORCE_FMT "Use fmt even when std::format is available." ON)
option(LOGGER_USE_STD_FORMAT_STRING "Use std::format_string/std::format for Logger APIs (disables runtime format strings)." ON)
#------------------------------------------------------------------------------------------
# Detect C++23 library features (std::format / std::print)
include(CheckCXXSourceCompiles)

function(logger_try_compile_cxx23 _result_var _source)
    if(DEFINED ${_result_var})
        return()
    endif()

    set(_logger_try_dir "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/CMakeTmp/${_result_var}")
    file(MAKE_DIRECTORY "${_logger_try_dir}")
    file(WRITE "${_logger_try_dir}/main.cpp" "${_source}\n")

    message(CHECK_START "Performing Test ${_result_var}")
    try_compile(_logger_try_ok
        "${_logger_try_dir}/build"
        SOURCES "${_logger_try_dir}/main.cpp"
        CMAKE_FLAGS
            "-DCMAKE_CXX_STANDARD=23"
            "-DCMAKE_CXX_STANDARD_REQUIRED=ON"
            "-DCMAKE_CXX_EXTENSIONS=OFF"
    )
    if(_logger_try_ok)
        message(CHECK_PASS "Success")
        set(${_result_var} 1 CACHE INTERNAL "Test ${_result_var}")
    else()
        message(CHECK_FAIL "Failed")
        set(${_result_var} 0 CACHE INTERNAL "Test ${_result_var}")
    endif()
endfunction()
#------------------------------------------------------------------------------------------
logger_try_compile_cxx23(LOGGER_HAS_STD_FORMAT [==[
    #include <format>
    #include <string>

    int main() {
        auto s = std::format("{}", 42);
        return static_cast<int>(s.size() == 0);
    }
]==])
#---------------------------------------------
if(LOGGER_HAS_STD_FORMAT)
    logger_try_compile_cxx23(LOGGER_HAS_STD_PRINT [==[
        #include <print>

        int main() {
            std::print("{}", 42);
            return 0;
        }
    ]==])
endif()
#------------------------------------------------------------------------------------------
if(LOGGER_HAS_STD_FORMAT)
    set(LOGGER_HAS_STD_FORMAT 1)
else()
    set(LOGGER_HAS_STD_FORMAT 0)
endif()
#---------------------------------------------
if(LOGGER_HAS_STD_PRINT)
    set(LOGGER_HAS_STD_PRINT 1)
else()
    set(LOGGER_HAS_STD_PRINT 0)
endif()
#---------------------------------------------

if(LOGGER_FORCE_FMT)
    set(LOGGER_HAS_STD_FORMAT 0)
    set(LOGGER_HAS_STD_PRINT 0)
endif()
#---------------------------------------------
if(LOGGER_HAS_STD_FORMAT)
    set(CMAKE_CXX_STANDARD 23)
else()
    set(CMAKE_CXX_STANDARD 20)
endif()
#---------------------------------------------
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
#------------------------------------------------------------------------------------------
message(STATUS "LOGGER_FORCE_FMT: ${LOGGER_FORCE_FMT}")
message(STATUS "LOGGER_USE_STD_FORMAT_STRING: ${LOGGER_USE_STD_FORMAT_STRING}")
message(STATUS "LOGGER_HAS_STD_FORMAT: ${LOGGER_HAS_STD_FORMAT}")
message(STATUS "LOGGER_HAS_STD_PRINT: ${LOGGER_HAS_STD_PRINT}")
message(STATUS "CMAKE_CXX_STANDARD: ${CMAKE_CXX_STANDARD}")
#------------------------------------------------------------------------------------------
# Print out the build type
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
#------------------------------------------------------------------------------------------
if(LOGGER_STANDALONE_PROJECT)
    if(MSVC)
        # MSVC specific warning flags
        add_library(common_options INTERFACE)
        target_compile_options(common_options INTERFACE /W4)  # Equivalent to -Wall in GCC/Clang
    else()
        # GCC/Clang specific warning flags (only add what the compiler supports)
        include(CheckCXXCompilerFlag)
        add_library(common_options INTERFACE)
        set(_LOGGER_COMMON_OPTIONS
            -Wall
            -Wextra
            -Wpedantic
            -pedantic-errors
            -Werror=format-security
            -fasynchronous-unwind-tables
            -Wshadow
            -Werror
            -Wformat
            -Wconversion
            -Wsign-conversion
            -Wcast-qual
            -Wcast-align
            -Wnull-dereference
            -Wdouble-promotion
            -Wimplicit-fallthrough
            -Woverloaded-virtual
            -fstack-protector-strong
            -Werror=return-type
            -Wundef
            -fvisibility=hidden
            -fvisibility-inlines-hidden
            -ffile-prefix-map=${CMAKE_SOURCE_DIR}=.
            -fmacro-prefix-map=${CMAKE_SOURCE_DIR}=.
        )

        foreach(_flag IN LISTS _LOGGER_COMMON_OPTIONS)
            string(MD5 _flag_hash "${_flag}")
            set(_flag_var "LOGGER_HAS_FLAG_${_flag_hash}")
            if(_flag STREQUAL "-Werror=format-security")
                # GCC rejects this flag unless -Wformat is enabled too.
                unset(${_flag_var} CACHE)
                unset(${_flag_var})
                set(_LOGGER_REQUIRED_DEFINITIONS_SAVE "${CMAKE_REQUIRED_DEFINITIONS}")
                set(CMAKE_REQUIRED_DEFINITIONS "-Wformat" "${_flag}")
                check_cxx_source_compiles("int main(){return 0;}" ${_flag_var})
                set(CMAKE_REQUIRED_DEFINITIONS "${_LOGGER_REQUIRED_DEFINITIONS_SAVE}")
                unset(_LOGGER_REQUIRED_DEFINITIONS_SAVE)
            else()
                check_cxx_compiler_flag("${_flag}" ${_flag_var})
            endif()
            if(${_flag_var})
                target_compile_options(common_options INTERFACE "${_flag}")
            endif()
        endforeach()
        unset(_LOGGER_COMMON_OPTIONS)
    endif()
endif()
#------------------------------------------------------------------------------------------
# Set the runtime output directory only if this is a standalone project
if(LOGGER_STANDALONE_PROJECT)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
    # message(STATUS "Library output directory: ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
    # Optionally, to set these directories only for specific build configurations (Debug, Release, etc.),
    # you can use the following approach:
    # set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin)
    # set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)
    # set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/lib)
    # set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/lib)
    # set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/lib)
    # set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/lib)
    # message(STATUS "Library output directory: ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
    # Diagnostic output
    message(STATUS "ARCHIVE_OUTPUT_DIRECTORY: ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}")
    message(STATUS "LIBRARY_OUTPUT_DIRECTORY: ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
    message(STATUS "RUNTIME_OUTPUT_DIRECTORY: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
endif()
#------------------------------------------------------------------------------------------
# Offer switches for the type of the library and optional builds
option(BUILD_LOGGER_SHARED_LIBS "Build using shared libraries" ON)
message(STATUS "BUILD_LOGGER_SHARED_LIBS: ${BUILD_LOGGER_SHARED_LIBS}")
#---------------------------------------
option(BUILD_LOGGER_EXAMPLE "Build Logger example" ${LOGGER_STANDALONE_PROJECT})
option(BUILD_LOGGER_BENCHMARKS "Build Logger benchmarks" ${LOGGER_STANDALONE_PROJECT})
#---------------------------------------
option(LOGGER_DEBUG "Enable Logger debug messages" OFF)
# Automatically enable LOGGER_DEBUG if the build type is Debug
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(LOGGER_DEBUG ON CACHE BOOL "Enable Logger debug messages" FORCE)
endif()
message(STATUS "LOGGER_DEBUG: ${LOGGER_DEBUG}")
#---------------------------------------
option(BUILD_LOGGER_SQL "Build using shared libraries" ON)
message(STATUS "BUILD_LOGGER_SQL: ${BUILD_LOGGER_SQL}")
#------------------------------------------------------------------------------------------
# Force colored output
option(FORCE_COLORED_OUTPUT "Always produce ANSI-colored output (GNU/Clang only)." OFF)
if(${FORCE_COLORED_OUTPUT})
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        add_compile_options(-fdiagnostics-color=always)
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        add_compile_options(-fcolor-diagnostics)
    endif()
endif()
#------------------------------------------------------------------------------------------
# Find SQLite3
find_package(SQLite3 QUIET)
if (SQLite3_FOUND AND BUILD_LOGGER_SQL)
    set(BUILD_LOGGER_SQL ON)
    set(HAS_LOGGER_SQL 1)
    
else()
    set(BUILD_LOGGER_SQL OFF)
    set(HAS_LOGGER_SQL 0)
endif()
# add_compile_definitions(HAS_LOGGER_SQL)
message(STATUS "HAS_LOGGER_SQL: ${HAS_LOGGER_SQL}")
# Add HAS_LOGGER_SQL to compiler definitions
#------------------------------------------------------------------------------------------
# Define the source and include directories
set(LOGGER_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(LOGGER_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
#------------------------------------------------------------------------------------------
# Define the source files
set(LOGGER_SOURCES
    ${LOGGER_SOURCE_DIR}/Logger.cpp
    ${LOGGER_SOURCE_DIR}/LoggerAPI.cpp
    ${LOGGER_SOURCE_DIR}/Log.cpp
    ${LOGGER_SOURCE_DIR}/LogRecord.cpp
    ${LOGGER_SOURCE_DIR}/LogLevel.cpp
    ${LOGGER_SOURCE_DIR}/TimeStamp.cpp
)
#------------------------------------------------------------------------------------------
# Add SQLogger.cpp only if BUILD_LOGGER_SQL is enabled
if(BUILD_LOGGER_SQL)
    list(APPEND LOGGER_SOURCES ${LOGGER_SOURCE_DIR}/SQLogger.cpp)
endif()
#------------------------------------------------------------------------------------------
# Find and link fmt library if std::format is unavailable
if (NOT LOGGER_HAS_STD_FORMAT)
    find_package(fmt REQUIRED)
endif()
#------------------------------------------------------------------------------------------
# Define the Logger library target
set(LOGGER_LIBRARY_NAME "${LOGGER_PROJECT_NAME}_lib")
# Check if the library should be built as a shared library
if(BUILD_LOGGER_SHARED_LIBS)
    add_library(${PROJECT_NAME} SHARED ${LOGGER_SOURCES})
    # If needed, set properties specific to shared libraries here
else()
    add_library(${PROJECT_NAME} STATIC ${LOGGER_SOURCES})
    # If needed, set properties specific to static libraries here
endif()
#------------------------------------------------------------------------------------------
if(BUILD_LOGGER_SHARED_LIBS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE LOGGER_BUILDING_LIBRARY)
else()
    target_compile_definitions(${PROJECT_NAME} PUBLIC LOGGER_STATIC)
endif()

target_compile_definitions(${PROJECT_NAME} PUBLIC
    LOGGER_HAS_STD_FORMAT=${LOGGER_HAS_STD_FORMAT}
    LOGGER_HAS_STD_PRINT=${LOGGER_HAS_STD_PRINT}
    LOGGER_USE_STD_FORMAT_STRING=$<BOOL:${LOGGER_USE_STD_FORMAT_STRING}>
)

target_compile_definitions(${PROJECT_NAME} PRIVATE HAS_LOGGER_SQL=${HAS_LOGGER_SQL})
target_include_directories(${PROJECT_NAME} PUBLIC 
    $<BUILD_INTERFACE:${LOGGER_INCLUDE_DIR}>
    $<INSTALL_INTERFACE:include>
)
if(LOGGER_STANDALONE_PROJECT AND TARGET common_options)
    target_compile_options(${PROJECT_NAME} PRIVATE
        $<TARGET_PROPERTY:common_options,INTERFACE_COMPILE_OPTIONS>
    )
endif()
if(BUILD_LOGGER_SQL)
    target_link_libraries(${PROJECT_NAME} PRIVATE SQLite::SQLite3)
endif()
#------------------------------------------------------------------------------------------
if (NOT LOGGER_HAS_STD_FORMAT)
    target_link_libraries(${PROJECT_NAME} PUBLIC fmt::fmt)
endif()
#------------------------------------------------------------------------------------------
# Alias for Logger library
add_library(Logger::logger ALIAS ${PROJECT_NAME})
#------------------------------------------------------------------------------------------
# Set LOGGER_LIBRARIES variable
if(LOGGER_STANDALONE_PROJECT)
    set(LOGGER_LIBRARIES Logger::logger)
else()
    set(LOGGER_LIBRARIES Logger::logger PARENT_SCOPE)
endif()
message(STATUS "LOGGER_LIBRARIES: ${LOGGER_LIBRARIES}")
#------------------------------------------------------------------------------------------
# Building examples
if(BUILD_LOGGER_EXAMPLE)
    set(LOGGER_EXAMPLE_NAME "${PROJECT_NAME}_example")
    add_executable(${LOGGER_EXAMPLE_NAME} example/main.cpp)
    target_link_libraries(${LOGGER_EXAMPLE_NAME} PRIVATE ${PROJECT_NAME})
    if(LOGGER_STANDALONE_PROJECT)
        target_link_libraries(${LOGGER_EXAMPLE_NAME} PRIVATE common_options)
    endif()
endif()
#------------------------------------------------------------------------------------------
# Benchmarks
if(BUILD_LOGGER_BENCHMARKS)
    if(LOGGER_STANDALONE_PROJECT)
        add_subdirectory(benchmark)
    else()
        message(WARNING "BUILD_LOGGER_BENCHMARKS is only supported when building Logger standalone; skipping.")
    endif()
endif()
#------------------------------------------------------------------------------------------
# Compiler-specific flags
if(MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "/O2 /MD")
    set(CMAKE_CXX_FLAGS_DEBUG "/Od /MDd")
    target_compile_options(${PROJECT_NAME} PRIVATE 
        "$<$<CONFIG:Release>:/O2>" 
        "$<$<CONFIG:Debug>:/Od>" 
        "$<$<CONFIG:Debug>:/Zi>"
    )
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_link_options(${PROJECT_NAME} PRIVATE /DEBUG)
    endif()

    set_target_properties(${PROJECT_NAME} PROPERTIES
        WINDOWS_EXPORT_ALL_SYMBOLS TRUE
    )
else()
    # For non-MSVC compilers
    target_compile_options(${PROJECT_NAME} PRIVATE 
        "$<$<CONFIG:Release>:-O3>"
    )
    # Conditionally include AddressSanitizer flags only if standalone and in Debug mode
    if(LOGGER_STANDALONE_PROJECT AND "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang|GNU")
        set(SANITIZE_FLAGS "$<$<CONFIG:Debug>:-fsanitize=address>")
        target_compile_options(${PROJECT_NAME} PRIVATE 
            "$<$<CONFIG:Debug>:-g>"
            "$<$<CONFIG:Debug>:-O0>"
        )
        
        target_compile_options(${PROJECT_NAME} PUBLIC ${SANITIZE_FLAGS})
        target_link_options(${PROJECT_NAME} PUBLIC ${SANITIZE_FLAGS})
    endif()
endif()
#------------------------------------------------------------------------------------------
if(LOGGER_DEBUG)
    target_compile_definitions(${PROJECT_NAME} PRIVATE LOGGER_DEBUG)
    if(BUILD_LOGGER_EXAMPLE)
        target_compile_definitions(${LOGGER_EXAMPLE_NAME} PRIVATE LOGGER_DEBUG)
    endif()
endif()
#------------------------------------------------------------------------------------------
# Doxygen (only if it's a standalone project)
if(DOXYGEN_FOUND AND LOGGER_STANDALONE_PROJECT)
    set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in)
    set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
    configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
    add_custom_target(docs
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating documentation with Doxygen"
        VERBATIM)
endif()
#------------------------------------------------------------------------------------------
# Export the library targets (only if it's a standalone project)
if(LOGGER_STANDALONE_PROJECT)
    include(GNUInstallDirs)
    install(TARGETS ${PROJECT_NAME} EXPORT LoggerTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    install(DIRECTORY ${LOGGER_INCLUDE_DIR}/${LOGGER_PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

    install(EXPORT LoggerTargets
        FILE LoggerTargets.cmake
        NAMESPACE Logger::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${LOGGER_PROJECT_NAME}
    )
endif()
#------------------------------------------------------------------------------------------
