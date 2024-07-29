//--------------------------------------------------------------
// Main Header 
//--------------------------------------------------------------
#include "Logger.hpp"
//--------------------------------------------------------------
// Standard cpp library
//--------------------------------------------------------------
#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <map>
#include <unordered_map>
#include <set>
#include <list>
#include <deque>
#include <array>
//--------------------------------------------------------------
enum class Colors { Red, Green, Blue };
//--------------------------------------------------------------
void exampleLogging(void) {
    LOG_INFO("This is a normal log message without variables.");
    LOG_WARNING("This is a warning log message with a variable: {}", 42);
    LOG_ERROR("This is an error log message with two variables: {} and {}", "variable1", 1234);
    LOG_DEBUG("This is a debug log message with a variable: {}", 3.14159);

    // Log containers
    std::vector<int> vec = {1, 2, 3, 4, 5};
    LOG_INFO_STREAM("Logging vector:", vec);
    LOG_WARNING_STREAM("Logging vector warning:", vec);

    std::map<std::string, int> map = {{"one", 1}, {"two", 2}, {"three", 3}};
    LOG_INFO_STREAM("Logging map:", map);
    LOG_ERROR_STREAM("Logging map error:", map);

    std::unordered_map<int, std::string> unordered_map = {{1, "one"}, {2, "two"}, {3, "three"}};
    LOG_INFO_STREAM("Logging unordered_map:", unordered_map);

    std::set<int> set = {10, 20, 30};
    LOG_INFO_STREAM("Logging set:", set);

    // Log enum
    Colors color = Colors::Red;
    LOG_INFO_STREAM("Logging enum:", color);
} // end void exampleLogging(void)
//--------------------------------------------------------------
void measureLoggingOverhead(void) {
    const int iterations = 1000;
    auto start = std::chrono::high_resolution_clock::now();
    //--------------------------
    for (int i = 0; i < iterations; ++i) {
        LOG_INFO("Logging overhead test: iteration {}", i);
    } // end for(int i = 0; i < iterations; ++i)
    //--------------------------
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::micro> duration = end - start;
    std::cout << "Total time taken for " << iterations << " logging operations: " 
              << duration.count() << " microseconds\n";
    std::cout << "Average time per logging operation: " 
              << duration.count() / iterations << " microseconds\n";
    //--------------------------
} // end void measureLoggingOverhead(void)
//--------------------------------------------------------------
void measureLoggingOverheadWarning(void) {
    const int iterations = 1000;
    auto start = std::chrono::high_resolution_clock::now();
    //--------------------------
    for (int i = 0; i < iterations; ++i) {
        LOG_WARNING("Logging overhead test: iteration {}", i);
    } // end for(int i = 0; i < iterations; ++i)
    //--------------------------
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::micro> duration = end - start;
    std::cout << "Total time taken for " << iterations << " logging operations: " 
              << duration.count() << " microseconds\n";
    std::cout << "Average time per logging operation: " 
              << duration.count() / iterations << " microseconds\n";
    //--------------------------
} // end void measureLoggingOverhead(void)
//--------------------------------------------------------------
int main(void) {
    //--------------------------
    // Example logging usage
    //--------------------------
    exampleLogging();
    //--------------------------
    // Measure the logging overhead
    //--------------------------
    measureLoggingOverhead();
    //--------------------------
    measureLoggingOverheadWarning();
    //--------------------------
    return 0;
    //--------------------------
} // end int main(void)
//--------------------------------------------------------------