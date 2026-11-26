//--------------------------------------------------------------
// A run's bookkeeping (GoogleTest): the run's row and its state row, the end written once, the
// cursor over the unfinished runs, a folder and a database name of any kind, and two programs that
// start on a new database file together. The library's own schema is applied by the library; the
// cases read the database through the session's own path.
//--------------------------------------------------------------
#include "Log.hpp"
#include "LogRun.hpp"
#include "LogSession.hpp"
#include "UnfinishedRuns.hpp"

#include <gtest/gtest.h>

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <type_traits>
#include <vector>

#if HAS_LOGGER_SQL == 1
    #include <sqlite3.h>
#endif
#if !defined(_WIN32)
    #include <sys/wait.h>
    #include <unistd.h>
#endif
//--------------------------------------------------------------
namespace {
    //--------------------------
#if HAS_LOGGER_SQL == 1
    // How long the cases' own connections wait for a writer: longer than any case holds a lock.
    constexpr int C_TEST_BUSY_MILLISECONDS = 3000;
#endif
    //--------------------------
    // A fresh folder name per case, under the working folder; removed by the case's end.
    class Scratch {
        public:
            explicit Scratch(const std::string& name) :
                m_folder((std::filesystem::current_path() / ("logsession-" + name)).string()) {
                std::error_code _error;
                std::filesystem::remove_all(m_folder, _error);
            }
            Scratch(const Scratch&)            = delete;
            Scratch& operator=(const Scratch&) = delete;
            Scratch(Scratch&&)                 = delete;
            Scratch& operator=(Scratch&&)      = delete;
            ~Scratch(void) {
                std::error_code _error;
                std::filesystem::remove_all(m_folder, _error);
            }
            const std::string& folder(void) const noexcept {
                return m_folder;
            }
        private:
            std::string m_folder;
    };// end class Scratch
    //--------------------------
#if HAS_LOGGER_SQL == 1
    // The first column of the first row of a question, as text; "?" for any refusal.
    std::string asked(const std::string& database, const char* const sql) {
        sqlite3* db = nullptr;
        std::string answer = "?";
        if (sqlite3_open_v2(database.c_str(), &db, SQLITE_OPEN_READONLY, nullptr) == SQLITE_OK) {
            sqlite3_busy_timeout(db, C_TEST_BUSY_MILLISECONDS);
            sqlite3_stmt* stmt = nullptr;
            if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
                if (sqlite3_step(stmt) == SQLITE_ROW) {
                    const unsigned char* text = sqlite3_column_text(stmt, 0);
                    answer = text == nullptr ? std::string{} : std::string(reinterpret_cast<const char*>(text));
                }
                sqlite3_finalize(stmt);
            }
        }
        sqlite3_close(db);
        return answer;
    }
    //--------------------------
    // One statement through a connection of the case's own that may write; false for any refusal.
    bool executed(const std::string& database, const char* const sql) {
        sqlite3* db = nullptr;
        bool done = false;
        if (sqlite3_open_v2(database.c_str(), &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr) == SQLITE_OK) {
            sqlite3_busy_timeout(db, C_TEST_BUSY_MILLISECONDS);
            done = sqlite3_exec(db, sql, nullptr, nullptr, nullptr) == SQLITE_OK;
        }
        sqlite3_close(db);
        return done;
    }
    //--------------------------
    // The folder's run, made and left open: no end is written, as when a program dies.
    void left_open(const std::string& folder, const std::string& name) {
        const Logger::LogRun run(Logger::LogPlace(folder, "runs.db"), Logger::RunIdentity(name, "prog", 1));
        EXPECT_TRUE(run.id().has_value());
    }
#endif
    //--------------------------
} // namespace
//--------------------------------------------------------------
// The structs keep views: a literal, a view and a named string build them, a temporary string
// does not (its constructor is deleted, because the view would dangle).
TEST(LogSessionRun, TheStructsTakeNamedTextsAndRefuseATemporaryString) {
    const std::string named = "text";
    const std::string_view view = named;
    const Logger::LogPlace   from_literal("folder", "file.db");
    const Logger::LogPlace   from_named(named, view);
    const Logger::RunIdentity who_literal("name", "program", 1);
    const Logger::RunIdentity who_named(named, view, 2);
    const Logger::Unfinished  row(3, named, who_named);
    EXPECT_EQ(from_literal.database, "file.db");
    EXPECT_EQ(*from_named.directory, "text");
    EXPECT_EQ(who_literal.program, "program");
    EXPECT_EQ(who_named.process, 2);
    EXPECT_EQ(row.identity.name, "text");
    EXPECT_FALSE((std::is_constructible_v<Logger::LogPlace, std::string, std::string_view>));
    EXPECT_FALSE((std::is_constructible_v<Logger::LogPlace, std::string_view, std::string>));
    EXPECT_FALSE((std::is_constructible_v<Logger::RunIdentity, std::string, std::string_view, int>));
    EXPECT_FALSE((std::is_constructible_v<Logger::RunIdentity, std::string_view, std::string, int>));
    EXPECT_FALSE((std::is_constructible_v<Logger::Unfinished, int, std::string, Logger::RunIdentity>));
    EXPECT_TRUE((std::is_constructible_v<Logger::RunIdentity, std::string&, const char (&)[5], int>));
    EXPECT_FALSE(std::is_default_constructible_v<Logger::LogPlace>);
    EXPECT_FALSE(std::is_default_constructible_v<Logger::RunIdentity>);
    EXPECT_FALSE(std::is_default_constructible_v<Logger::Unfinished>);
}
//--------------------------------------------------------------
#if HAS_LOGGER_SQL == 1
TEST(LogSessionRun, TheRunRowAndItsStateRowAreBothThere) {
    const Scratch _scratch("both");
    const Logger::LogRun run(Logger::LogPlace(_scratch.folder(), "runs.db"), Logger::RunIdentity("one", "prog", 4242));
    ASSERT_TRUE(run.session().live());
    ASSERT_TRUE(run.id().has_value());
    const std::string db{*run.session().database()};
    EXPECT_EQ(asked(db, "SELECT count(*) FROM run"), "1");
    EXPECT_EQ(asked(db, "SELECT pid FROM run_state"), "4242");
    EXPECT_EQ(asked(db, "SELECT name || '/' || program FROM run"), "one/prog");
}
//--------------------------------------------------------------
// A table named run that cannot take the run's row: the session stays live, the cause is named
// and no half of the run stays.
TEST(LogSessionRun, ARefusedRowIsNamedAndLeavesTheSessionLive) {
    const Scratch _scratch("refused");
    std::error_code _error;
    std::filesystem::create_directories(_scratch.folder(), _error);
    const std::string db = _scratch.folder() + "/runs.db";
    ASSERT_TRUE(executed(db, "CREATE TABLE run (id INTEGER PRIMARY KEY, other TEXT);"));
    const Logger::LogRun run(Logger::LogPlace(_scratch.folder(), "runs.db"), Logger::RunIdentity("one", "prog", 1));
    ASSERT_TRUE(run.session().live()) << "a refused row leaves the session live";
    ASSERT_FALSE(run.id().has_value());
    EXPECT_EQ(run.id().error(), Logger::LogError::REFUSED);
    EXPECT_EQ(asked(db, "SELECT count(*) FROM run_state"), "0");
}
//--------------------------------------------------------------
TEST(LogSessionRun, EndedIsWrittenOnceAndThenAlready) {
    const Scratch _scratch("ended");
    const Logger::LogRun run(Logger::LogPlace(_scratch.folder(), "runs.db"), Logger::RunIdentity("one", "prog", 1));
    ASSERT_TRUE(run.id().has_value());
    const auto first  = run.ended("ok");
    const auto second = run.ended("failed");
    ASSERT_TRUE(first.has_value());
    ASSERT_TRUE(second.has_value());
    EXPECT_EQ(*first, Logger::Ended::WRITTEN);
    EXPECT_EQ(*second, Logger::Ended::ALREADY);
    EXPECT_EQ(asked(std::string{*run.session().database()}, "SELECT outcome FROM run_state"), "ok");
}
//--------------------------------------------------------------
TEST(LogSessionRun, EndedOfAnotherRunAndOfNoRun) {
    const Scratch _scratch("other");
    left_open(_scratch.folder(), "old");
    const Logger::LogRun run(Logger::LogPlace(_scratch.folder(), "runs.db"), Logger::RunIdentity("new", "prog", 2));
    ASSERT_TRUE(run.id().has_value());
    std::size_t count = 0U;
    for (Logger::UnfinishedRuns old(run, 4uz); const std::optional<Logger::Unfinished> row = old.next();) {
        ++count;
        EXPECT_EQ(std::string(row->identity.name), "old");
        EXPECT_EQ(row->identity.process, 1);
        EXPECT_EQ(*run.ended("lost", row->run), Logger::Ended::WRITTEN);
    }
    EXPECT_EQ(count, 1U);
    Logger::UnfinishedRuns again(run, 4uz);
    EXPECT_FALSE(again.next().has_value());
    EXPECT_FALSE(again.error().has_value());
    EXPECT_EQ(*run.ended("lost", 99999), Logger::Ended::ALREADY) << "no such run: nothing to write";
}
//--------------------------------------------------------------
TEST(LogSessionRun, EndedUnderLoggingThreadsIsWrittenExactlyOnce) {
    const Scratch _scratch("threads");
    const Logger::LogRun run(Logger::LogPlace(_scratch.folder(), "runs.db"), Logger::RunIdentity("one", "prog", 1));
    ASSERT_TRUE(run.id().has_value());
    std::atomic<int>   written{0};
    std::atomic<bool>  go{false};
    std::vector<std::thread> threads;
    for (int t = 0; t < 4; ++t) {
        threads.emplace_back([&] {
            for (int i = 0; i < 100; ++i) {
                LOG_WARNING_FUNCTION("noise {}", i);
            }
        });
        threads.emplace_back([&] {
            while (!go.load()) {
                std::this_thread::yield();
            }
            const auto answer = run.ended("ok");
            if (answer.has_value() and *answer == Logger::Ended::WRITTEN) {
                written.fetch_add(1);
            }
        });
    }
    go.store(true);
    for (std::thread& thread : threads) {
        thread.join();
    }
    EXPECT_EQ(written.load(), 1) << "only the call whose UPDATE changed the row answers WRITTEN";
}
//--------------------------------------------------------------
TEST(LogSessionRun, AnyFolderAndAnyDatabaseNameWork) {
    const Scratch _scratch("any");
    const std::string folder = _scratch.folder() + "/a/b c/d";
    const Logger::LogRun run(Logger::LogPlace(folder, "x y.sqlite3"), Logger::RunIdentity("one", "prog", 1));
    ASSERT_TRUE(run.session().live());
    EXPECT_EQ(run.session().folder().value(), folder);
    EXPECT_EQ(run.session().database().value(), folder + "/x y.sqlite3");
    EXPECT_TRUE(std::filesystem::is_regular_file(std::string{*run.session().database()}));
    EXPECT_TRUE(run.id().has_value());
}
//--------------------------------------------------------------
// A program with no run name (one that stopped before it read its arguments) makes a run with an empty name:
// the row is made with the empty text, and not refused as a NULL in a NOT NULL column.
TEST(LogSessionRun, ARunWithNoNameIsARowWithTheEmptyName) {
    const Scratch _scratch("empty-name");
    const Logger::LogRun run(Logger::LogPlace(_scratch.folder(), "runs.db"), Logger::RunIdentity(std::string_view{}, "prog", 1));
    ASSERT_TRUE(run.session().live());
    ASSERT_TRUE(run.id().has_value());
    const std::string db{*run.session().database()};
    EXPECT_EQ(asked(db, "SELECT count(*) FROM run WHERE name = ''"), "1");
    EXPECT_EQ(asked(db, "SELECT count(*) FROM run_state"), "1");
}
//--------------------------------------------------------------
TEST(LogSessionRun, ASecondRefusedSessionMakesNoRow) {
    const Scratch _scratch("second");
    const Logger::LogRun first(Logger::LogPlace(_scratch.folder(), "runs.db"), Logger::RunIdentity("one", "prog", 1));
    const Logger::LogRun second(Logger::LogPlace(_scratch.folder(), "runs.db"), Logger::RunIdentity("two", "prog", 2));
    ASSERT_TRUE(first.session().live());
    EXPECT_FALSE(second.session().live());
    EXPECT_EQ(second.id().error(), Logger::LogError::NO_SESSION);
    EXPECT_EQ(asked(std::string{*first.session().database()}, "SELECT count(*) FROM run"), "1");
}
//--------------------------------------------------------------
TEST(LogSessionRun, ASessionAloneStoresMessagesWithNoRun) {
    const Scratch _scratch("none");
    const Logger::LogSession session(Logger::LogPlace(_scratch.folder(), "runs.db"));
    ASSERT_TRUE(session.live());
    LOG_WARNING_FUNCTION("alone {}", 1);
    const std::string db{*session.database()};
    EXPECT_EQ(asked(db, "SELECT count(*) FROM logs WHERE message = 'alone 1' AND run_id IS NULL"), "1");
    EXPECT_EQ(asked(db, "SELECT count(*) FROM run"), "0") << "the library's tables are there, with no run";
}
//--------------------------------------------------------------
TEST(LogSessionRun, AnEmptyDatabaseNameIsRefused) {
    const Scratch _scratch("noname");
    const Logger::LogSession session(Logger::LogPlace(_scratch.folder(), ""));
    EXPECT_FALSE(session.live());
    EXPECT_FALSE(session.database().has_value());
}
//--------------------------------------------------------------
// The text of an older run is kept whole, whatever its length.
TEST(LogSessionRun, ALongTextArrivesWhole) {
    const Scratch _scratch("long");
    const std::string name(300U, 'n');
    left_open(_scratch.folder(), name);
    const Logger::LogRun run(Logger::LogPlace(_scratch.folder(), "runs.db"), Logger::RunIdentity("new", "prog", 2));
    Logger::UnfinishedRuns old(run, 4uz);
    const std::optional<Logger::Unfinished> row = old.next();
    ASSERT_TRUE(row.has_value());
    EXPECT_EQ(row->identity.name.size(), 300U);
    EXPECT_EQ(std::string(row->identity.name), name);
}
//--------------------------------------------------------------
TEST(LogSessionRun, TheCursorIsBoundedByMost) {
    const Scratch _scratch("bounded");
    for (int n = 0; n < 5; ++n) {
        left_open(_scratch.folder(), "old" + std::to_string(n));
    }
    const Logger::LogRun run(Logger::LogPlace(_scratch.folder(), "runs.db"), Logger::RunIdentity("new", "prog", 2));
    Logger::UnfinishedRuns old(run, 3uz);
    std::size_t count = 0U;
    while (old.next().has_value()) {
        ++count;
    }
    EXPECT_EQ(count, 3U);
    EXPECT_FALSE(old.next().has_value());
    EXPECT_FALSE(old.error().has_value()) << "a list that ended by its bound is not a refusal";
}
//--------------------------------------------------------------
// ended() on the connection the cursor reads from, while the cursor stands on a row.
TEST(LogSessionRun, TheCursorMayEndTheRowItIsOn) {
    const Scratch _scratch("inloop");
    for (int n = 0; n < 3; ++n) {
        left_open(_scratch.folder(), "old" + std::to_string(n));
    }
    const Logger::LogRun run(Logger::LogPlace(_scratch.folder(), "runs.db"), Logger::RunIdentity("new", "prog", 2));
    std::size_t count = 0U;
    for (Logger::UnfinishedRuns old(run, 16uz); const std::optional<Logger::Unfinished> row = old.next();) {
        const std::string name(row->identity.name);
        const std::expected<Logger::Ended, Logger::LogError> ended = run.ended("lost", row->run);
        ASSERT_TRUE(ended.has_value()) << "ended() inside the loop: " << Logger::name(ended.error());
        EXPECT_EQ(*ended, Logger::Ended::WRITTEN);
        EXPECT_EQ(name, "old" + std::to_string(count)) << "the row's text is whole after ended()";
        ++count;
    }
    EXPECT_EQ(count, 3U);
    Logger::UnfinishedRuns again(run, 16uz);
    EXPECT_FALSE(again.next().has_value());
    EXPECT_EQ(asked(std::string{*run.session().database()}, "SELECT count(*) FROM unfinished"), "1") << "only the new run is open";
}
//--------------------------------------------------------------
// A cursor dropped on its first row holds no read open: the database can be reset after it.
TEST(LogSessionRun, ACursorLeftEarlyLeavesNothingOpen) {
    const Scratch _scratch("early");
    for (int n = 0; n < 3; ++n) {
        left_open(_scratch.folder(), "old" + std::to_string(n));
    }
    const Logger::LogRun run(Logger::LogPlace(_scratch.folder(), "runs.db"), Logger::RunIdentity("new", "prog", 2));
    {
        Logger::UnfinishedRuns old(run, 16uz);
        ASSERT_TRUE(old.next().has_value());
    }
    sqlite3* db = nullptr;
    ASSERT_EQ(sqlite3_open_v2(std::string{*run.session().database()}.c_str(), &db, SQLITE_OPEN_READWRITE, nullptr), SQLITE_OK);
    sqlite3_busy_timeout(db, C_TEST_BUSY_MILLISECONDS);
    int busy = -1;
    char* error = nullptr;
    EXPECT_EQ(sqlite3_exec(db, "PRAGMA wal_checkpoint(TRUNCATE);",
                           [](void* p_busy, int, char** values, char**) -> int {
                               *static_cast<int*>(p_busy) = std::stoi(values[0]);
                               return 0;
                           }, &busy, &error), SQLITE_OK);
    sqlite3_free(error);
    sqlite3_close(db);
    EXPECT_EQ(busy, 0) << "a reader was still open";
}
//--------------------------------------------------------------
TEST(LogSessionRun, AMessageAfterTheRunCarriesItsId) {
    const Scratch _scratch("carries");
    const Logger::LogRun run(Logger::LogPlace(_scratch.folder(), "runs.db"), Logger::RunIdentity("one", "prog", 1));
    ASSERT_TRUE(run.id().has_value());
    LOG_WARNING_FUNCTION("after {}", 7);
    EXPECT_EQ(asked(std::string{*run.session().database()}, "SELECT run_id FROM logs WHERE message = 'after 7'"),
              std::to_string(*run.id()));
}
//--------------------------------------------------------------
// Programs that start on a new database file in the same instant each get their own run row.
TEST(LogSessionRun, TwoProgramsStartingTogetherEachGetARow) {
#if defined(_WIN32)
    GTEST_SKIP() << "the case forks processes";
#else
    constexpr int C_ROUNDS = 12;
    for (int round = 0; round < C_ROUNDS; ++round) {
        const Scratch _scratch("two-" + std::to_string(round));
        std::array<int, 2> pipe_ends{};
        ASSERT_EQ(::pipe(pipe_ends.data()), 0);
        std::array<pid_t, 2> children{};
        for (std::size_t index = 0U; index < children.size(); ++index) {
            const pid_t pid = ::fork();
            ASSERT_GE(pid, 0);
            if (pid == 0) {
                ::close(pipe_ends[1]);
                char byte = 0;
                // The read ends when the parent closes its end: that is the signal to go.
                const ssize_t got = ::read(pipe_ends[0], &byte, 1U);
                static_cast<void>(got);
                bool taken = false;
                {
                    const std::string name = "program " + std::to_string(index);
                    const Logger::LogRun run(Logger::LogPlace(_scratch.folder(), "shared.db"), Logger::RunIdentity(name, "prog", static_cast<std::int64_t>(::getpid())));
                    taken = run.id().has_value();
                }
                ::_exit(taken ? 0 : 1);
            }
            children[index] = pid;
        }
        ::close(pipe_ends[0]);
        ::close(pipe_ends[1]);
        for (const pid_t pid : children) {
            int status = 0;
            ASSERT_EQ(::waitpid(pid, &status, 0), pid);
            EXPECT_TRUE(WIFEXITED(status) and WEXITSTATUS(status) == 0) << "a program lost its run (round " << round << ")";
        }
        const std::string db = _scratch.folder() + "/shared.db";
        EXPECT_EQ(asked(db, "SELECT count(*) FROM run"), "2") << "round " << round;
        EXPECT_EQ(asked(db, "SELECT count(*) FROM run_state"), "2") << "round " << round;
    }
#endif
}
#else
TEST(LogSessionRun, ABuildWithoutSqlHasNoBookkeepingAndSaysSo) {
    const Scratch _scratch("nosql");
    const Logger::LogRun run(Logger::LogPlace(_scratch.folder(), "runs.db"), Logger::RunIdentity("one", "prog", 1));
    ASSERT_TRUE(run.session().live());
    EXPECT_EQ(run.id().error(), Logger::LogError::NO_SQL);
    EXPECT_EQ(run.ended("ok").error(), Logger::LogError::NO_SQL);
    EXPECT_FALSE(run.session().database().has_value());
}
#endif
