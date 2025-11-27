#pragma once

#include <functional>
#include <memory>
#include <mqtt/async_client.h>
#include <string>
#include <yaml-cpp/yaml.h>

namespace client {
class MqttClient {
public:
  MqttClient(const std::string &config_path);
  ~MqttClient();

  bool Connect();
  void Disconnect();

  void Publish(const std::string &topic, const std::string &payload,
               int qos = 1);
  void Subscribe(const std::string &topic,
                 std::function<void(mqtt::const_message_ptr)> handler,
                 int qos = 1);

private:
  void loadConfig_(const std::string &config_path);
  void connectWithConfig_();

  std::string server_uri_;
  std::string client_id_;
  mqtt::connect_options conn_opts_;
  std::unique_ptr<mqtt::async_client> client_;
};

} // namespace auto_charge::client
