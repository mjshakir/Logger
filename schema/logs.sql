-- The one log database: the library applies this file when a session opens, and the Python tools
-- read the same file. Every statement is safe to apply twice.
-- Two writers keep a run's rows (the Logger library, tools/shared/log_database.py); change both together.
--   began: INSERT INTO run (name, program); INSERT INTO run_state (run_id, pid)
--   ended: UPDATE run_state SET ended, outcome WHERE run_id = ? AND ended IS NULL
-- logs: the Logger's table plus run_id; run: one row per process; run_state: one per run begun.
-- unfinished: runs not yet ended; log_lines: what a person reads, ordered by id.

CREATE TABLE IF NOT EXISTS run (
    id      INTEGER PRIMARY KEY AUTOINCREMENT,
    name    TEXT NOT NULL,                                          -- the run's folder name, or '' for a program with none
    program TEXT NOT NULL,                                          -- the name of the program that made the run
    started TEXT NOT NULL DEFAULT (datetime('now', 'localtime'))   -- as the Logger stamps a line: local time, to the second
);

CREATE TABLE IF NOT EXISTS logs (
    id            INTEGER PRIMARY KEY AUTOINCREMENT,
    timestamp     TEXT NOT NULL,
    log_level     TEXT NOT NULL,
    function_name TEXT,
    file_name     TEXT,
    line_number   INTEGER,
    message       TEXT NOT NULL,
    run_id        INTEGER REFERENCES run (id)
);

CREATE INDEX IF NOT EXISTS logs_by_run ON logs (run_id, id);

CREATE VIEW IF NOT EXISTS log_lines AS
    SELECT l.id, l.timestamp, l.log_level AS level, r.program, r.name AS run,
           l.file_name AS file, l.function_name AS function, l.line_number AS line, l.message
    FROM logs AS l LEFT JOIN run AS r ON r.id = l.run_id;

CREATE TABLE IF NOT EXISTS run_state (
    run_id  INTEGER PRIMARY KEY REFERENCES run (id),
    pid     INTEGER NOT NULL,                                       -- the process, so a live run is not called lost
    ended   TEXT,                                                   -- NULL while it runs; local time as started is written
    outcome TEXT                                                    -- the caller's own word for how the run ended, set once by ended()
);

CREATE VIEW IF NOT EXISTS unfinished AS
    SELECT r.id, r.name, r.program, r.started, s.pid
    FROM run AS r JOIN run_state AS s ON s.run_id = r.id
    WHERE s.ended IS NULL;
