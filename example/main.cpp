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
//--------------------------------------------------------------
void exampleLogging(void) {
    LOG_NORMAL("This is a normal log message without variables.");
    LOG_WARNING("This is a warning log message with a variable: {}", 42);
    LOG_ERROR("This is an error log message with two variables: {} and {}", "variable1", 1234);
    DEBUG_PRINT("This is a debug log message with a variable: {}", 3.14159);
} // end void exampleLogging(void)
//--------------------------------------------------------------
void measureLoggingOverhead(void) {
    const int iterations = 1000;
    auto start = std::chrono::high_resolution_clock::now();
    //--------------------------
    for (int i = 0; i < iterations; ++i) {
        LOG_NORMAL("Logging overhead test: iteration {}", i);
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