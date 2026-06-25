if(NOT LOGGER_STANDALONE_PROJECT)
    message(FATAL_ERROR "Tests are only supported when building Logger standalone.")
endif()

# Try finding installed GoogleTest, else fetch it (mirrors the benchmark/ pattern).
find_package(GTest QUIET)

if(NOT GTest_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.15.2
        GIT_SHALLOW    TRUE
    )
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()

find_package(Threads REQUIRED)
include(GoogleTest)

function(create_test_target TARGET_NAME SOURCE_FILE)
    add_executable(${TARGET_NAME} ${SOURCE_FILE})

    target_link_libraries(${TARGET_NAME} PRIVATE Logger::logger Threads::Threads GTest::gtest GTest::gtest_main)
    if(TARGET common_options)
        target_link_libraries(${TARGET_NAME} PRIVATE common_options)
    endif()

    # The concurrency test queries logs.db directly to verify rows.
    if(DEFINED HAS_LOGGER_SQL AND HAS_LOGGER_SQL)
        target_compile_definitions(${TARGET_NAME} PRIVATE HAS_LOGGER_SQL=1)
        if(TARGET SQLite::SQLite3)
            target_link_libraries(${TARGET_NAME} PRIVATE SQLite::SQLite3)
        endif()
    else()
        target_compile_definitions(${TARGET_NAME} PRIVATE HAS_LOGGER_SQL=0)
    endif()

    set_target_properties(${TARGET_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/testbin
    )

    gtest_discover_tests(${TARGET_NAME})
endfunction()

create_test_target(${PROJECT_NAME}_concurrency_test ${CMAKE_CURRENT_LIST_DIR}/LoggerConcurrencyTest.cpp)
