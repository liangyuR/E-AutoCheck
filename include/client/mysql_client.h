// ===========================
// file: mysql_client.h
// ===========================
#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

// glog
#include <glog/logging.h>

// yaml-cpp
#include <yaml-cpp/yaml.h>

// MySQL Connector/C++ X DevAPI
#include <mysqlx/xdevapi.h>

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
  uint16_t port = 33060; // X Protocol 默认 33060
  std::string user;
  std::string password;
  std::string database;
};

struct PoolConfig {
  bool enabled = true;
  int max_size = 10;
  int min_idle = 2;
  int acquire_timeout_ms = 3000;
  int keepalive_sec = 60; // 借出/归还时 ping
};

struct RetryConfig {
  int max_retries = 2;
  int base_backoff_ms = 200;
};

struct BehaviorConfig {
  bool read_from_replicas = true;
  bool force_master_in_tx = true; // 预留
  std::string timezone = "UTC";
  int slow_sql_ms = 200;
};

struct Timeouts {
  int connect_timeout_ms = 5000;
  int read_timeout_ms = 5000;
  int write_timeout_ms = 5000;
};

struct MySqlConfig {
  Endpoint rw;
  std::vector<Endpoint> ro; // optional
  std::string charset = "utf8mb4";
  Timeouts timeouts;
};

struct Config {
  MySqlConfig mysql;
  PoolConfig pool;
  RetryConfig retry;
  BehaviorConfig behavior;

  static Config FromYamlFile(const std::string &path);
};

// ---------------------------
// Internal helpers (opaque in header)
// ---------------------------
class ConnectionHolder;
class ConnectionPool;

// ---------------------------
// Public client (sync)
// ---------------------------
class MySqlClient {
public:
  explicit MySqlClient(const Config &cfg);
  ~MySqlClient();

  // SELECT; useReplica=true → 从库；false → 主库
  std::vector<std::unordered_map<std::string, std::string>>
  executeQuery(const std::string &sql, const std::vector<DbValue> &params = {},
               bool useReplica = true);

  // INSERT/UPDATE/DELETE
  uint64_t executeUpdate(const std::string &sql,
                         const std::vector<DbValue> &params = {});

  // Utility: simple ping master
  void ping();

private:
  std::unique_ptr<ConnectionPool> pool_;
  Config cfg_;
};

} // namespace db