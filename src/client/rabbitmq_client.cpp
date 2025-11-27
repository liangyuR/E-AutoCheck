#include "client/rabbitmq_client.h"

#include <glog/logging.h>
#include <thread>

namespace client {

RabbitMqClient::RabbitMqClient(const std::string &config_path) {
  loadConfig_(config_path);
  setupEventLoop_();
}

RabbitMqClient::~RabbitMqClient() { Disconnect(); }

void RabbitMqClient::loadConfig_(const std::string &config_path) {
  try {
    YAML::Node config = YAML::LoadFile(config_path);
    auto rabbitmq_config = config["rabbitmq"];

    host_ = rabbitmq_config["host"].as<std::string>("127.0.0.1");
    port_ = rabbitmq_config["port"].as<int>(5672);
    username_ = rabbitmq_config["username"].as<std::string>("guest");
    password_ = rabbitmq_config["password"].as<std::string>("guest");
    vhost_ = rabbitmq_config["vhost"].as<std::string>("/");
    exchange_ = rabbitmq_config["exchange"].as<std::string>("");
    queue_name_ = rabbitmq_config["queue_name"].as<std::string>("test111");

    // 读取队列模板并替换 %s 占位符
    auto req_queue_pattern =
        rabbitmq_config["req_queue"].as<std::string>("%s.DataServer.req");
    auto resp_queue_pattern =
        rabbitmq_config["resp_queue"].as<std::string>("DataServer.%s.resp");

    // 替换 %s 为 queue_name
    size_t pos = req_queue_pattern.find("%s");
    if (pos != std::string::npos) {
      req_queue_ = req_queue_pattern.replace(pos, 2, queue_name_);
    } else {
      req_queue_ = req_queue_pattern;
    }

    pos = resp_queue_pattern.find("%s");
    if (pos != std::string::npos) {
      resp_queue_ = resp_queue_pattern.replace(pos, 2, queue_name_);
    } else {
      resp_queue_ = resp_queue_pattern;
    }

    LOG(INFO) << "RabbitMQ config loaded: " << host_ << ":" << port_
              << ", req_queue=" << req_queue_ << ", resp_queue=" << resp_queue_;
  } catch (const YAML::Exception &e) {
    LOG(ERROR) << "Failed to load RabbitMQ config: " << e.what();
    throw;
  }
}

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
    if (channel_ == nullptr) {
      LOG(ERROR) << "Channel not initialized";
      return;
    }

    message_handler_ = std::move(handler);

    // Start consuming from queue
    RabbitMqClient *self = this;
    channel_->consume(queue_name)
        .onReceived([self](const AMQP::Message &message, uint64_t deliveryTag,
                           bool redelivered) {
          std::string payload(message.body(), message.bodySize());
          LOG(INFO) << "Received message: " << payload;

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

} // namespace auto_charge::client
