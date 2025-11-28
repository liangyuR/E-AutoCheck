#include "client/mqtt_client.h"

#include <glog/logging.h>
#include <utility>

namespace client {

MqttConfig MqttConfig::FromYamlFile(const std::string &path) {
  YAML::Node root = YAML::LoadFile(path);
  MqttConfig config;

  const auto mqtt = root["mqtt"];
  if (!mqtt) {
    throw std::runtime_error("Missing 'mqtt' section in YAML");
  }

  if (mqtt["server_uri"]) {
    config.server_uri = mqtt["server_uri"].as<std::string>();
  }
  if (mqtt["client_id"]) {
    config.client_id = mqtt["client_id"].as<std::string>();
  }
  if (mqtt["username"]) {
    config.username = mqtt["username"].as<std::string>();
  }
  if (mqtt["password"]) {
    config.password = mqtt["password"].as<std::string>();
  }
  if (mqtt["keep_alive"]) {
    config.keep_alive = mqtt["keep_alive"].as<int>();
  }
  if (mqtt["clean_session"]) {
    config.clean_session = mqtt["clean_session"].as<bool>();
  }

  return config;
}

MqttClient::MqttClient(const MqttConfig &config) {
  server_uri_ = config.server_uri;
  client_id_ = config.client_id;
  conn_opts_.set_keep_alive_interval(config.keep_alive);
  conn_opts_.set_clean_session(config.clean_session);
  if (!config.username.empty()) {
    conn_opts_.set_user_name(config.username);
  }
  if (!config.password.empty()) {
    conn_opts_.set_password(config.password);
  }
  connectWithConfig_();
}

MqttClient::~MqttClient() { Disconnect(); }

bool MqttClient::Connect() {
  try {
    mqtt::token_ptr conntok = client_->connect(conn_opts_);
    conntok->wait();
    LOG(INFO) << "MQTT connected successfully";
    return true;
  } catch (const mqtt::exception &exc) {
    LOG(ERROR) << "MQTT connect error: " << exc.what();
    return false;
  }
}

void MqttClient::Disconnect() {
  try {
    client_->disconnect()->wait();
    LOG(INFO) << "MQTT disconnected";
  } catch (const mqtt::exception &exc) {
    LOG(ERROR) << "MQTT disconnect error: " << exc.what();
  }
}

void MqttClient::Publish(const std::string &topic, const std::string &payload,
                         int qos) {
  try {
    mqtt::message_ptr msg = mqtt::make_message(topic, payload, qos, false);
    client_->publish(msg)->wait();
    LOG(INFO) << "Published to " << topic << ": " << payload;
  } catch (const mqtt::exception &exc) {
    LOG(ERROR) << "MQTT publish error: " << exc.what();
  }
}

void MqttClient::Subscribe(const std::string &topic,
                           std::function<void(mqtt::const_message_ptr)> handler,
                           int qos) {
  try {
    client_->subscribe(topic, qos)->wait();
    client_->set_message_callback(std::move(handler));
    LOG(INFO) << "Subscribed to " << topic;
  } catch (const mqtt::exception &exc) {
    LOG(ERROR) << "MQTT subscribe error: " << exc.what();
  }
}

void MqttClient::connectWithConfig_() {
  client_ = std::make_unique<mqtt::async_client>(server_uri_, client_id_);
}

} // namespace client
