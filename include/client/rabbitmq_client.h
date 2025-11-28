#pragma once

#include <amqpcpp.h>
#include <amqpcpp/libevent.h>
#include <atomic>
#include <event2/event.h>
#include <functional>
#include <future>
#include <memory>
#include <string>
#include <thread>
#include <yaml-cpp/yaml.h>

namespace client {

struct RabbitMqConfig {
  std::string host = "127.0.0.1";
  int port = 5672;
  std::string username = "guest";
  std::string password = "guest";
  std::string vhost = "/";
  std::string exchange;
  std::string queue_name = "test111";
  std::string req_queue;
  std::string resp_queue;

  static RabbitMqConfig FromYamlFile(const std::string &path);
};

class RabbitMqClient {
public:
  explicit RabbitMqClient(const RabbitMqConfig &config);
  ~RabbitMqClient();

  bool Connect();      // 非阻塞连接
  void ConnectAsync(); // 异步连接
  void Disconnect();

  // 连接状态检查
  bool IsConnected() const;

  // 等待连接完成
  bool WaitForConnection(
      std::chrono::milliseconds timeout = std::chrono::milliseconds(10000));

  void Publish(const std::string &routing_key, const std::string &payload);
  void Subscribe(const std::string &queue_name,
                 std::function<void(const std::string &)> handler);

private:
  void setupEventLoop_();
  bool ConnectInternal_(); // 内部连接实现

  // Configuration
  std::string host_;
  int port_{5672};
  std::string username_{"guest"};
  std::string password_{"guest"};
  std::string vhost_{"/"};
  std::string exchange_;
  std::string queue_name_;
  std::string req_queue_;
  std::string resp_queue_;

  // AMQP connection objects
  struct event_base *event_base_{nullptr};
  AMQP::LibEventHandler *handler_{nullptr};
  AMQP::TcpConnection *connection_{nullptr};
  AMQP::TcpChannel *channel_{nullptr};

  // Connection state
  std::atomic<bool> connected_{false};
  std::atomic<bool> connecting_{false};
  std::mutex connection_mutex_;
  std::condition_variable connection_cv_;
  std::string connection_error_;
  std::future<bool> connection_future_;

  // Message handler
  std::function<void(const std::string &)> message_handler_;
};
} // namespace client
