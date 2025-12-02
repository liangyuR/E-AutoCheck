#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlEngine>
#include <QQuickStyle>

#include "client/http_client.h"
#include "client/mysql_client.h"
#include "client/rabbitmq_client.h"
#include "client/redis_client.h"
#include "device/device_manager.h"
#include "device/device_repo.h"
#include "utils/log_init.h"

#include <absl/status/status.h>
#include <glog/logging.h>
#include <yaml-cpp/yaml.h>

absl::Status InitClient() {
  try {
    // MySQL
    auto mysqlConfig = db::Config::FromYamlFile("config/base.yaml");
    db::MySqlClient::Init(mysqlConfig);

    // // RabbitMQ
    // auto rabbitConfig =
    //     client::RabbitMqConfig::FromYamlFile("config/base.yaml");
    // client::RabbitMqClient::Init(rabbitConfig);

    // // Redis
    // YAML::Node root = YAML::LoadFile("config/base.yaml");
    // if (root["redis"]) {
    //   client::RedisClient::Init(root["redis"]);
    // } else {
    //   LOG(WARNING) << "Redis config not found in config/base.yaml";
    // }

    // // HttpClient (TODO: Convert to singleton or manage lifecycle)
    // client::HttpClient httpClient(
    //     client::HttpConfig::FromYamlFile("config/base.yaml"));

  } catch (const std::exception &e) {
    return absl::InternalError(e.what());
  }
  return absl::OkStatus();
}

void AsyncLoadDevices(device::DeviceManager *device_manager) {
  std::thread([device_manager]() {
    LOG(INFO) << "开始后台加载...";

    if (auto status = InitClient(); !status.ok()) {
      LOG(ERROR) << "初始化客户端失败: " << status.message();
      return;
    }

    auto devices_result = device::DeviceRepo::GetAllPipeDevices();
    if (!devices_result.ok()) {
      LOG(ERROR) << "加载设备失败: " << devices_result.status().message();
      // 继续运行，不加载设备
    } else {
      const auto &devices = devices_result.value();
      LOG(INFO) << "加载设备成功，共 " << devices.size() << " 个";

      // 回到主线程添加设备，确保线程安全和 UI 更新
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

  QQuickStyle::setStyle("Material");
  engine.loadFromModule("GUI", "Main");

  if (engine.rootObjects().isEmpty())
    return -1;

  AsyncLoadDevices(device_manager);

  return QGuiApplication::exec();
}
