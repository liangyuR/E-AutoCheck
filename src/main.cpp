#include <QGuiApplication>
#include <QQmlApplicationEngine>

#include "client/http_client.h"
#include "client/mqtt_client.h"
#include "client/mysql_client.h"
#include "client/rabbitmq_client.h"
#include "utils/log_init.h"

#include <absl/status/status.h>
#include <glog/logging.h>

absl::Status InitClient() {
  db::MySqlClient mysqlClient(db::Config::FromYamlFile("config/base.yaml"));
  client::MqttClient mqttClient(
      client::MqttConfig::FromYamlFile("config/base.yaml"));
  client::RabbitMqClient rabbitMqClient(
      client::RabbitMqConfig::FromYamlFile("config/base.yaml"));
  client::HttpClient httpClient(
      client::HttpConfig::FromYamlFile("config/base.yaml"));
  return absl::OkStatus();
}

int main(int argc, char *argv[]) {
  QGuiApplication app(argc, argv);
  QQmlApplicationEngine engine;

  if (auto status = InitLogging(); !status.ok()) {
    LOG(ERROR) << "初始化日志失败: " << status.message();
    return -1;
  }

  if (auto status = InitClient(); !status.ok()) {
    LOG(ERROR) << "初始化客户端失败: " << status.message();
    return -1;
  }

  engine.loadFromModule("GUI", "Main");

  if (engine.rootObjects().isEmpty())
    return -1;

  return QGuiApplication::exec();
}
