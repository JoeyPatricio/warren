#include <cstdint>
#include <iostream>
#include <memory>
#include <optional>
#include <sqlite3.h>
#include <string>
#include <vector>

int create_jobs_table(sqlite3 *db) {
  // AUTOINCREMENT keeps the ID of a deleted job from being reused.
  const char *sql = "CREATE TABLE IF NOT EXISTS jobs ("
                    "ID INTEGER PRIMARY KEY AUTOINCREMENT, "
                    "Job BLOB NOT NULL, "
                    "State TEXT NOT NULL DEFAULT 'queued' CHECK (State IN "
                    "('queued', 'running', 'done', 'failed', 'cancelled')));";
  if (sqlite3_exec(db, sql, nullptr, nullptr, nullptr) != SQLITE_OK) {
    std::cerr << "Could not create jobs table: " << sqlite3_errmsg(db) << "\n";
    return -1;
  }
  return 0;
}

// Returns the job's ID, or nullopt if it could not be queued.
std::optional<std::int64_t> queue_job(sqlite3 *db,
                                      const std::vector<std::string> &job) {
  // Each argument ends with NUL, as in the request. SQLite's text functions
  // stop at NUL, so the column is a blob.
  std::string encoded;
  for (const std::string &arg : job) {
    encoded += arg;
    encoded += '\0';
  }
  sqlite3_stmt *handle = nullptr;
  int rc = sqlite3_prepare_v2(db, "INSERT INTO jobs (Job) VALUES (?);", -1,
                              &handle, nullptr);
  std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)> insert(
      handle, sqlite3_finalize);
  if (rc == SQLITE_OK) {
    rc = sqlite3_bind_blob(insert.get(), 1, encoded.data(), encoded.size(),
                           SQLITE_STATIC);
  }
  if (rc == SQLITE_OK) {
    rc = sqlite3_step(insert.get());
  }
  if (rc != SQLITE_DONE) {
    std::cerr << "Could not queue job: " << sqlite3_errmsg(db) << "\n";
    return std::nullopt;
  }
  return sqlite3_last_insert_rowid(db);
}
