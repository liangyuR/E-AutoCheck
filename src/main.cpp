#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlEngine>
#include <QQuickStyle>

#include "check_manager.h"
#include "client/mysql_client.h"
#include "client/rabbitmq_client.h"
#include "client/redis_client.h"
#include "db/migration.h"
#include "device/device_manager.h"
#include "device/device_repo.h"
#include "model/history_model.h"
#include "model/pile_model.h"
#include "utils/log_init.h"

#include <absl/status/status.h>
#include <glog/logging.h>
#include <yaml-cpp/yaml.h>

#include <chrono>
#include <iomanip>
#include <sstream>

absl::Status InitClient() {
  auto mysqlConfig = db::Config::FromYamlFile("config/base.yaml");
  db::MySqlClient::Init(mysqlConfig);

  if (auto status = db::EnsureTablesExist(); !status.ok()) {
    LOG(ERROR) << "数据库表初始化失败: " << status.message();
    return status;
  }

  auto rabbitConfig = client::RabbitMqConfig::FromYamlFile("config/base.yaml");
  client::RabbitMqClient::Init(rabbitConfig);

  YAML::Node root = YAML::LoadFile("config/base.yaml");
  client::RedisClient::Init(root["redis"]);
  client::RedisClient::GetInstance()->Connect();

  return absl::OkStatus();
}

void AsyncLoadDevices(device::DeviceManager *device_manager,
                      EAutoCheck::CheckManager *check_manager) {
  std::thread([device_manager, check_manager]() {
    if (auto status = InitClient(); !status.ok()) {
      LOG(ERROR) << "初始化客户端失败: " << status.message();
      return;
    }

    if (auto status = check_manager->SubMsg(); !status.ok()) {
      LOG(ERROR) << "订阅自检消息失败: " << status.message();
      return;
    }

    auto devices_result = device::DeviceRepo::GetAllPipeDevices();
    if (!devices_result.ok()) {
      LOG(ERROR) << "加载设备失败: " << devices_result.status().message();
    } else {
      const auto &devices = devices_result.value();
      LOG(INFO) << "加载设备成功，共 " << devices.size() << " 个";

      QMetaObject::invokeMethod(device_manager, [device_manager, devices]() {
        for (const auto &device : devices) {
          device_manager->addDevice(device);
        }
        LOG(INFO) << "已将设备添加到管理器";
      });
    }
  }).detach();
}

int main(int argc, char *argv[]) {
  QGuiApplication app(argc, argv);
  QQmlApplicationEngine engine;

  if (auto status = InitLogging(); !status.ok()) {
    LOG(ERROR) << "初始化日志失败: " << status.message();
    return -1;
  }

  auto *device_manager = new device::DeviceManager(&app);
  qmlRegisterSingletonInstance("EAutoCheck", 1, 0, "DeviceManager",
                               device_manager);

  auto *check_manager = new EAutoCheck::CheckManager(device_manager, &app);
  qmlRegisterSingletonInstance("EAutoCheck", 1, 0, "CheckManager",
                               check_manager);

  auto *history_model = new qml_model::HistoryModel(&app);
  qmlRegisterSingletonInstance("EAutoCheck", 1, 0, "HistoryModel",
                               history_model);

  auto *pile_model = new qml_model::PileModel(&app);
  qmlRegisterSingletonInstance("EAutoCheck", 1, 0, "PileModel", pile_model);

  QQuickStyle::setStyle("Material");
  engine.loadFromModule("GUI", "Main");

  if (engine.rootObjects().isEmpty())
    return -1;

  AsyncLoadDevices(device_manager, check_manager);

  int ret = QGuiApplication::exec();
  db::MySqlClient::Shutdown();
  return ret;
}
