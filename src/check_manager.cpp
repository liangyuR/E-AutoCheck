#include "check_manager.h"

#include <QDateTime>
#include <QTimer>
#include <QUuid>
#include <absl/status/status.h>
#include <array>
#include <glog/logging.h>
#include <nlohmann/json.hpp>
#include <random>
#include <unordered_map>
#include <vector>

#include "client/mysql_client.h"
#include "client/rabbitmq_client.h"
#include "client/redis_client.h"
#include "device/device_manager.h"
#include "device/device_object.h"
#include "utils/convert.h"

namespace EAutoCheck {
CheckManager::CheckManager(device::DeviceManager *device_manager,
                           QObject *parent)
    : QObject(parent), device_manager_(device_manager) {}

CheckManager::~CheckManager() = default;

absl::Status CheckManager::SubMsg() {
  auto *rabbit_client = client::RabbitMqClient::GetInstance();
  if (rabbit_client == nullptr) {
    return absl::InternalError("Failed to get RabbitMQ client");
  }

  rabbit_client->Subscribe(
      "DataServer.AutoCheck.notice",
      [this](const std::string &message) { recvMsg(message); });
  rabbit_client->Subscribe(
      "DataServer.AutoCheck.resp",
      [this](const std::string &message) { recvMsg(message); });

  return absl::OkStatus();
}

absl::Status CheckManager::CheckDevice(const QString &equip_no) {
  auto *rabbit_client = client::RabbitMqClient::GetInstance();
  if (rabbit_client == nullptr) {
    return absl::InternalError("Failed to get RabbitMQ client");
  }

  LOG(INFO) << "设备 " << equip_no.toStdString() << " 自检开始";

  // 自增索引
  // TODO(@liangyu), 读 Mysql 自检测表的 ID 最大值 + 1
  uint64_t index = sequence_index_.fetch_add(1);

  // 构建 JSON
  nlohmann::json j; // NOLINT
  j["ReqType"] = 1010;
  j["Index"] = index;
  j["Content"] = {{"pileId", equip_no.toStdString()}};

  std::string payload = j.dump();

  // Routing Key: {appName}.{foreName}.req
  std::string routing_key = "AutoCheck.DataServer.req"; // TODO(@liangyu) fixme
  rabbit_client->Publish(routing_key, payload);
  DLOG(INFO) << "json: " << j.dump();

  return absl::OkStatus();
}

void CheckManager::recvMsg(const std::string &message) {
  try {
    nlohmann::json j = nlohmann::json::parse(message); // NOLINT
    if (!j.contains("Data")) {
      LOG(WARNING) << "收到非法消息，缺少 Data 字段";
      return;
    }

    // TODO(@liangyu) 需要兼容不同设备，比如换电站可能不是 pileId
    auto data = j["Data"];
    if (!data.contains("pileId")) {
      LOG(WARNING) << "收到非法消息，缺少 pileId 字段";
      return;
    }

    std::string pile_id = data["pileId"].get<std::string>();
    std::string desc = data.value("desc", "");
    std::string state = data.value("state", "");
    int result = 0;
    bool is_finished = false;
    bool is_checking = false;

    // 处理进度通知格式 (NoticeType)
    if (j.contains("NoticeType")) {
      int notice_type = j["NoticeType"].get<int>();
      if (notice_type != 1010) {
        return; // 只处理自检相关的通知
      }

      // 根据 state 判断状态
      if (state == "CMD_SENT") {
        // 指令已下发，等待启动
        is_checking = false;
        is_finished = false;
      } else if (state == "STARTED") {
        // 自检启动成功
        is_checking = true;
        is_finished = false;
        result = data.value("result", 0);
      } else if (state == "COMPLETED") {
        // 自检完成
        is_checking = false;
        is_finished = true;
        result = data.value("result", 0);
      } else {
        LOG(WARNING) << "未知状态: " << state;
        return;
      }

      DLOG(INFO) << "自检进度: pileId=" << pile_id << " 描述=" << desc
                 << " 状态=" << state << " 结果=" << result
                 << " 检测中=" << is_checking << " 已完成=" << is_finished;

      if (device_manager_ != nullptr) {
        device_manager_->updateSelfCheckProgress(pile_id, desc, is_checking);
      }
    }
    // 处理最终结果格式 (ReqType)
    else if (j.contains("ReqType") && j.contains("Result")) {
      int req_type = j["ReqType"].get<int>();
      if (req_type != 1010) {
        return; // 只处理自检相关的请求
      }

      result = j["Result"].get<int>();
      int success_count = data.value("successCount", 0);
      int fail_count = data.value("failCount", 0);
      int code = data.value("code", 0);

      // 最终结果，自检已完成
      is_checking = false;
      is_finished = true;

      if (auto status = saveResultToMysql(pile_id); !status.ok()) {
        LOG(ERROR) << "保存自检结果到 MySQL 失败: " << status.message();
        return;
      }

      DLOG(INFO) << "自检最终结果: pileId=" << pile_id << " 结果=" << result
                 << " 成功数=" << success_count << " 失败数=" << fail_count
                 << " 错误码=" << code;

      // 构建最终结果描述
      std::string final_desc = "自检完成";
      if (result == 0 && fail_count == 0) {
        final_desc = "自检成功";
      } else if (fail_count > 0) {
        final_desc = "自检失败 (" + std::to_string(fail_count) + "项失败)";
      }

      // Update UI
      if (device_manager_ != nullptr) {
        // 更新进度为完成
        device_manager_->updateSelfCheckProgress(pile_id, final_desc, false);

        // 构建并更新最终结果
        device::SelfCheckResult check_result;
        check_result.last_check_result = result;
        check_result.success_count = success_count;
        check_result.fail_count = fail_count;

        // 设置时间
        check_result.finish_time = std::chrono::system_clock::now();
        check_result.last_check_time_str = QDateTime::currentDateTime()
                                               .toString("yyyy-MM-dd HH:mm:ss")
                                               .toStdString();

        // 设置总体状态
        if (result == 0 && fail_count == 0) {
          check_result.status = device::SelfCheckStatus::Passed;
        } else {
          check_result.status = device::SelfCheckStatus::Failed;
        }

        device_manager_->updateSelfCheck(pile_id, check_result);
      }
    } else {
      LOG(WARNING) << "消息格式无效，缺少 NoticeType 或 ReqType";
      return;
    }

  } catch (const nlohmann::json::exception &e) {
    LOG(ERROR) << "解析自检消息失败: " << e.what() << " 消息内容=" << message;
  } catch (const std::exception &e) {
    LOG(ERROR) << "处理自检消息出错: " << e.what();
  }
}

absl::StatusOr<std::vector<device::CCUAttributes>>
CheckManager::getResultFromRedis(const std::string &device_id) {
  if (device_manager_ == nullptr) {
    return absl::InternalError("DeviceManager is not initialized");
  }

  auto device = device_manager_->getDeviceByEquipNo(device_id);
  if (!device) {
    return absl::NotFoundError("Device not found: " + device_id);
  }

  const std::string type = device->Attributes().type;
  if (type != device::kDeviceTypePILE) {
    return absl::UnimplementedError("Unsupported device type: " + type);
  }
  auto keys_or = getRedisKey(device_id);
  if (!keys_or.ok()) {
    return keys_or.status();
  }

  auto *redis = client::RedisClient::GetInstance();
  if (redis == nullptr || !redis->IsConnected()) {
    return absl::InternalError("Redis client is not ready");
  }

  std::vector<device::CCUAttributes> ccu_attributes;
  for (const auto &module_key : keys_or.value()) {
    DLOG(INFO) << "Processing module key: " << module_key;
    auto hash_opt = redis->HGetAll(module_key);
    if (!hash_opt) {
      return absl::InternalError("HGETALL failed: " + module_key);
    }
    if (hash_opt->empty()) {
      return absl::NotFoundError("Redis hash empty: " + module_key);
    }
    auto hash = *hash_opt;
    hash["index"] = module_key.substr(module_key.find_last_of('#') + 1);
    auto attributes = utils::ToCCUAttr(hash);
    if (!attributes.ok()) {
      return attributes.status();
    }
    ccu_attributes.push_back(attributes.value());
  }

  if (ccu_attributes.empty()) {
    return absl::NotFoundError("No valid CCUAttributes found in Redis for: " +
                               device_id);
  }

  LOG(INFO) << "Successfully fetched " << ccu_attributes.size()
            << " CCU attributes from Redis for device: " << device_id;
  return ccu_attributes;
}

absl::Status CheckManager::saveResultToMysql(const std::string &device_id) {
  auto *mysql = db::MySqlClient::GetInstance();
  if (mysql == nullptr) {
    return absl::InternalError("MySQL client not initialized");
  }

  if (device_manager_ == nullptr) {
    return absl::InternalError("DeviceManager is not initialized");
  }

  auto device = device_manager_->getDeviceByEquipNo(device_id);
  if (!device) {
    return absl::NotFoundError("Device not found: " + device_id);
  }

  auto device_type = device->Attributes().type;

  nlohmann::json details_json; // NOLINT
  details_json["deviceId"] = device_id;
  details_json["deviceName"] = device->Attributes().name;
  details_json["deviceType"] = device->Attributes().type;

  int issue_count = 0;
  auto attributes_or = getResultFromRedis(device_id);
  if (!attributes_or.ok()) {
    return attributes_or.status();
  }

  std::vector<nlohmann::json> modules;
  auto add_issue_if_true = [&issue_count](bool value) {
    if (value) {
      ++issue_count;
    }
  };

  for (const auto &attr : attributes_or.value()) {
    nlohmann::json module_json; // NOLINT
    module_json["index"] = attr.index;

    module_json["acContactor1"] = {
        {"refuse", attr.ac_contactor_1.contactor1_refuse},
        {"stuck", attr.ac_contactor_1.contactor1_stuck}};
    add_issue_if_true(attr.ac_contactor_1.contactor1_refuse);
    add_issue_if_true(attr.ac_contactor_1.contactor1_stuck);

    module_json["acContactor2"] = {
        {"refuse", attr.ac_contactor_2.contactor1_refuse},
        {"stuck", attr.ac_contactor_2.contactor1_stuck}};
    add_issue_if_true(attr.ac_contactor_2.contactor1_refuse);
    add_issue_if_true(attr.ac_contactor_2.contactor1_stuck);

    module_json["parallelContactor"] = {
        {"positiveRefuse", attr.parallel_contactor.positive_refuse},
        {"positiveStuck", attr.parallel_contactor.positive_stuck},
        {"negativeRefuse", attr.parallel_contactor.negative_refuse},
        {"negativeStuck", attr.parallel_contactor.negative_stuck}};
    add_issue_if_true(attr.parallel_contactor.positive_refuse);
    add_issue_if_true(attr.parallel_contactor.positive_stuck);
    add_issue_if_true(attr.parallel_contactor.negative_refuse);
    add_issue_if_true(attr.parallel_contactor.negative_stuck);

    module_json["fan1"] = {{"stopped", attr.fan_1.stopped},
                           {"rotating", attr.fan_1.rotating}};
    module_json["fan2"] = {{"stopped", attr.fan_2.stopped},
                           {"rotating", attr.fan_2.rotating}};
    module_json["fan3"] = {{"stopped", attr.fan_3.stopped},
                           {"rotating", attr.fan_3.rotating}};
    module_json["fan4"] = {{"stopped", attr.fan_4.stopped},
                           {"rotating", attr.fan_4.rotating}};
    add_issue_if_true(attr.fan_1.stopped);
    add_issue_if_true(attr.fan_2.stopped);
    add_issue_if_true(attr.fan_3.stopped);
    add_issue_if_true(attr.fan_4.stopped);

    module_json["gunA"] = {
        {"positiveContactorRefuse", attr.gun_a.positive_contactor_refuse},
        {"positiveContactorStuck", attr.gun_a.positive_contactor_stuck},
        {"negativeContactorRefuse", attr.gun_a.negative_contactor_refuse},
        {"negativeContactorStuck", attr.gun_a.negative_contactor_stuck},
        {"unlocked", attr.gun_a.unlocked},
        {"locked", attr.gun_a.locked},
        {"auxPower12v", attr.gun_a.aux_power_12v},
        {"auxPower24v", attr.gun_a.aux_power_24v}};
    add_issue_if_true(attr.gun_a.positive_contactor_refuse);
    add_issue_if_true(attr.gun_a.positive_contactor_stuck);
    add_issue_if_true(attr.gun_a.negative_contactor_refuse);
    add_issue_if_true(attr.gun_a.negative_contactor_stuck);

    module_json["gunB"] = {
        {"positiveContactorRefuse", attr.gun_b.positive_contactor_refuse},
        {"positiveContactorStuck", attr.gun_b.positive_contactor_stuck},
        {"negativeContactorRefuse", attr.gun_b.negative_contactor_refuse},
        {"negativeContactorStuck", attr.gun_b.negative_contactor_stuck},
        {"unlocked", attr.gun_b.unlocked},
        {"locked", attr.gun_b.locked},
        {"auxPower12v", attr.gun_b.aux_power_12v},
        {"auxPower24v", attr.gun_b.aux_power_24v}};
    add_issue_if_true(attr.gun_b.positive_contactor_refuse);
    add_issue_if_true(attr.gun_b.positive_contactor_stuck);
    add_issue_if_true(attr.gun_b.negative_contactor_refuse);
    add_issue_if_true(attr.gun_b.negative_contactor_stuck);

    modules.emplace_back(module_json);
  }

  details_json["ccuModules"] = modules;

  std::string status = "OK";
  std::string summary = "All CCU modules normal";
  if (!attributes_or.ok()) {
    status = "ERROR";
    summary = "Failed to read CCU data from Redis";
  } else if (issue_count > 0) {
    status = "WARN";
    summary = "Detected " + std::to_string(issue_count) + " abnormal flags";
  }

  const std::string sql =
      "INSERT INTO self_check_record "
      "(Type, EquipNo, CheckCategory, Status, Summary, DetailsJSON, "
      "TriggerSource, TriggeredBy) "
      "VALUES (?, ?, ?, ?, ?, ?, ?, ?)";

  const std::string check_category = "FULL";
  const std::string trigger_source = "AUTO";

  std::vector<db::DbValue> params = {
      device_type, device_id,           check_category, status,
      summary,     details_json.dump(), trigger_source, std::nullptr_t{}};

  auto insert_result = mysql->executeUpdate(sql, params);
  if (!insert_result.ok()) {
    return insert_result.status();
  }

  LOG(INFO) << "Saved self check record for " << device_id
            << " status=" << status << " issues=" << issue_count;
  return absl::OkStatus();
}

absl::StatusOr<std::vector<std::string>>
CheckManager::getRedisKey(const std::string &device_id) {
  if (device_manager_ == nullptr) {
    return absl::InternalError("DeviceManager is not initialized");
  }

  auto device = device_manager_->getDeviceByEquipNo(device_id);
  if (!device) {
    LOG(WARNING) << "Device not found: " << device_id;
    return absl::NotFoundError("Device not found: " + device_id);
  }
  const std::string &type = device->Attributes().type;
  std::string modules_key = "selfcheck:" + type + "#" + device_id + ":CCU#*";

  auto *redis = client::RedisClient::GetInstance();
  if (redis == nullptr || !redis->IsConnected()) {
    LOG(ERROR) << "Redis client is not ready; cannot fetch result for "
               << device_id;
    return absl::InternalError("Redis client is not ready");
  }

  return redis->Keys(modules_key);
}
} // namespace EAutoCheck
