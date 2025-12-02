#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlEngine>
#include <QQuickStyle>

#include "client/http_client.h"
#include "client/mysql_client.h"
#include "client/rabbitmq_client.h"
#include "client/redis_client.h"
#include "self_check_manager.h"
#include "ui/self_check_controller.h"
#include "utils/log_init.h"

#include <absl/status/status.h>
#include <glog/logging.h>
#include <yaml-cpp/yaml.h>

absl::Status InitClient() {
  try {
    // MySQL
    auto mysqlConfig = db::Config::FromYamlFile("config/base.yaml");
    db::MySqlClient::Init(mysqlConfig);

    // RabbitMQ
    auto rabbitConfig =
        client::RabbitMqConfig::FromYamlFile("config/base.yaml");
    client::RabbitMqClient::Init(rabbitConfig);

    // Redis
    YAML::Node root = YAML::LoadFile("config/base.yaml");
    if (root["redis"]) {
      client::RedisClient::Init(root["redis"]);
    } else {
      LOG(WARNING) << "Redis config not found in config/base.yaml";
    }

    // HttpClient (TODO: Convert to singleton or manage lifecycle)
    client::HttpClient httpClient(
        client::HttpConfig::FromYamlFile("config/base.yaml"));

  } catch (const std::exception &e) {
    return absl::InternalError(e.what());
  }
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

  auto *selfCheckManager = new selfcheck::SelfCheckManager(&app);
  auto *selfCheckController =
      new ui::SelfCheckController(selfCheckManager, &app);
  qmlRegisterSingletonInstance("EAutoCheck", 1, 0, "SelfCheck",
                               selfCheckController);

  QQuickStyle::setStyle("Material");
  engine.loadFromModule("GUI", "Main");

  if (engine.rootObjects().isEmpty())
    return -1;

  return QGuiApplication::exec();
}
