#include <benchmark/benchmark.h>

#include "Logger.hpp"

#include <cstddef>
#include <map>
#include <numeric>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

static void BM_Logger_Debug_NoArgs(benchmark::State& state) {
    auto& logger = Logger::Logger::instance();

    for (auto _ : state) {
        logger.debug("hello world");
    }
}

BENCHMARK(BM_Logger_Debug_NoArgs);

static void BM_Logger_Debug_Int(benchmark::State& state) {
    auto& logger = Logger::Logger::instance();
    int value = 0;

    for (auto _ : state) {
        logger.debug("{}", value++);
        benchmark::DoNotOptimize(value);
    }
}

BENCHMARK(BM_Logger_Debug_Int);

static void BM_Logger_Debug_ThreeInts(benchmark::State& state) {
    auto& logger = Logger::Logger::instance();
    int a = 1;
    int b = 2;
    int c = 3;

    for (auto _ : state) {
        logger.debug("a={} b={} c={}", a, b, c);
        ++a;
        ++b;
        ++c;
        benchmark::DoNotOptimize(a);
    }
}

BENCHMARK(BM_Logger_Debug_ThreeInts);

static void BM_Logger_Debug_FormatBytes(benchmark::State& state) {
    auto& logger = Logger::Logger::instance();
    std::string payload(static_cast<std::size_t>(state.range(0)), 'x');

    for (auto _ : state) {
        logger.debug("{}", payload);
        benchmark::DoNotOptimize(payload);
    }

    state.SetComplexityN(state.range(0));
}

BENCHMARK(BM_Logger_Debug_FormatBytes)
    ->RangeMultiplier(2)
    ->Range(8, 8 << 15)
    ->Complexity();

static void BM_Logger_DebugFunction_FormatBytes(benchmark::State& state) {
    auto& logger = Logger::Logger::instance();
    std::string payload(static_cast<std::size_t>(state.range(0)), 'x');

    for (auto _ : state) {
        logger.debug_function("LoggerBenchmark::DebugFunction", "{}", payload);
        benchmark::DoNotOptimize(payload);
    }

    state.SetComplexityN(state.range(0));
}

BENCHMARK(BM_Logger_DebugFunction_FormatBytes)
    ->RangeMultiplier(2)
    ->Range(8, 8 << 15)
    ->Complexity();

static void BM_Logger_DebugStream_VectorN(benchmark::State& state) {
    auto& logger = Logger::Logger::instance();
    std::vector<int> values(static_cast<std::size_t>(state.range(0)));
    std::iota(values.begin(), values.end(), 0);

    for (auto _ : state) {
        logger.debug_stream("values:", values);
        benchmark::DoNotOptimize(values.data());
    }

    state.SetComplexityN(state.range(0));
}

BENCHMARK(BM_Logger_DebugStream_VectorN)
    ->RangeMultiplier(2)
    ->Range(1, 1 << 14)
    ->Complexity();

static void BM_Logger_DebugStream_VectorStringN(benchmark::State& state) {
    auto& logger = Logger::Logger::instance();
    std::vector<std::string> values(static_cast<std::size_t>(state.range(0)));
    for (std::size_t i = 0; i < values.size(); ++i) {
        values[i] = "value_" + std::to_string(i);
    }

    for (auto _ : state) {
        logger.debug_stream("values:", values);
        benchmark::DoNotOptimize(values.data());
    }

    state.SetComplexityN(state.range(0));
}

BENCHMARK(BM_Logger_DebugStream_VectorStringN)
    ->RangeMultiplier(2)
    ->Range(1, 1 << 13)
    ->Complexity();

static void BM_Logger_DebugStream_MapN(benchmark::State& state) {
    auto& logger = Logger::Logger::instance();
    std::map<int, int> values;
    for (int i = 0; i < state.range(0); ++i) {
        values.emplace(i, i);
    }

    for (auto _ : state) {
        logger.debug_stream("values:", values);
        benchmark::DoNotOptimize(values.size());
    }

    state.SetComplexityN(state.range(0));
}

BENCHMARK(BM_Logger_DebugStream_MapN)
    ->RangeMultiplier(2)
    ->Range(1, 1 << 12)
    ->Complexity();

static void BM_Logger_DebugStream_UnorderedMapN(benchmark::State& state) {
    auto& logger = Logger::Logger::instance();
    std::unordered_map<int, int> values;
    values.reserve(static_cast<std::size_t>(state.range(0)));
    for (int i = 0; i < state.range(0); ++i) {
        values.emplace(i, i);
    }

    for (auto _ : state) {
        logger.debug_stream("values:", values);
        benchmark::DoNotOptimize(values.size());
    }

    state.SetComplexityN(state.range(0));
}

BENCHMARK(BM_Logger_DebugStream_UnorderedMapN)
    ->RangeMultiplier(2)
    ->Range(1, 1 << 12)
    ->Complexity();

static void BM_Logger_DebugStream_SetN(benchmark::State& state) {
    auto& logger = Logger::Logger::instance();
    std::set<int> values;
    for (int i = 0; i < state.range(0); ++i) {
        values.emplace(i);
    }

    for (auto _ : state) {
        logger.debug_stream("values:", values);
        benchmark::DoNotOptimize(values.size());
    }

    state.SetComplexityN(state.range(0));
}

BENCHMARK(BM_Logger_DebugStream_SetN)
    ->RangeMultiplier(2)
    ->Range(1, 1 << 12)
    ->Complexity();
