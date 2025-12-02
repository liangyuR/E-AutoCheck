#include "client/redis_client.h"

#include <chrono>
#include <memory>

namespace client {

std::unique_ptr<RedisClient> RedisClient::instance_;
std::mutex RedisClient::instance_mutex_;

void RedisClient::Init(const YAML::Node &redis_config) {
  std::lock_guard<std::mutex> lock(instance_mutex_);
  if (!instance_) {
    instance_.reset(new RedisClient(redis_config));
  }
}

RedisClient *RedisClient::GetInstance() { return instance_.get(); }

RedisClient::RedisClient(const YAML::Node &redis_config) {
  LoadConfig_(redis_config);
  // 不再在构造函数中连接，避免阻塞
}

RedisClient::~RedisClient() { Disconnect(); }

bool RedisClient::IsConnected() const { return connected_.load(); }

bool RedisClient::Connect() {
  if (connected_.load()) {
    return true;
  }

  if (connecting_.load()) {
    // 如果正在连接，等待完成
    return WaitForConnection();
  }

  // 启动异步连接
  ConnectAsync();
  return WaitForConnection();
}

void RedisClient::ConnectAsync() {
  if (connected_.load() || connecting_.load()) {
    return;
  }

  connecting_.store(true);

  // 启动异步连接任务
  connection_future_ =
      std::async(std::launch::async, [this]() { return ConnectInternal_(); });
}

bool RedisClient::WaitForConnection(std::chrono::milliseconds timeout) {
  if (connected_.load()) {
    return true;
  }

  if (!connecting_.load()) {
    return false;
  }

  // 等待连接完成
  if (connection_future_.valid()) {
    auto status = connection_future_.wait_for(timeout);
    if (status == std::future_status::ready) {
      bool result = connection_future_.get();
      connecting_.store(false);
      return result;
    }
    // 超时
    connecting_.store(false);
    LOG(ERROR) << "[RedisClient] Connection timeout after " << timeout.count()
               << "ms";
    return false;
  }

  return false;
}

bool RedisClient::ConnectInternal_() {
  try {
    redis_ = std::make_unique<sw::redis::Redis>(connection_options_);
    redis_->ping(); // Test connection
    connected_.store(true);
    LOG(INFO) << "[RedisClient] Redis connected successfully";
    return true;
  } catch (const sw::redis::Error &err) {
    LOG(ERROR) << "[RedisClient] Redis connection failed: " << err.what();
    connected_.store(false);
    return false;
  }
}

void RedisClient::Disconnect() {
  if (redis_) {
    redis_.reset();
    connected_.store(false);
    connecting_.store(false);
    LOG(INFO) << "[RedisClient] Redis disconnected";
  }
}

bool RedisClient::Set(const std::string &key, const std::string &value) {
  if (!connected_.load()) {
    LOG(ERROR) << "[RedisClient] Redis not connected";
    return false;
  }

  try {
    redis_->set(key, value);
    return true;
  } catch (const sw::redis::Error &err) {
    LOG(ERROR) << "[RedisClient] Redis SET failed for key '" << key
               << "': " << err.what();
    return false;
  }
}

std::optional<std::string> RedisClient::Get(const std::string &key) const {
  if (!connected_.load()) {
    LOG(ERROR) << "[RedisClient] Redis not connected";
    return std::nullopt;
  }

  try {
    auto value = redis_->get(key);
    return value ? std::optional<std::string>(value.value()) : std::nullopt;
  } catch (const sw::redis::Error &err) {
    LOG(ERROR) << "[RedisClient] Redis GET failed for key '" << key
               << "': " << err.what();
    return std::nullopt;
  }
}

std::optional<std::string> RedisClient::HGet(const std::string &key,
                                             const std::string &field) const {
  if (!connected_.load()) {
    LOG(ERROR) << "[RedisClient] Redis not connected";
    return std::nullopt;
  }

  try {
    auto value = redis_->hget(key, field);
    return value ? std::optional<std::string>(value.value()) : std::nullopt;
  } catch (const sw::redis::Error &err) {
    LOG(ERROR) << "[RedisClient] Redis HGET failed for key '" << key
               << "' field '" << field << "': " << err.what();
    return std::nullopt;
  }
}

void RedisClient::LoadConfig_(const YAML::Node &redis_config) {
  try {
    connection_options_.host =
        redis_config["host"].as<std::string>("127.0.0.1");
    connection_options_.port = redis_config["port"].as<int>(6379);
    connection_options_.password = redis_config["password"].as<std::string>("");
    connection_options_.db = redis_config["db"].as<int>(0);

    // Set reasonable timeout defaults
    int timeout_ms = redis_config["timeout"].as<int>(5000);
    connection_options_.connect_timeout = std::chrono::milliseconds(timeout_ms);
    connection_options_.socket_timeout = std::chrono::milliseconds(timeout_ms);

    LOG(INFO) << "[RedisClient] Redis config loaded: "
              << connection_options_.host << ":" << connection_options_.port;
  } catch (const YAML::Exception &e) {
    LOG(ERROR) << "[RedisClient] Failed to load Redis config: " << e.what();
    throw std::runtime_error("Invalid Redis configuration");
  }
}

} // namespace client