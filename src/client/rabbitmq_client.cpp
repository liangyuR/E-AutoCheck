#include "client/rabbitmq_client.h"

#include <glog/logging.h>
#include <thread>

namespace client {

RabbitMqConfig RabbitMqConfig::FromYamlFile(const std::string &path) {
  YAML::Node root = YAML::LoadFile(path);
  RabbitMqConfig config;

  const auto rabbitmq = root["rabbitmq"];
  if (!rabbitmq) {
    throw std::runtime_error("Missing 'rabbitmq' section in YAML");
  }

  if (rabbitmq["host"]) {
    config.host = rabbitmq["host"].as<std::string>();
  }
  if (rabbitmq["port"]) {
    config.port = rabbitmq["port"].as<int>();
  }
  if (rabbitmq["username"]) {
    config.username = rabbitmq["username"].as<std::string>();
  }
  if (rabbitmq["password"]) {
    config.password = rabbitmq["password"].as<std::string>();
  }
  if (rabbitmq["vhost"]) {
    config.vhost = rabbitmq["vhost"].as<std::string>();
  }
  if (rabbitmq["exchange"]) {
    config.exchange = rabbitmq["exchange"].as<std::string>();
  }
  if (rabbitmq["queue_name"]) {
    config.queue_name = rabbitmq["queue_name"].as<std::string>();
  }

  // 读取队列模板并替换 %s 占位符
  std::string req_queue_pattern = "%s.DataServer.req";
  std::string resp_queue_pattern = "DataServer.%s.resp";
  if (rabbitmq["req_queue"]) {
    req_queue_pattern = rabbitmq["req_queue"].as<std::string>();
  }
  if (rabbitmq["resp_queue"]) {
    resp_queue_pattern = rabbitmq["resp_queue"].as<std::string>();
  }

  // 替换 %s 为 queue_name
  size_t pos = req_queue_pattern.find("%s");
  if (pos != std::string::npos) {
    config.req_queue = req_queue_pattern.replace(pos, 2, config.queue_name);
  } else {
    config.req_queue = req_queue_pattern;
  }

  pos = resp_queue_pattern.find("%s");
  if (pos != std::string::npos) {
    config.resp_queue = resp_queue_pattern.replace(pos, 2, config.queue_name);
  } else {
    config.resp_queue = resp_queue_pattern;
  }

  return config;
}

std::unique_ptr<RabbitMqClient> RabbitMqClient::instance_;
std::mutex RabbitMqClient::instance_mutex_;

void RabbitMqClient::Init(const RabbitMqConfig &config) {
  std::lock_guard<std::mutex> lock(instance_mutex_);
  if (!instance_) {
    instance_.reset(new RabbitMqClient(config));
  }
}

RabbitMqClient *RabbitMqClient::GetInstance() { return instance_.get(); }

RabbitMqClient::RabbitMqClient(const RabbitMqConfig &config) {
  host_ = config.host;
  port_ = config.port;
  username_ = config.username;
  password_ = config.password;
  vhost_ = config.vhost;
  exchange_ = config.exchange;
  queue_name_ = config.queue_name;
  req_queue_ = config.req_queue;
  resp_queue_ = config.resp_queue;
  setupEventLoop_();
}

RabbitMqClient::~RabbitMqClient() { Disconnect(); }

void RabbitMqClient::setupEventLoop_() {
  event_base_ = event_base_new();
  if (event_base_ == nullptr) {
    LOG(ERROR) << "Failed to create event base";
    throw std::runtime_error("Failed to create event base");
  }
}

bool RabbitMqClient::Connect() {
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

void RabbitMqClient::ConnectAsync() {
  if (connected_.load() || connecting_.load()) {
    return;
  }

  connecting_.store(true);

  // 启动异步连接任务
  connection_future_ =
      std::async(std::launch::async, [this]() { return ConnectInternal_(); });
}

bool RabbitMqClient::IsConnected() const { return connected_.load(); }

bool RabbitMqClient::WaitForConnection(std::chrono::milliseconds timeout) {
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
    LOG(ERROR) << "RabbitMQ connection timeout after " << timeout.count()
               << " ms";
    return false;
  }

  return false;
}

bool RabbitMqClient::ConnectInternal_() {
  try {
    // Reset connection state
    {
      std::lock_guard<std::mutex> lock(connection_mutex_);
      connected_.store(false);
      connection_error_.clear();
    }

    // Create LibEvent handler
    handler_ = new AMQP::LibEventHandler(event_base_);

    // Create connection with credentials
    AMQP::Address address(host_, port_, AMQP::Login(username_, password_),
                          vhost_);
    connection_ = new AMQP::TcpConnection(handler_, address);

    // Create channel
    channel_ = new AMQP::TcpChannel(connection_);

    // Set channel callbacks to detect connection state
    channel_->onReady([this]() {
      std::lock_guard<std::mutex> lock(connection_mutex_);
      connected_.store(true);
      connection_cv_.notify_all();
      LOG(INFO) << "RabbitMQ connected successfully";
    });

    channel_->onError([this](const char *message) {
      std::lock_guard<std::mutex> lock(connection_mutex_);
      connected_.store(false);
      connection_error_ = message;
      connection_cv_.notify_all();
      LOG(ERROR) << "RabbitMQ connection error: " << message;
    });

    // Declare exchange if specified
    if (!exchange_.empty()) {
      // MuskEx is a topic exchange on the server (non-durable), declare it
      // accordingly
      channel_->declareExchange(exchange_, AMQP::topic);
      LOG(INFO) << "Declared topic exchange: " << exchange_;
    }

    // Declare queues (try durable first, fallback to non-durable if needed)
    try {
      channel_->declareQueue(req_queue_, AMQP::durable);
      channel_->declareQueue(resp_queue_, AMQP::durable);
      LOG(INFO) << "Declared durable queues";
    } catch (const std::exception &e) {
      LOG(WARNING) << "Failed to declare durable queues, trying non-durable: "
                   << e.what();
      channel_->declareQueue(req_queue_);
      channel_->declareQueue(resp_queue_);
      LOG(INFO) << "Declared non-durable queues";
    }

    // Bind queues to exchange if exchange is specified
    if (!exchange_.empty()) {
      // For topic exchange, use queue name as routing key pattern
      channel_->bindQueue(exchange_, req_queue_, req_queue_);
      channel_->bindQueue(exchange_, resp_queue_, resp_queue_);
      LOG(INFO) << "Bound queues to exchange " << exchange_;
    }

    // Start event loop in a separate thread
    std::thread([this]() { event_base_dispatch(event_base_); }).detach();

    // Wait for connection to be established or fail
    {
      std::unique_lock<std::mutex> lock(connection_mutex_);
      if (!connection_cv_.wait_for(lock, std::chrono::seconds(10), [this]() {
            return connected_.load() || !connection_error_.empty();
          })) {
        LOG(ERROR) << "RabbitMQ connection timeout";
        return false;
      }

      if (!connected_.load()) {
        LOG(ERROR) << "RabbitMQ connection failed: " << connection_error_;
        return false;
      }
    }

    return true;
  } catch (const std::exception &e) {
    LOG(ERROR) << "RabbitMQ connect error: " << e.what();
    return false;
  }
}

void RabbitMqClient::Disconnect() {
  try {
    connected_.store(false);
    connecting_.store(false);

    if (channel_ != nullptr) {
      delete channel_;
      channel_ = nullptr;
    }
    if (connection_ != nullptr) {
      delete connection_;
      connection_ = nullptr;
    }
    if (handler_ != nullptr) {
      delete handler_;
      handler_ = nullptr;
    }
    if (event_base_ != nullptr) {
      event_base_loopbreak(event_base_);
      event_base_free(event_base_);
      event_base_ = nullptr;
    }
    LOG(INFO) << "RabbitMQ disconnected";
  } catch (const std::exception &e) {
    LOG(ERROR) << "RabbitMQ disconnect error: " << e.what();
  }
}

void RabbitMqClient::Publish(const std::string &routing_key,
                             const std::string &payload) {
  try {
    if (!Connect()) {
      LOG(ERROR) << "Failed to connect to RabbitMQ";
      return;
    }

    if (channel_ == nullptr) {
      LOG(ERROR) << "Channel not initialized";
      return;
    }

    // Publish message with persistent delivery mode
    AMQP::Envelope envelope(payload.c_str(), payload.size());
    envelope.setDeliveryMode(2); // Persistent

    channel_->publish(exchange_, routing_key, envelope);

    LOG(INFO) << "Published to queue " << routing_key << ": " << payload;
  } catch (const std::exception &e) {
    LOG(ERROR) << "RabbitMQ publish error: " << e.what();
  }
}

void RabbitMqClient::Subscribe(
    const std::string &queue_name,
    std::function<void(const std::string &)> handler) {
  try {
    if (!Connect()) {
      LOG(ERROR) << "Failed to connect to RabbitMQ";
      return;
    }

    if (channel_ == nullptr) {
      LOG(ERROR) << "Channel not initialized";
      return;
    }

    message_handler_ = std::move(handler);

    // Declare queue if it doesn't exist
    try {
      channel_->declareQueue(queue_name, AMQP::durable);
      LOG(INFO) << "Declared queue: " << queue_name;
    } catch (const std::exception &e) {
      LOG(WARNING) << "Failed to declare durable queue, trying non-durable: "
                   << e.what();
      channel_->declareQueue(queue_name);
      LOG(INFO) << "Declared non-durable queue: " << queue_name;
    }

    // Bind queue to exchange if exchange is specified
    if (!exchange_.empty()) {
      // For topic exchange, use queue name as routing key pattern
      channel_->bindQueue(exchange_, queue_name, queue_name);
      LOG(INFO) << "Bound queue " << queue_name << " to exchange " << exchange_;
    }

    // Start consuming from queue
    RabbitMqClient *self = this;
    channel_->consume(queue_name)
        .onReceived([self](const AMQP::Message &message, uint64_t deliveryTag,
                           bool redelivered) {
          std::string payload(message.body(), message.bodySize());
          DLOG(INFO) << "Received message: " << payload;

          if (self->message_handler_) {
            self->message_handler_(payload);
          }

          // Acknowledge message
          self->channel_->ack(deliveryTag);
        })
        .onError([](const char *error_msg) {
          LOG(ERROR) << "Consumer error: " << error_msg;
        });

    LOG(INFO) << "Subscribed to queue: " << queue_name;
  } catch (const std::exception &e) {
    LOG(ERROR) << "RabbitMQ subscribe error: " << e.what();
  }
}

} // namespace client
