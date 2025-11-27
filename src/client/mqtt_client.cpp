#include "client/mqtt_client.h"

#include <glog/logging.h>
#include <utility>

namespace client {

MqttClient::MqttClient(const std::string &config_path) {
  loadConfig_(config_path);
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

void MqttClient::loadConfig_(const std::string &config_path) {
  try {
    YAML::Node config = YAML::LoadFile(config_path);
    server_uri_ = config["server_uri"].as<std::string>("tcp://localhost:1883");
    client_id_ = config["client_id"].as<std::string>("auto_charge_client");
    conn_opts_.set_keep_alive_interval(20);
    conn_opts_.set_clean_session(true);
    // Add more options like username/password if needed
    LOG(INFO) << "MQTT config loaded from " << config_path;
  } catch (const YAML::Exception &e) {
    LOG(ERROR) << "Failed to load MQTT config: " << e.what();
    throw;
  }
}

void MqttClient::connectWithConfig_() {
  client_ = std::make_unique<mqtt::async_client>(server_uri_, client_id_);
}

} // namespace auto_charge::client
