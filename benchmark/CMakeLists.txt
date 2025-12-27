if(NOT LOGGER_STANDALONE_PROJECT)
    message(FATAL_ERROR "Benchmarks are only supported when building Logger standalone.")
endif()

# Try finding installed Google Benchmark
find_package(benchmark QUIET)

# Use installed version or FetchContent
if(NOT benchmark_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        googlebenchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG        v1.8.5
        GIT_SHALLOW    TRUE
    )

    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)

    FetchContent_MakeAvailable(googlebenchmark)

    if(TARGET benchmark AND NOT TARGET benchmark::benchmark)
        add_library(benchmark::benchmark ALIAS benchmark)
    endif()
    if(TARGET benchmark_main AND NOT TARGET benchmark::benchmark_main)
        add_library(benchmark::benchmark_main ALIAS benchmark_main)
    endif()
endif()

function(create_benchmark_target TARGET_NAME SOURCE_FILE)
    add_executable(${TARGET_NAME}
        ${SOURCE_FILE}
        ${PROJECT_SOURCE_DIR}/src/Logger.cpp
    )

    target_include_directories(${TARGET_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/include)
    target_compile_definitions(${TARGET_NAME} PRIVATE
        HAS_LOGGER_SQL=0
        LOGGER_HAS_STD_FORMAT=${LOGGER_HAS_STD_FORMAT}
        LOGGER_HAS_STD_PRINT=${LOGGER_HAS_STD_PRINT}
    )

    if(TARGET common_options)
        target_link_libraries(${TARGET_NAME} PRIVATE common_options)
    endif()

    set_target_properties(${TARGET_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/benchmarkbin
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/benchmarklib
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/benchmarklib
    )

    if(TARGET benchmark::benchmark_main)
        target_link_libraries(${TARGET_NAME} PRIVATE benchmark::benchmark_main)
    elseif(TARGET benchmark_main)
        target_link_libraries(${TARGET_NAME} PRIVATE benchmark_main)
    else()
        message(FATAL_ERROR "Google Benchmark target not found (benchmark_main).")
    endif()

    if(NOT LOGGER_HAS_STD_FORMAT)
        target_link_libraries(${TARGET_NAME} PRIVATE fmt::fmt)
    endif()

    enable_testing()
    add_test(NAME ${TARGET_NAME} COMMAND ${TARGET_NAME} --benchmark_min_time=0.01s)
endfunction()

create_benchmark_target(${PROJECT_NAME}_benchmark ${CMAKE_CURRENT_LIST_DIR}/LoggerBenchmark.cpp)
