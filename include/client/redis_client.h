#pragma once

#include <atomic>
#include <future>
#include <glog/logging.h>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <sw/redis++/redis++.h>
#include <thread>
#include <yaml-cpp/yaml.h>

namespace client {
class RedisClient {
public:
  // Singleton management
  static void Init(const YAML::Node &redis_config);
  static RedisClient *GetInstance();

  // Delete copy constructor and assignment
  RedisClient(const RedisClient &) = delete;
  RedisClient &operator=(const RedisClient &) = delete;

  // Move constructor and assignment - 由于包含 std::future，需要手动实现
  // Singleton usually doesn't need move, but we can keep or delete.
  // Given it's a singleton held by unique_ptr, we generally don't move the
  // instance around. But let's delete move as well for strict singleton.
  RedisClient(RedisClient &&) = delete;
  RedisClient &operator=(RedisClient &&) = delete;

  ~RedisClient();

  // Connection management
  bool IsConnected() const;
  bool Connect();      // 非阻塞连接
  void ConnectAsync(); // 异步连接
  void Disconnect();

  // 等待连接完成
  bool WaitForConnection(
      std::chrono::milliseconds timeout = std::chrono::milliseconds(5000));

  // Redis operations with proper error handling
  bool Set(const std::string &key, const std::string &value);
  std::optional<std::string> Get(const std::string &key) const;
  std::optional<std::string> HGet(const std::string &key,
                                  const std::string &field) const;

private:
  explicit RedisClient(const YAML::Node &redis_config);

  void LoadConfig_(const YAML::Node &redis_config);
  bool ConnectInternal_(); // 内部连接实现

  sw::redis::ConnectionOptions connection_options_;
  std::unique_ptr<sw::redis::Redis> redis_;
  std::atomic<bool> connected_{false};
  std::atomic<bool> connecting_{false};
  std::future<bool> connection_future_;

  static std::unique_ptr<RedisClient> instance_;
  static std::mutex instance_mutex_;
};

} // namespace client