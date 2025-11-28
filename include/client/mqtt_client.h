#pragma once

#include <functional>
#include <memory>
#include <mqtt/async_client.h>
#include <string>
#include <yaml-cpp/yaml.h>

namespace client {

struct MqttConfig {
  std::string server_uri = "tcp://localhost:1883";
  std::string client_id = "auto_charge_client";
  std::string username;
  std::string password;
  int keep_alive = 20;
  bool clean_session = true;

  static MqttConfig FromYamlFile(const std::string &path);
};

class MqttClient {
public:
  explicit MqttClient(const MqttConfig &config);
  ~MqttClient();

  bool Connect();
  void Disconnect();

  void Publish(const std::string &topic, const std::string &payload,
               int qos = 1);
  void Subscribe(const std::string &topic,
                 std::function<void(mqtt::const_message_ptr)> handler,
                 int qos = 1);

private:
  void connectWithConfig_();

  std::string server_uri_;
  std::string client_id_;
  mqtt::connect_options conn_opts_;
  std::unique_ptr<mqtt::async_client> client_;
};

} // namespace client
