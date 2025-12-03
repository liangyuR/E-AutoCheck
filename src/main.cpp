#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlEngine>
#include <QQuickStyle>

#include "check_manager.h"
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

    // RabbitMQ
    auto rabbitConfig =
        client::RabbitMqConfig::FromYamlFile("config/base.yaml");
    client::RabbitMqClient::Init(rabbitConfig);

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

void AsyncLoadDevices(device::DeviceManager *device_manager,
                      EAutoCheck::CheckManager *check_manager) {
  std::thread([device_manager, check_manager]() {
    LOG(INFO) << "开始后台加载...";

    if (auto status = InitClient(); !status.ok()) {
      LOG(ERROR) << "初始化客户端失败: " << status.message();
      return;
    }

    if (auto status = check_manager->Subscribe(); !status.ok()) {
      LOG(ERROR) << "订阅自检消息失败: " << status.message();
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
          LOG(INFO) << "添加设备: name=" << device.name
                    << ", id=" << device.equip_no;
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

  auto *check_manager = new EAutoCheck::CheckManager(&app);
  qmlRegisterSingletonInstance("EAutoCheck", 1, 0, "CheckManager",
                               check_manager);

  // 连接自检进度信号到设备管理器
  QObject::connect(check_manager, &EAutoCheck::CheckManager::checkStatusUpdated,
                   device_manager,
                   [device_manager](const QString &pileId, const QString &desc,
                                    int result, bool isFinished) {
                     std::string equip_no = pileId.toStdString();
                     std::string desc_str = desc.toStdString();
                     // isFinished 为 true 时，is_checking 应为 false
                     bool is_checking = !isFinished;
                     device_manager->updateSelfCheckProgress(equip_no, desc_str,
                                                             is_checking);
                   });

  QQuickStyle::setStyle("Material");
  engine.loadFromModule("GUI", "Main");

  if (engine.rootObjects().isEmpty())
    return -1;

  AsyncLoadDevices(device_manager, check_manager);

  int ret = QGuiApplication::exec();
  db::MySqlClient::Shutdown();
  return ret;
}
