//--------------------------------------------------------------
// Single-threaded per-operation latency (the "hot path" view).
//
// Unlike LoggerBenchmark.cpp (which calls debug() -- a no-op sink when
// LOGGER_DEBUG is off), this hits the REAL sinks: INFO -> stdout,
// ERROR/WARNING -> console + SQLite. Run with output -> /dev/null and cwd on a
// tmpfs; report the median ns/op (--benchmark_repetitions=15
// --benchmark_report_aggregates_only=true --benchmark_min_warmup_time=0.05).
//--------------------------------------------------------------
#include <benchmark/benchmark.h>

#include <numeric>
#include <string>
#include <vector>

#include "Logger.hpp"
//--------------------------------------------------------------
static void BM_Info(benchmark::State& state) {
    auto& logger = Logger::Logger::instance();
    long i = 0;
    for (auto _ : state) {
        logger.info("lat info {}", i++);
        benchmark::DoNotOptimize(i);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Info);
//--------------------------------------------------------------
static void BM_Warning(benchmark::State& state) {
    auto& logger = Logger::Logger::instance();
    long i = 0;
    for (auto _ : state) {
        logger.warning("lat warn {}", i++);
        benchmark::DoNotOptimize(i);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Warning);
//--------------------------------------------------------------
static void BM_Error(benchmark::State& state) {
    auto& logger = Logger::Logger::instance();
    long i = 0;
    for (auto _ : state) {
        logger.error("lat err {}", i++);
        benchmark::DoNotOptimize(i);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Error);
//--------------------------------------------------------------
static void BM_Info_Function(benchmark::State& state) {
    auto& logger = Logger::Logger::instance();
    long i = 0;
    for (auto _ : state) {
        logger.info_function(__FILE__, "bench::fn", __LINE__, "lat info {}", i++);
        benchmark::DoNotOptimize(i);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Info_Function);
//--------------------------------------------------------------
// Raw formatting cost vs message size (INFO -> stdout, no DB).
//--------------------------------------------------------------
static void BM_Info_Payload(benchmark::State& state) {
    auto& logger = Logger::Logger::instance();
    std::string payload(static_cast<std::size_t>(state.range(0)), 'x');
    for (auto _ : state) {
        logger.info("{}", payload);
        benchmark::DoNotOptimize(payload.data());
    }
    state.SetComplexityN(state.range(0));
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Info_Payload)->RangeMultiplier(8)->Range(8, 8 << 13)->Complexity();
//--------------------------------------------------------------
// Container/stream path scaling (the extra print_container pass).
//--------------------------------------------------------------
static void BM_Info_Stream_VecN(benchmark::State& state) {
    auto& logger = Logger::Logger::instance();
    std::vector<int> values(static_cast<std::size_t>(state.range(0)));
    std::iota(values.begin(), values.end(), 0);
    for (auto _ : state) {
        logger.info_stream("vals:", values);
        benchmark::DoNotOptimize(values.data());
    }
    state.SetComplexityN(state.range(0));
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Info_Stream_VecN)->RangeMultiplier(8)->Range(8, 4096)->Complexity();
//--------------------------------------------------------------
