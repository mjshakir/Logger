//--------------------------------------------------------------
// Concurrency correctness unit test (GoogleTest).
//
// Many threads concurrently emit ERROR/WARNING/INFO carrying a unique
// "CHK|<level>|r=<round>|t=<thread>|n=<i>" token. For each thread count we
// assert that the (lock-free, synchronous) logger never loses, duplicates,
// merges or tears a line on the console, and never loses/duplicates a SQLite
// row. TSan can't run on this aarch64 box, so this exact-count stress is the
// primary concurrency check (pair with an ASan build for memory/UB).
//--------------------------------------------------------------
#include "Logger.hpp"

#include <gtest/gtest.h>

#include <cstdio>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

#include <unistd.h>

#if HAS_LOGGER_SQL == 1
    #include <sqlite3.h>
#endif
//--------------------------------------------------------------
namespace {
    //--------------------------
    constexpr int N = 300; // log calls per thread, per level
    //--------------------------
    struct ConsoleStats {
        long lines = 0;
        long markers = 0;
        long lines_without_marker = 0;
    };
    //--------------------------
    ConsoleStats scan(const std::string& path) {
        std::ifstream file(path);
        std::string   line;
        ConsoleStats  s;
        while (std::getline(file, line)) {
            ++s.lines;
            long here = 0;
            for (std::size_t pos = line.find("CHK|"); pos != std::string::npos; pos = line.find("CHK|", pos + 4)) {
                ++here;
            }
            s.markers += here;
            if (here == 0) {
                ++s.lines_without_marker;
            }
        }
        return s;
    }
    //--------------------------
    // Run the threaded workload with stdout+stderr captured to files at the fd level,
    // then restore the original fds so GoogleTest's own reporting is unaffected.
    void run_workload(int round, int threads) {
        std::fflush(stdout);
        std::fflush(stderr);
        const int saved_out = ::dup(STDOUT_FILENO);
        const int saved_err = ::dup(STDERR_FILENO);
        std::FILE* fo = std::fopen("gtest_chk_out.txt", "w");
        std::FILE* fe = std::fopen("gtest_chk_err.txt", "w");
        ::dup2(::fileno(fo), STDOUT_FILENO);
        ::dup2(::fileno(fe), STDERR_FILENO);
        //--------------------------
        auto& logger = Logger::Logger::instance();
        std::vector<std::thread> workers;
        workers.reserve(static_cast<std::size_t>(threads));
        for (int t = 0; t < threads; ++t) {
            workers.emplace_back([round, t, &logger] {
                for (int i = 0; i < N; ++i) {
                    logger.error("CHK|E|r={}|t={}|n={}", round, t, i);
                    logger.warning("CHK|W|r={}|t={}|n={}", round, t, i);
                    logger.info("CHK|I|r={}|t={}|n={}", round, t, i);
                }
            });
        }
        for (auto& w : workers) {
            w.join();
        }
        //--------------------------
        std::fflush(stdout);
        std::fflush(stderr);
        ::dup2(saved_out, STDOUT_FILENO);
        ::dup2(saved_err, STDERR_FILENO);
        ::close(saved_out);
        ::close(saved_err);
        std::fclose(fo);
        std::fclose(fe);
    }
    //--------------------------
    long db_count(int round, bool distinct) {
#if HAS_LOGGER_SQL == 1
        sqlite3* db = nullptr;
        long value = -1;
        if (sqlite3_open("logs.db", &db) == SQLITE_OK) {
            const std::string like = "CHK|%|r=" + std::to_string(round) + "|%";
            const char* sql = distinct
                ? "SELECT count(DISTINCT message) FROM logs WHERE message LIKE ?;"
                : "SELECT count(*) FROM logs WHERE message LIKE ?;";
            sqlite3_stmt* stmt = nullptr;
            if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
                sqlite3_bind_text(stmt, 1, like.c_str(), -1, SQLITE_TRANSIENT);
                if (sqlite3_step(stmt) == SQLITE_ROW) {
                    value = sqlite3_column_int64(stmt, 0);
                }
                sqlite3_finalize(stmt);
            }
            sqlite3_close(db);
        }
        return value;
#else
        (void)round; (void)distinct;
        return -1;
#endif
    }
    //--------------------------
    // One monotonic round per workload run so the DB filter (LIKE '...|r=R|...') is unique.
    int next_round(void) {
        static int counter = 0;
        return counter++;
    }
    //--------------------------
    class LoggerConcurrency : public ::testing::TestWithParam<int> {
        protected:
            static void SetUpTestSuite(void) {
                // Start from a clean DB before the singleton SQLogger opens it on first log.
                std::remove("logs.db");
                std::remove("logs.db-wal");
                std::remove("logs.db-shm");
            }
    };
    //--------------------------
    TEST_P(LoggerConcurrency, NoTornNoLossNoDuplication) {
        const int threads = GetParam();
        const int round   = next_round();
        //--------------------------
        run_workload(round, threads);
        //--------------------------
        const long expected_console = static_cast<long>(threads) * N * 3;
        const ConsoleStats out = scan("gtest_chk_out.txt");
        const ConsoleStats err = scan("gtest_chk_err.txt");
        const long lines   = out.lines + err.lines;
        const long markers = out.markers + err.markers;
        //--------------------------
        EXPECT_EQ(lines, expected_console)   << "console line count mismatch (lost or merged lines)";
        EXPECT_EQ(markers, expected_console) << "token count mismatch (lost or duplicated messages)";
        EXPECT_EQ(out.lines_without_marker + err.lines_without_marker, 0) << "lines without a token (torn lines)";
        //--------------------------
#if HAS_LOGGER_SQL == 1
        const long expected_db = static_cast<long>(threads) * N * 2; // error + warning
        EXPECT_EQ(db_count(round, false), expected_db) << "SQLite row count (lost or duplicated rows)";
        EXPECT_EQ(db_count(round, true),  expected_db) << "SQLite distinct messages (duplicated rows)";
#endif
    }
    //--------------------------
    INSTANTIATE_TEST_SUITE_P(ThreadCounts, LoggerConcurrency, ::testing::Values(2, 4, 8, 16));
    //--------------------------
} // namespace
//--------------------------------------------------------------
