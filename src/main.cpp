#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlEngine>
#include <QQuickStyle>

#include "check_manager.h"
#include "client/mysql_client.h"
#include "client/rabbitmq_client.h"
#include "client/redis_client.h"
#include "db/db_table.h"
#include "device/device_repo.h"
#include "model/device_model.h"
#include "model/history_model.h"
#include "model/pile_model.h"
#include "utils/log_init.h"
#include "watcher/online_status_watcher.h"

#include <absl/status/status.h>
#include <glog/logging.h>
#include <yaml-cpp/yaml.h>

#include <chrono>
#include <iomanip>
#include <sstream>

absl::Status InitClient() {
  auto mysqlConfig = db::Config::FromYamlFile("config/base.yaml");
  db::MySqlClient::Init(mysqlConfig);

  if (auto status = db::CreateSelfCheckRecordTable(); !status.ok()) {
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

// 加载设备的最后检测信息
void LoadLatestCheckInfo(qml_model::DeviceModel *device_model,
                         const std::vector<device::PileAttr> &devices) {
  for (const auto &device_attr : devices) {
    const auto &equip_no = device_attr.equip_no;
    auto result =
        device::DeviceRepo::GetLatestPileItems(QString::fromStdString(equip_no));

    if (!result.ok()) {
      DLOG(WARNING) << "获取设备最后检测信息失败: " << equip_no << ", "
                    << result.status().message();
      continue;
    }

    const auto &ccu_items = result.value();
    if (ccu_items.empty()) {
      DLOG(INFO) << "设备无历史检测记录: " << equip_no;
      continue;
    }

    // 构建 SelfCheckResult，主要用于更新最后检测时间
    device::SelfCheckResult check_result;
    check_result.last_check_time_str = ccu_items.front().last_check_time;
    check_result.status = device::SelfCheckStatus::Passed; // 有记录说明已完成

    // 统计成功/失败数量（简单判断：任何异常状态都算失败）
    int fail_count = 0;
    for (const auto &ccu : ccu_items) {
      // 检查各模块是否有异常
      if (ccu.ac_contactor_1.contactor1_stuck ||
          ccu.ac_contactor_1.contactor1_refuse ||
          ccu.ac_contactor_2.contactor1_stuck ||
          ccu.ac_contactor_2.contactor1_refuse ||
          ccu.parallel_contactor.positive_stuck ||
          ccu.parallel_contactor.positive_refuse ||
          ccu.parallel_contactor.negative_stuck ||
          ccu.parallel_contactor.negative_refuse) {
        ++fail_count;
      }
    }
    check_result.fail_count = fail_count;
    check_result.success_count =
        static_cast<int>(ccu_items.size()) - fail_count;
    if (fail_count > 0) {
      check_result.status = device::SelfCheckStatus::Failed;
    }

    // 在主线程更新 Model
    QMetaObject::invokeMethod(
        device_model,
        [device_model, equip_no, check_result]() {
          device_model->updateSelfCheck(equip_no, check_result);
        },
        Qt::QueuedConnection);

    DLOG(INFO) << "已加载设备最后检测信息: " << equip_no
               << ", 时间: " << check_result.last_check_time_str;
  }
}

void AsyncLoadDevices(qml_model::DeviceModel *device_model,
                      EAutoCheck::CheckManager *check_manager,
                      watcher::OnlineStatusWatcher *online_watcher) {
  std::thread([device_model, check_manager, online_watcher]() {
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
      return;
    }

    const auto &devices = devices_result.value();
    LOG(INFO) << "加载设备成功，共 " << devices.size() << " 个";

    // 先将设备添加到 Model
    QMetaObject::invokeMethod(device_model,
                              [device_model, devices, online_watcher]() {
                                for (const auto &device : devices) {
                                  device_model->addDevice(device);
                                }
                                LOG(INFO) << "已将设备添加到管理器";

                                // 设备加载完成后，启动在线状态监控
                                if (online_watcher != nullptr) {
                                  online_watcher->start();
                                }
                              });

    // 在后台线程加载每个设备的最后检测信息
    LoadLatestCheckInfo(device_model, devices);
    LOG(INFO) << "已加载所有设备的最后检测信息";
  }).detach();
}

int main(int argc, char *argv[]) {
  QGuiApplication app(argc, argv);
  QQmlApplicationEngine engine;

  if (auto status = InitLogging(); !status.ok()) {
    LOG(ERROR) << "初始化日志失败: " << status.message();
    return -1;
  }

  auto *device_model = new qml_model::DeviceModel(&app);
  qmlRegisterSingletonInstance("EAutoCheck", 1, 0, "DeviceModel", device_model);

  auto *check_manager = new EAutoCheck::CheckManager(device_model, &app);
  qmlRegisterSingletonInstance("EAutoCheck", 1, 0, "CheckManager",
                               check_manager);

  auto *history_model = new qml_model::HistoryModel(&app);
  qmlRegisterSingletonInstance("EAutoCheck", 1, 0, "HistoryModel",
                               history_model);

  auto *pile_model = new qml_model::PileModel(&app);
  qmlRegisterSingletonInstance("EAutoCheck", 1, 0, "PileModel", pile_model);

  // 创建在线状态监控器，默认 5 秒轮询一次
  auto *online_watcher = new watcher::OnlineStatusWatcher(device_model, &app);
  online_watcher->setPollIntervalMs(5000);
  qmlRegisterSingletonInstance("EAutoCheck", 1, 0, "OnlineStatusWatcher",
                               online_watcher);

  QQuickStyle::setStyle("Material");
  engine.loadFromModule("GUI", "Main");

  if (engine.rootObjects().isEmpty())
    return -1;

  AsyncLoadDevices(device_model, check_manager, online_watcher);

  int ret = QGuiApplication::exec();
  db::MySqlClient::Shutdown();
  return ret;
}
