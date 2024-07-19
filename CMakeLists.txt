cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
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
project(${PARENT_DIR_NAME} VERSION 1.0 DESCRIPTION "A Logger function library" LANGUAGES CXX)
#------------------------------------------------------------------------------------------
# Set the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
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
        # GCC/Clang specific warning flags
        add_library(common_options INTERFACE)
        target_compile_options(common_options INTERFACE -Wall -Wextra -Wpedantic -pedantic-errors -Werror=format-security -fasynchronous-unwind-tables)
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
option(BUILD_LOGGER_EXAMPLE "Build Logger example" ${LOGGER_STANDALONE_PROJECT})
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
# Define the source and include directories
set(LOGGER_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(LOGGER_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
#------------------------------------------------------------------------------------------
# Define the source files
set(LOGGER_SOURCES
    ${LOGGER_SOURCE_DIR}/Logger.cpp
)
#------------------------------------------------------------------------------------------
# Find and link fmt library if not using std::format
if (NOT __cpp_lib_format)
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
target_include_directories(${PROJECT_NAME} PUBLIC 
    $<BUILD_INTERFACE:${LOGGER_INCLUDE_DIR}>
    $<INSTALL_INTERFACE:include>
)
#------------------------------------------------------------------------------------------
if (NOT __cpp_lib_format)
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
            ${SANITIZE_FLAGS}
        )
        target_link_options(${PROJECT_NAME} PRIVATE ${SANITIZE_FLAGS})
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