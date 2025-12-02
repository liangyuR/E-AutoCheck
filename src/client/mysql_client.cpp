// ===========================
// file: mysql_client.cpp
// ===========================
#include "client/mysql_client.h"
#include <cassert>
#include <deque>
#include <iomanip>
#include <list>
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

  config.mysql.rw = ParseEndpoint(mysql["rw"]);

  if (mysql["ro"]) {
    for (const auto &node : mysql["ro"]) {
      config.mysql.ro.push_back(ParseEndpoint(node));
    }
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

  if (root["pool"]) {
    const auto poll = root["pool"];
    if (poll["enabled"])
      config.pool.enabled = poll["enabled"].as<bool>();
    if (poll["max_size"])
      config.pool.max_size = poll["max_size"].as<int>();
    if (poll["min_idle"])
      config.pool.min_idle = poll["min_idle"].as<int>();
    if (poll["acquire_timeout_ms"])
      config.pool.acquire_timeout_ms = poll["acquire_timeout_ms"].as<int>();
    if (poll["keepalive_sec"])
      config.pool.keepalive_sec = poll["keepalive_sec"].as<int>();
  }

  if (root["retry"]) {
    const auto retry = root["retry"];
    if (retry["max_retries"])
      config.retry.max_retries = retry["max_retries"].as<int>();
    if (retry["base_backoff_ms"])
      config.retry.base_backoff_ms = retry["base_backoff_ms"].as<int>();
  }

  if (root["behavior"]) {
    const auto behavior = root["behavior"];
    if (behavior["read_from_replicas"])
      config.behavior.read_from_replicas =
          behavior["read_from_replicas"].as<bool>();
    if (behavior["force_master_in_tx"])
      config.behavior.force_master_in_tx =
          behavior["force_master_in_tx"].as<bool>();
    if (behavior["timezone"])
      config.behavior.timezone = behavior["timezone"].as<std::string>();
    if (behavior["slow_sql_ms"])
      config.behavior.slow_sql_ms = behavior["slow_sql_ms"].as<int>();
  }

  return config;
}

// ---------------------------
// ConnectionHolder (X DevAPI)
// ---------------------------
class ConnectionHolder {
public:
  ConnectionHolder(const Endpoint &ep, const MySqlConfig &mcfg,
                   const BehaviorConfig &behavior)
      : endpoint_(ep), mcfg_(mcfg), behavior_(behavior) {
    connect();
  }

  ~ConnectionHolder() = default;

  mysqlx::Session &sess() { return *session_; }

  void ensureSessionSettings() {
    // 字符集与时区保持一致性
    if (!mcfg_.charset.empty()) {
      session_->sql("SET NAMES " + mcfg_.charset).execute();
    }
    if (!behavior_.timezone.empty()) {
      session_->sql("SET time_zone=?").bind(behavior_.timezone).execute();
    }
  }

  void ping() {
    try {
      session_->sql("SELECT 1").execute();
    } catch (const mysqlx::Error &) {
      reconnect();
    }
  }

  const Endpoint &endpoint() const { return endpoint_; }

private:
  void connect() {
    // 使用显式主机/端口/用户/密码
    mysqlx::SessionSettings settings(
        mysqlx::SessionOption::HOST, endpoint_.host,
        mysqlx::SessionOption::PORT, static_cast<int>(endpoint_.port),
        mysqlx::SessionOption::USER, endpoint_.user, mysqlx::SessionOption::PWD,
        endpoint_.password, mysqlx::SessionOption::CONNECT_TIMEOUT,
        mcfg_.timeouts.connect_timeout_ms);
    session_.reset(new mysqlx::Session(settings));
    if (!endpoint_.database.empty()) {
      session_->sql("USE " + endpoint_.database).execute();
    }
    ensureSessionSettings();
  }

  void reconnect() {
    LOG(WARNING) << "Reconnecting to " << endpoint_.host << ":"
                 << endpoint_.port;
    connect();
  }

private:
  Endpoint endpoint_;
  MySqlConfig mcfg_;
  BehaviorConfig behavior_;
  std::unique_ptr<mysqlx::Session> session_;
};

// ---------------------------
// ConnectionPool
// ---------------------------
class ConnectionPool {
public:
  explicit ConnectionPool(const Config &cfg) : cfg_(cfg), rr_index_(0) {
    if (!cfg_.pool.enabled) {
      master_.reset(
          new ConnectionHolder(cfg_.mysql.rw, cfg_.mysql, cfg_.behavior));
      if (cfg_.behavior.read_from_replicas && !cfg_.mysql.ro.empty()) {
        for (auto &ep : cfg_.mysql.ro) {
          replicas_.emplace_back(
              new ConnectionHolder(ep, cfg_.mysql, cfg_.behavior));
        }
      }
      return;
    }

    // 预热主库
    for (int i = 0; i < cfg_.pool.min_idle; ++i) {
      idle_master_.push(std::unique_ptr<ConnectionHolder>(
          new ConnectionHolder(cfg_.mysql.rw, cfg_.mysql, cfg_.behavior)));
    }

    // 预热从库（每个端点 1 条）
    if (cfg_.behavior.read_from_replicas) {
      for (auto &ep : cfg_.mysql.ro) {
        idle_replicas_.push(std::unique_ptr<ConnectionHolder>(
            new ConnectionHolder(ep, cfg_.mysql, cfg_.behavior)));
        replica_templates_.push_back(ep);
      }
    }
  }

  std::unique_ptr<ConnectionHolder> acquireMaster(int timeout_ms) {
    if (!cfg_.pool.enabled) {
      master_->ping();
      // 无池：每次新建一个 session（避免共享状态）
      return std::unique_ptr<ConnectionHolder>(
          new ConnectionHolder(master_->endpoint(), cfg_.mysql, cfg_.behavior));
    }
    auto deadline = std::chrono::steady_clock::now() +
                    std::chrono::milliseconds(timeout_ms);
    std::unique_lock<std::mutex> lk(mu_master_);
    while (true) {
      if (!idle_master_.empty()) {
        auto ptr = std::move(idle_master_.front());
        idle_master_.pop();
        lk.unlock();
        ptr->ping();
        return ptr;
      }
      if (total_master_ < cfg_.pool.max_size) {
        ++total_master_;
        lk.unlock();
        try {
          auto ptr = std::unique_ptr<ConnectionHolder>(
              new ConnectionHolder(cfg_.mysql.rw, cfg_.mysql, cfg_.behavior));
          ptr->ping();
          return ptr;
        } catch (...) {
          std::lock_guard<std::mutex> g(mu_master_);
          --total_master_;
          throw;
        }
      }
      if (cv_master_.wait_until(lk, deadline) == std::cv_status::timeout) {
        throw DatabaseError("Acquire master connection timeout");
      }
    }
  }

  std::unique_ptr<ConnectionHolder> acquireReplica(int timeout_ms) {
    if (!cfg_.behavior.read_from_replicas || cfg_.mysql.ro.empty()) {
      return acquireMaster(timeout_ms);
    }

    if (!cfg_.pool.enabled) {
      const auto ep = nextReplicaEndpoint();
      return std::unique_ptr<ConnectionHolder>(
          new ConnectionHolder(ep, cfg_.mysql, cfg_.behavior));
    }

    auto deadline = std::chrono::steady_clock::now() +
                    std::chrono::milliseconds(timeout_ms);
    std::unique_lock<std::mutex> lk(mu_replica_);
    while (true) {
      if (!idle_replicas_.empty()) {
        auto ptr = std::move(idle_replicas_.front());
        idle_replicas_.pop();
        lk.unlock();
        ptr->ping();
        return ptr;
      }
      if (total_replicas_ < cfg_.pool.max_size && !replica_templates_.empty()) {
        ++total_replicas_;
        auto ep = nextReplicaEndpoint();
        lk.unlock();
        try {
          auto ptr = std::unique_ptr<ConnectionHolder>(
              new ConnectionHolder(ep, cfg_.mysql, cfg_.behavior));
          ptr->ping();
          return ptr;
        } catch (...) {
          std::lock_guard<std::mutex> g(mu_replica_);
          --total_replicas_;
          throw;
        }
      }
      if (cv_replica_.wait_until(lk, deadline) == std::cv_status::timeout) {
        throw DatabaseError("Acquire replica connection timeout");
      }
    }
  }

  void releaseMaster(std::unique_ptr<ConnectionHolder> c) {
    if (!cfg_.pool.enabled)
      return;
    std::lock_guard<std::mutex> lk(mu_master_);
    idle_master_.push(std::move(c));
    cv_master_.notify_one();
  }

  void releaseReplica(std::unique_ptr<ConnectionHolder> c) {
    if (!cfg_.pool.enabled)
      return;
    std::lock_guard<std::mutex> lk(mu_replica_);
    idle_replicas_.push(std::move(c));
    cv_replica_.notify_one();
  }

  void pingMaster() {
    if (!cfg_.pool.enabled) {
      master_->ping();
      return;
    }
    std::lock_guard<std::mutex> lk(mu_master_);
    if (!idle_master_.empty())
      idle_master_.front()->ping();
  }

private:
  Endpoint nextReplicaEndpoint() {
    size_t n = cfg_.mysql.ro.size();
    if (n == 0)
      return cfg_.mysql.rw;
    auto i = rr_index_.fetch_add(1, std::memory_order_relaxed) % n;
    return cfg_.mysql.ro[i];
  }

private:
  Config cfg_;

  // no-pool singletons
  std::unique_ptr<ConnectionHolder> master_;
  std::vector<std::unique_ptr<ConnectionHolder>> replicas_;

  // pooled masters
  std::mutex mu_master_;
  std::condition_variable cv_master_;
  std::queue<std::unique_ptr<ConnectionHolder>> idle_master_;
  int total_master_ = 0;

  // pooled replicas
  std::mutex mu_replica_;
  std::condition_variable cv_replica_;
  std::queue<std::unique_ptr<ConnectionHolder>> idle_replicas_;
  int total_replicas_ = 0;

  // replica endpoints for construction
  std::vector<Endpoint> replica_templates_;
  std::atomic<size_t> rr_index_;
};

// ---------------------------
// Param binding (X DevAPI)
// ---------------------------
static void BindParams(mysqlx::SqlStatement &stmt,
                       const std::vector<DbValue> &params) {
  for (const auto &v : params) {
    if (std::holds_alternative<std::nullptr_t>(v)) {
      stmt.bind(nullptr); // 绑定 NULL
    } else if (std::holds_alternative<int64_t>(v)) {
      stmt.bind(std::get<int64_t>(v));
    } else if (std::holds_alternative<double>(v)) {
      stmt.bind(std::get<double>(v));
    } else {
      stmt.bind(std::get<std::string>(v));
    }
  }
}

// ---------------------------
// Convert results (SqlResult -> vector<map>)
// ---------------------------
static std::string ValueToString(const mysqlx::Value &v) {
  switch (v.getType()) {
  case mysqlx::Value::Type::VNULL:
    return std::string();
  case mysqlx::Value::Type::INT64:
    return std::to_string(v.get<int64_t>());
  case mysqlx::Value::Type::UINT64:
    return std::to_string(v.get<uint64_t>());
  case mysqlx::Value::Type::FLOAT:
    return std::to_string(v.get<float>());
  case mysqlx::Value::Type::DOUBLE:
    return std::to_string(v.get<double>());
  case mysqlx::Value::Type::BOOL:
    return v.get<bool>() ? "1" : "0";
  case mysqlx::Value::Type::STRING:
    return v.get<std::string>();
  case mysqlx::Value::Type::RAW: {
    // RAW 类型可以直接转换为 std::string
    return v.get<std::string>(); // 注意：可能包含 '\0'
  }
  default: {
    std::ostringstream oss;
    oss << "<unsupported>";
    return oss.str();
  }
  }
}

static std::vector<std::unordered_map<std::string, std::string>>
FetchAll(mysqlx::SqlResult &res) {
  std::vector<std::unordered_map<std::string, std::string>> rows;

  const auto &cols = res.getColumns(); // 列信息（含标签）
  std::vector<std::string> names;
  for (const auto &col : cols) {
    std::string label = col.getColumnLabel();
    if (label.empty())
      label = col.getColumnName();
    names.push_back(std::move(label));
  }

  while (true) {
    mysqlx::Row row = res.fetchOne();
    if (!row)
      break; // fetchOne() 无数据时返回 "空" 行
    std::unordered_map<std::string, std::string> out;
    out.reserve(names.size());
    for (size_t i = 0; i < names.size(); ++i) {
      out[names[i]] = ValueToString(row[i]);
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
    } catch (const mysqlx::Error &e) {
      const std::string msg = e.what();
      const bool maybeTransient =
          msg.find("Lost connection") != std::string::npos ||
          msg.find("gone away") != std::string::npos ||
          msg.find("timeout") != std::string::npos;
      if (!maybeTransient || attempt >= rc.max_retries)
        throw;
      const int backoff = rc.base_backoff_ms * (1 << attempt);
      std::this_thread::sleep_for(std::chrono::milliseconds(backoff));
      ++attempt;
    }
  }
}

// ---------------------------
// MySqlClient impl (X DevAPI)
// ---------------------------
std::unique_ptr<MySqlClient> MySqlClient::instance_;
std::mutex MySqlClient::instance_mutex_;

void MySqlClient::Init(const Config &cfg) {
  std::lock_guard<std::mutex> lock(instance_mutex_);
  if (!instance_) {
    // private constructor, cannot use make_unique
    instance_.reset(new MySqlClient(cfg));
  }
}

MySqlClient *MySqlClient::GetInstance() { return instance_.get(); }

MySqlClient::MySqlClient(const Config &cfg) : cfg_(cfg) {
  pool_.reset(new ConnectionPool(cfg_));
}

MySqlClient::~MySqlClient() = default;

std::vector<std::unordered_map<std::string, std::string>>
MySqlClient::executeQuery(const std::string &sql,
                          const std::vector<DbValue> &params, bool useReplica) {
  const auto start = std::chrono::steady_clock::now();
  auto work = [&]() {
    std::unique_ptr<ConnectionHolder> conn =
        useReplica ? pool_->acquireReplica(cfg_.pool.acquire_timeout_ms)
                   : pool_->acquireMaster(cfg_.pool.acquire_timeout_ms);
    auto stmt = conn->sess().sql(sql);
    BindParams(stmt, params);
    auto res = stmt.execute();
    auto rows = FetchAll(res);

    const auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(
                         std::chrono::steady_clock::now() - start)
                         .count();
    if (dur >= cfg_.behavior.slow_sql_ms) {
      LOG(WARNING) << "[SLOW " << dur << "ms] " << sql;
    } else {
      LOG(INFO) << "[OK " << dur << "ms] rows=" << rows.size();
    }

    if (useReplica)
      pool_->releaseReplica(std::move(conn));
    else
      pool_->releaseMaster(std::move(conn));
    return rows;
  };

  try {
    return WithRetry(cfg_.retry, work);
  } catch (const mysqlx::Error &e) {
    std::ostringstream oss;
    oss << "Query failed: " << e.what();
    LOG(ERROR) << oss.str() << " SQL=" << sql;
    throw DatabaseError(oss.str(), 0, sql);
  }
}

uint64_t MySqlClient::executeUpdate(const std::string &sql,
                                    const std::vector<DbValue> &params) {
  const auto start = std::chrono::steady_clock::now();
  auto work = [&]() -> uint64_t {
    std::unique_ptr<ConnectionHolder> conn =
        pool_->acquireMaster(cfg_.pool.acquire_timeout_ms);
    auto stmt = conn->sess().sql(sql);
    BindParams(stmt, params);
    auto res = stmt.execute();
    const uint64_t affected =
        static_cast<uint64_t>(res.getAffectedItemsCount());

    const auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(
                         std::chrono::steady_clock::now() - start)
                         .count();
    if (dur >= cfg_.behavior.slow_sql_ms) {
      LOG(WARNING) << "[SLOW " << dur << "ms] " << sql;
    } else {
      LOG(INFO) << "[OK " << dur << "ms] affected=" << affected;
    }

    pool_->releaseMaster(std::move(conn));
    return affected;
  };

  try {
    return WithRetry(cfg_.retry, work);
  } catch (const mysqlx::Error &e) {
    std::ostringstream oss;
    oss << "Update failed: " << e.what();
    LOG(ERROR) << oss.str() << " SQL=" << sql;
    throw DatabaseError(oss.str(), 0, sql);
  }
}

void MySqlClient::ping() {
  try {
    pool_->pingMaster();
  } catch (const mysqlx::Error &e) {
    throw DatabaseError(std::string("Ping failed: ") + e.what(), 0);
  }
}

} // namespace db