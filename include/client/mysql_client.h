// ===========================
// file: mysql_client.h
// ===========================
#pragma once

#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

// glog
#include <glog/logging.h>

// yaml-cpp
#include <yaml-cpp/yaml.h>

// MySQL Connector/C++ JDBC Classic Protocol
#include <mysql/jdbc.h>
// absl
#include <absl/status/statusor.h>

// db
#include "db/db_row.h"

namespace db {

// ---------------------------
// Error type
// ---------------------------
class DatabaseError : public std::runtime_error {
public:
  explicit DatabaseError(const std::string &msg, int code = 0,
                         std::string sql = {})
      : std::runtime_error(msg), code_(code), sql_(std::move(sql)) {}

  int code() const noexcept { return code_; }
  const std::string &sql() const noexcept { return sql_; }

private:
  int code_;
  std::string sql_;
};

// ---------------------------
// Param value
// ---------------------------
using DbValue = std::variant<std::nullptr_t, int64_t, double, std::string>;

// ---------------------------
// Config structs (no SSL)
// ---------------------------
struct Endpoint {
  std::string host = "127.0.0.1";
  uint16_t port = 3306; // Classic Protocol 默认 3306
  std::string user;
  std::string password;
  std::string database;
};

struct RetryConfig {
  int max_retries = 2;
  int base_backoff_ms = 200;
};

struct BehaviorConfig {
  std::string timezone = "UTC";
  int slow_sql_ms = 200;
};

struct Timeouts {
  int connect_timeout_ms = 5000;
  int read_timeout_ms = 5000;
  int write_timeout_ms = 5000;
};

struct MySqlConfig {
  Endpoint endpoint;
  std::string charset = "utf8mb4";
  Timeouts timeouts;
};

struct Config {
  MySqlConfig mysql;
  RetryConfig retry;
  BehaviorConfig behavior;

  static Config FromYamlFile(const std::string &path);
};

// ---------------------------
// Internal helpers (opaque in header)
// ---------------------------
// Simple connection holder - no pool needed

// ---------------------------
// Public client (sync)
// ---------------------------
class MySqlClient {
public:
  static void Init(const Config &cfg);
  static MySqlClient *GetInstance();

private:
  explicit MySqlClient(const Config &cfg);

public:
  ~MySqlClient();

  // Delete copy and assignment
  MySqlClient(const MySqlClient &) = delete;
  MySqlClient &operator=(const MySqlClient &) = delete;

  // SELECT
  absl::StatusOr<std::vector<DbRow>>
  executeQuery(const std::string &sql, const std::vector<DbValue> &params = {});

  // INSERT/UPDATE/DELETE
  absl::StatusOr<uint64_t>
  executeUpdate(const std::string &sql,
                const std::vector<DbValue> &params = {});

  // Utility: simple ping
  absl::Status ping();

private:
  void ensureConnection();
  void ensureConnectionUnlocked(); // Internal version without lock
  void reconnect();

  std::unique_ptr<sql::Connection> connection_;
  std::mutex connection_mutex_;
  Config cfg_;

  static std::unique_ptr<MySqlClient> instance_;
  static std::mutex instance_mutex_;
};

} // namespace db