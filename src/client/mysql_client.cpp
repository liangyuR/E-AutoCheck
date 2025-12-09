// ===========================
// file: mysql_client.cpp
// ===========================
#include "client/mysql_client.h"

#include <glog/logging.h>
#include <sstream>
#include <thread>

namespace db {

// ---------------------------
// YAML loader
// ---------------------------
static Endpoint ParseEndpoint(const YAML::Node &n) {
  Endpoint end_p;
  if (n["host"])
    end_p.host = n["host"].as<std::string>();
  if (n["port"])
    end_p.port = n["port"].as<uint16_t>();
  if (n["user"])
    end_p.user = n["user"].as<std::string>();
  if (n["password"])
    end_p.password = n["password"].as<std::string>();
  if (n["database"])
    end_p.database = n["database"].as<std::string>();
  return end_p;
}

Config Config::FromYamlFile(const std::string &path) {
  YAML::Node root = YAML::LoadFile(path);

  Config config;

  const auto mysql = root["mysql"];
  if (!mysql)
    throw DatabaseError("Missing 'mysql' section in YAML");

  // Parse single endpoint (support both direct endpoint and rw/ro for backward
  // compatibility)
  if (mysql["endpoint"]) {
    config.mysql.endpoint = ParseEndpoint(mysql["endpoint"]);
  } else if (mysql["rw"]) {
    // Backward compatibility: use rw as the single endpoint
    config.mysql.endpoint = ParseEndpoint(mysql["rw"]);
  } else {
    throw DatabaseError("Missing 'endpoint' or 'rw' in mysql section");
  }

  if (mysql["charset"])
    config.mysql.charset = mysql["charset"].as<std::string>();

  if (mysql["connect_timeout_ms"])
    config.mysql.timeouts.connect_timeout_ms =
        mysql["connect_timeout_ms"].as<int>();
  if (mysql["read_timeout_ms"])
    config.mysql.timeouts.read_timeout_ms = mysql["read_timeout_ms"].as<int>();
  if (mysql["write_timeout_ms"])
    config.mysql.timeouts.write_timeout_ms =
        mysql["write_timeout_ms"].as<int>();

  if (root["retry"]) {
    const auto retry = root["retry"];
    if (retry["max_retries"])
      config.retry.max_retries = retry["max_retries"].as<int>();
    if (retry["base_backoff_ms"])
      config.retry.base_backoff_ms = retry["base_backoff_ms"].as<int>();
  }

  if (root["behavior"]) {
    const auto behavior = root["behavior"];
    if (behavior["timezone"])
      config.behavior.timezone = behavior["timezone"].as<std::string>();
    if (behavior["slow_sql_ms"])
      config.behavior.slow_sql_ms = behavior["slow_sql_ms"].as<int>();
  }

  return config;
}

// ---------------------------
// Param binding (JDBC PreparedStatement)
// ---------------------------
static void BindParams(sql::PreparedStatement *stmt,
                       const std::vector<DbValue> &params) {
  for (size_t i = 0; i < params.size(); ++i) {
    const auto &v = params[i];
    if (std::holds_alternative<std::nullptr_t>(v)) {
      stmt->setNull(i + 1, sql::DataType::VARCHAR); // JDBC uses 1-based index
    } else if (std::holds_alternative<int64_t>(v)) {
      stmt->setInt64(i + 1, std::get<int64_t>(v));
    } else if (std::holds_alternative<double>(v)) {
      stmt->setDouble(i + 1, std::get<double>(v));
    } else {
      stmt->setString(i + 1, std::get<std::string>(v));
    }
  }
}

// ---------------------------
// Convert results (ResultSet -> vector<DbRow>)
// ---------------------------
static std::vector<DbRow> FetchAll(sql::ResultSet *res) {
  std::vector<DbRow> rows;

  // Get column metadata
  sql::ResultSetMetaData *meta = res->getMetaData();
  size_t col_count = meta->getColumnCount();
  std::vector<std::string> names;
  names.reserve(col_count);
  for (size_t i = 1; i <= col_count; ++i) { // JDBC uses 1-based index
    std::string label = meta->getColumnLabel(i);
    if (label.empty())
      label = meta->getColumnName(i);
    names.push_back(std::move(label));
  }

  // Fetch all rows
  while (res->next()) {
    DbRow::RawMap out;
    out.reserve(col_count);
    for (size_t i = 0; i < col_count; ++i) {
      std::string value;
      if (res->isNull(i + 1)) {
        value = "";
      } else {
        // Get as string for all types
        value = res->getString(i + 1);
      }
      out[names[i]] = value;
    }
    rows.emplace_back(std::move(out));
  }
  return rows;
}

// ---------------------------
// Retry helper
// ---------------------------
template <typename Fn>
auto WithRetry(const RetryConfig &rc, Fn &&fn) -> decltype(fn()) {
  int attempt = 0;
  while (true) {
    try {
      return fn();
    } catch (const sql::SQLException &e) {
      const std::string msg = e.what();
      const bool maybeTransient =
          msg.find("Lost connection") != std::string::npos ||
          msg.find("gone away") != std::string::npos ||
          msg.find("timeout") != std::string::npos ||
          e.getErrorCode() == 2006 || // MySQL server has gone away
          e.getErrorCode() == 2013;   // Lost connection to MySQL server
      if (!maybeTransient || attempt >= rc.max_retries)
        throw;
      const int backoff = rc.base_backoff_ms * (1 << attempt);
      std::this_thread::sleep_for(std::chrono::milliseconds(backoff));
      ++attempt;
    }
  }
}

// ---------------------------
// MySqlClient impl (JDBC Classic Protocol)
// ---------------------------
std::unique_ptr<MySqlClient> MySqlClient::instance_;
std::mutex MySqlClient::instance_mutex_;

void MySqlClient::Init(const Config &cfg) {
  std::lock_guard<std::mutex> lock(instance_mutex_);
  if (!instance_) {
    instance_.reset(new MySqlClient(cfg));
  }
}

void MySqlClient::Shutdown() {
  std::lock_guard<std::mutex> lock(instance_mutex_);
  instance_.reset();
}

MySqlClient *MySqlClient::GetInstance() {
  std::lock_guard<std::mutex> lock(instance_mutex_);
  if (!instance_) {
    LOG(FATAL) << "MySqlClient::GetInstance() called before Init()";
  }
  return instance_.get();
}

MySqlClient::MySqlClient(const Config &cfg) : cfg_(cfg) {
  // Lazy connection - don't connect here, connect on first use
}

MySqlClient::~MySqlClient() = default;

void MySqlClient::ensureConnection() {
  std::lock_guard<std::mutex> lock(connection_mutex_);
  ensureConnectionUnlocked();
}

void MySqlClient::ensureConnectionUnlocked() {
  // This method assumes connection_mutex_ is already locked
  if (connection_) {
    // Connection exists, try a simple ping to verify it's alive
    try {
      std::unique_ptr<sql::Statement> stmt(connection_->createStatement());
      std::unique_ptr<sql::ResultSet> res(stmt->executeQuery("SELECT 1"));
      res->next(); // Consume result
      return;
    } catch (const sql::SQLException &) {
      // Connection is dead, reset it
      connection_.reset();
    }
  }

  // Create new connection
  sql::Driver *driver = sql::mysql::get_driver_instance();
  if (!driver) {
    throw DatabaseError("Failed to get MySQL driver instance");
  }

  // Build connection URL (format: tcp://host:port)
  std::ostringstream url;
  url << "tcp://" << cfg_.mysql.endpoint.host << ":"
      << cfg_.mysql.endpoint.port;

  LOG(INFO) << "Connecting to " << cfg_.mysql.endpoint.host << ":"
            << cfg_.mysql.endpoint.port;

  // Connect using driver->connect(url, user, pass) as per JDBC API
  connection_.reset(driver->connect(url.str(), cfg_.mysql.endpoint.user,
                                    cfg_.mysql.endpoint.password));

  // Set schema (database)
  if (!cfg_.mysql.endpoint.database.empty()) {
    connection_->setSchema(cfg_.mysql.endpoint.database);
  }

  // Set charset if specified
  if (!cfg_.mysql.charset.empty()) {
    std::unique_ptr<sql::Statement> stmt(connection_->createStatement());
    std::ostringstream charset_sql;
    charset_sql << "SET NAMES '" << cfg_.mysql.charset << "'";
    stmt->execute(charset_sql.str());
  }

  // Set timezone if specified
  if (!cfg_.behavior.timezone.empty()) {
    std::unique_ptr<sql::Statement> stmt(connection_->createStatement());
    std::ostringstream tz_sql;
    tz_sql << "SET time_zone='" << cfg_.behavior.timezone << "'";
    stmt->execute(tz_sql.str());
  }
}

void MySqlClient::reconnect() {
  std::lock_guard<std::mutex> lock(connection_mutex_);
  LOG(WARNING) << "Reconnecting to " << cfg_.mysql.endpoint.host << ":"
               << cfg_.mysql.endpoint.port;
  connection_.reset();
  ensureConnectionUnlocked();
}

absl::StatusOr<std::vector<DbRow>>
MySqlClient::executeQuery(const std::string &sql,
                          const std::vector<DbValue> &params) {
  const auto start = std::chrono::steady_clock::now();
  auto work = [&]() -> std::vector<DbRow> {
    std::lock_guard<std::mutex> lock(connection_mutex_);
    ensureConnectionUnlocked();

    std::unique_ptr<sql::PreparedStatement> stmt;
    std::unique_ptr<sql::ResultSet> res;

    if (params.empty()) {
      // No parameters, use regular statement
      std::unique_ptr<sql::Statement> regular_stmt(
          connection_->createStatement());
      res.reset(regular_stmt->executeQuery(sql));
    } else {
      // Has parameters, use prepared statement
      stmt.reset(connection_->prepareStatement(sql));
      BindParams(stmt.get(), params);
      res.reset(stmt->executeQuery());
    }

    auto rows = FetchAll(res.get());

    const auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(
                         std::chrono::steady_clock::now() - start)
                         .count();
    if (dur >= cfg_.behavior.slow_sql_ms) {
      LOG(WARNING) << "[SLOW " << dur << "ms] " << sql;
    } else {
      LOG(INFO) << "[OK " << dur << "ms] rows=" << rows.size();
    }

    return rows;
  };

  try {
    return WithRetry(cfg_.retry, work);
  } catch (const sql::SQLException &e) {
    // Try to reconnect on error
    try {
      reconnect();
      // Retry once after reconnect
      return WithRetry(cfg_.retry, work);
    } catch (...) {
      std::ostringstream oss;
      oss << "Query failed: " << e.what() << " (Error: " << e.getErrorCode()
          << ")";
      LOG(ERROR) << oss.str() << " SQL=" << sql;
      return absl::InternalError(oss.str());
    }
  } catch (const std::exception &e) {
    std::ostringstream oss;
    oss << "Query failed: " << e.what();
    LOG(ERROR) << oss.str() << " SQL=" << sql;
    return absl::InternalError(oss.str());
  }
}

absl::StatusOr<uint64_t>
MySqlClient::executeUpdate(const std::string &sql,
                           const std::vector<DbValue> &params) {
  const auto start = std::chrono::steady_clock::now();
  auto work = [&]() -> uint64_t {
    std::lock_guard<std::mutex> lock(connection_mutex_);
    ensureConnectionUnlocked();

    std::unique_ptr<sql::PreparedStatement> stmt;
    int affected = 0;

    if (params.empty()) {
      // No parameters, use regular statement
      std::unique_ptr<sql::Statement> regular_stmt(
          connection_->createStatement());
      affected = regular_stmt->executeUpdate(sql);
    } else {
      // Has parameters, use prepared statement
      stmt.reset(connection_->prepareStatement(sql));
      BindParams(stmt.get(), params);
      affected = stmt->executeUpdate();
    }

    const auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(
                         std::chrono::steady_clock::now() - start)
                         .count();
    if (dur >= cfg_.behavior.slow_sql_ms) {
      LOG(WARNING) << "[SLOW " << dur << "ms] " << sql;
    } else {
      LOG(INFO) << "[OK " << dur << "ms] affected=" << affected;
    }

    return static_cast<uint64_t>(affected);
  };

  try {
    return WithRetry(cfg_.retry, work);
  } catch (const sql::SQLException &e) {
    // Try to reconnect on error
    try {
      reconnect();
      // Retry once after reconnect
      return WithRetry(cfg_.retry, work);
    } catch (...) {
      std::ostringstream oss;
      oss << "Update failed: " << e.what() << " (Error: " << e.getErrorCode()
          << ")";
      LOG(ERROR) << oss.str() << " SQL=" << sql;
      return absl::InternalError(oss.str());
    }
  } catch (const std::exception &e) {
    std::ostringstream oss;
    oss << "Update failed: " << e.what();
    LOG(ERROR) << oss.str() << " SQL=" << sql;
    return absl::InternalError(oss.str());
  }
}

absl::Status MySqlClient::ping() {
  try {
    std::lock_guard<std::mutex> lock(connection_mutex_);
    ensureConnectionUnlocked();
    std::unique_ptr<sql::Statement> stmt(connection_->createStatement());
    std::unique_ptr<sql::ResultSet> res(stmt->executeQuery("SELECT 1"));
    res->next(); // Consume result
    return absl::OkStatus();
  } catch (const sql::SQLException &e) {
    try {
      reconnect();
      return absl::OkStatus();
    } catch (...) {
      return absl::InternalError(std::string("Ping failed: ") + e.what());
    }
  } catch (const std::exception &e) {
    return absl::InternalError(std::string("Ping failed: ") + e.what());
  }
}

} // namespace db
