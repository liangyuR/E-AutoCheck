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

  (void)getResultFromRedis(equip_no.toStdString());

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

        // TODO(@liangyu) fixme
        if (auto status = processAndSaveResult(pile_id); !status.ok()) {
          LOG(ERROR) << "Failed to process and save result: "
                     << status.message();
        }
      } else {
        LOG(WARNING) << "Unknown state: " << state;
        return;
      }

      LOG(INFO) << "SelfCheck progress: pileId=" << pile_id << " desc=" << desc
                << " state=" << state << " result=" << result
                << " checking=" << is_checking << " finished=" << is_finished;

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

      LOG(INFO) << "SelfCheck final result: pileId=" << pile_id
                << " result=" << result << " success=" << success_count
                << " fail=" << fail_count << " code=" << code;

      // 构建最终结果描述
      std::string final_desc = "自检完成";
      if (result == 0 && fail_count == 0) {
        final_desc = "自检成功";
      } else if (fail_count > 0) {
        final_desc = "自检失败 (" + std::to_string(fail_count) + "项失败)";
      }

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
      LOG(WARNING) << "Invalid message format, missing NoticeType or ReqType";
      return;
    }

  } catch (const nlohmann::json::exception &e) {
    LOG(ERROR) << "Failed to parse self-check message: " << e.what()
               << " message=" << message;
  } catch (const std::exception &e) {
    LOG(ERROR) << "Error processing self-check message: " << e.what();
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
    LOG(WARNING) << "Unsupported device type for Redis self-check: " << type;
    return absl::UnimplementedError("Device type not supported");
  }

  auto keys_or = getRedisKey(device_id);
  if (!keys_or.ok()) {
    return keys_or.status();
  }

  auto *redis = client::RedisClient::GetInstance();
  if (redis == nullptr || !redis->IsConnected()) {
    LOG(ERROR) << "Redis client is not ready, cannot fetch result for "
               << device_id;
    return absl::InternalError("Redis client is not ready");
  }

  std::vector<device::CCUAttributes> ccu_attributes;
  bool parsed = false;

  for (const auto &module_key : keys_or.value()) {
    DLOG(INFO) << "Processing module key: " << module_key;

    if (module_key.find(":CCU#") == std::string::npos) {
      LOG(WARNING) << "Skip non-CCU module key: " << module_key;
      continue;
    }
    device::CCUAttributes attributes;

    auto hash_opt = redis->HGetAll(module_key);
    if (!hash_opt) {
      LOG(ERROR) << "Failed to HGETALL for key: " << module_key;
      return absl::InternalError("Failed to fetch Redis hash");
    }

    if (hash_opt->empty()) {
      LOG(WARNING) << "Redis hash is empty for key: " << module_key;
      return absl::NotFoundError("Redis hash empty");
    }

    std::array<bool, 32> seen{};
    for (const auto &item : *hash_opt) {
      const std::string &field = item.first;
      const std::string &value_str = item.second;

      auto delimiter_pos = field.find(';');
      if (delimiter_pos == std::string::npos) {
        LOG(WARNING) << "Field missing index delimiter: " << field;
        continue;
      }

      int index = -1;
      try {
        index = std::stoi(field.substr(0, delimiter_pos));
      } catch (const std::exception &e) {
        LOG(WARNING) << "Failed to parse field index for '" << field
                     << "': " << e.what();
        continue;
      }

      if (index < 0 || index >= static_cast<int>(seen.size())) {
        LOG(WARNING) << "Field index out of range: " << index << " (" << field
                     << ")";
        continue;
      }

      int numeric_value = 0;
      try {
        numeric_value = std::stoi(value_str);
      } catch (const std::exception &e) {
        LOG(WARNING) << "Failed to parse value for field '" << field
                     << "': " << e.what();
        continue;
      }

      bool status = false;
      if (numeric_value == 1) {
        status = true;
      } else if (numeric_value == 2) {
        status = false;
      } else {
        LOG(WARNING) << "Unknown field value " << numeric_value << " for field "
                     << field;
        continue;
      }

      seen[index] = true;
      switch (index) {
      case 0:
        attributes.ac_contactor_1.contactor1_refuse = status;
        break;
      case 1:
        attributes.ac_contactor_1.contactor1_stuck = status;
        break;
      case 2:
        attributes.ac_contactor_2.contactor1_refuse = status;
        break;
      case 3:
        attributes.ac_contactor_2.contactor1_stuck = status;
        break;
      case 4:
        attributes.parallel_contactor.positive_refuse = status;
        break;
      case 5:
        attributes.parallel_contactor.positive_stuck = status;
        break;
      case 6:
        attributes.parallel_contactor.negative_refuse = status;
        break;
      case 7:
        attributes.parallel_contactor.negative_stuck = status;
        break;
      case 8:
        attributes.fan_1.stopped = status;
        break;
      case 9:
        attributes.fan_1.rotating = status;
        break;
      case 10:
        attributes.fan_2.stopped = status;
        break;
      case 11:
        attributes.fan_2.rotating = status;
        break;
      case 12:
        attributes.fan_3.stopped = status;
        break;
      case 13:
        attributes.fan_3.rotating = status;
        break;
      case 14:
        attributes.fan_4.stopped = status;
        break;
      case 15:
        attributes.fan_4.rotating = status;
        break;
      case 16:
        attributes.gun_a.positive_contactor_refuse = status;
        break;
      case 17:
        attributes.gun_a.positive_contactor_stuck = status;
        break;
      case 18:
        attributes.gun_a.negative_contactor_refuse = status;
        break;
      case 19:
        attributes.gun_a.negative_contactor_stuck = status;
        break;
      case 20:
        attributes.gun_a.unlocked = status;
        break;
      case 21:
        attributes.gun_a.locked = status;
        break;
      case 22:
        attributes.gun_a.aux_power_12v = status;
        break;
      case 23:
        attributes.gun_a.aux_power_24v = status;
        break;
      case 24:
        attributes.gun_b.positive_contactor_refuse = status;
        break;
      case 25:
        attributes.gun_b.positive_contactor_stuck = status;
        break;
      case 26:
        attributes.gun_b.negative_contactor_refuse = status;
        break;
      case 27:
        attributes.gun_b.negative_contactor_stuck = status;
        break;
      case 28:
        attributes.gun_b.unlocked = status;
        break;
      case 29:
        attributes.gun_b.locked = status;
        break;
      case 30:
        attributes.gun_b.aux_power_12v = status;
        break;
      case 31:
        attributes.gun_b.aux_power_24v = status;
        break;
      default:
        LOG(WARNING) << "Unhandled field index: " << index;
        break;
      }
    }

    for (int expected = 0; expected < 32; ++expected) {
      if (!seen[expected]) {
        LOG(WARNING) << "Missing field index " << expected
                     << " in Redis hash for " << module_key;
      }
    }

    ccu_attributes.push_back(attributes);
    parsed = true;
  }

  if (!parsed) {
    return absl::NotFoundError("No CCU module found in Redis");
  }

  return ccu_attributes;
}

void CheckManager::saveResultToMysql(const std::string &device_id,
                                     const std::string &result) {}

absl::StatusOr<std::vector<std::string>>
CheckManager::getRedisKey(const std::string &device_id) {
  // eg: selfcheck:PILE#0817913362940806:CCU#9
  // selfcheck:{device_type}#{device_id}:{module_name}#{module_id}

  auto device = device_manager_->getDeviceByEquipNo(device_id);
  if (!device) {
    return absl::NotFoundError("Device not found: " + device_id);
  }
  const auto type = device->Attributes().type;

  const auto modules_key = "selfcheck:" + type + "#" + device_id;

  auto *redis = client::RedisClient::GetInstance();
  if (redis == nullptr || !redis->IsConnected()) {
    LOG(ERROR) << "Redis client is not ready, cannot fetch result for "
               << device_id;
    return absl::InternalError("Redis client is not ready");
  }

  auto modules_opt = redis->HGetAll(modules_key);
  if (!modules_opt) {
    LOG(WARNING) << "No modules found in Redis for key: " << modules_key;
    return absl::NotFoundError("No modules found in Redis");
  }

  std::vector<std::string> result;
  return result;
  // try {
  //   auto json_modules = nlohmann::json::parse(*modules_opt);
  //   if (json_modules.is_array()) {
  //     for (const auto &item : json_modules) {
  //       if (item.is_string()) {
  //         result.push_back(modules_key + ":" + item.get<std::string>());
  //       }
  //     }
  //   } else {
  //     // Fallback if it's not an array (e.g. single string?)
  //     // But requirement says "list", implying array.
  //     LOG(WARNING) << "Modules data is not an array for key: " <<
  //     modules_key;
  //   }
  // } catch (const std::exception &e) {
  //   LOG(ERROR) << "Failed to parse modules JSON: " << e.what();
  //   return absl::InternalError("Failed to parse modules JSON");
  // }

  // return result;
}

absl::Status CheckManager::processAndSaveResult(const std::string &device_id) {
  if (device_manager_ == nullptr) {
    return absl::InternalError("DeviceManager is not initialized");
  }

  // 1. 获取设备信息以确定 Redis Key
  auto device = device_manager_->getDeviceByEquipNo(device_id);
  if (!device) {
    LOG(WARNING) << "Device not found for result processing: " << device_id;
    return absl::NotFoundError("Device not found");
  }

  // 根据设备类型构建 Redis Key
  // 假设 Type 字段对应 Redis Key 中的类型标识
  // 例如: PILE -> AutoCheck:Result:Pile:{id}
  //       STACK -> AutoCheck:Result:Stack:{id}
  std::string type_str = device->Attributes().type;
  std::string redis_key_type = "Pile"; // Default
  if (type_str == "PILE") {
    redis_key_type = "Pile";
  } else if (type_str == "STACK" || type_str == "Stack") {
    redis_key_type = "Stack";
  } else {
    // 如果有其他类型，尝试直接使用 type
    // 字符串（首字母大写处理可选，这里直接用）
    redis_key_type = type_str;
  }

  std::string redis_key =
      "AutoCheck:Result:" + redis_key_type + ":" + device_id;

  // 2. 获取 Redis 客户端并读取结果
  auto *redis = client::RedisClient::GetInstance();
  if (redis == nullptr || !redis->IsConnected()) {
    LOG(ERROR) << "Redis client is not ready, cannot fetch result for "
               << device_id;
    return absl::InternalError("Redis client is not ready");
  }

  // 获取设备类型

  // 假设 Redis Key 格式为 AutoCheck:Result:{Type}:{pile_id}
  // std::string redis_key = "AutoCheck:Result:" + device_id;
  auto redis_val = redis->Get(redis_key);

  if (!redis_val) {
    LOG(WARNING) << "No check result found in Redis for key: " << redis_key;
    return absl::NotFoundError("No check result found in Redis");
  }

  // 2. 解析 Redis 数据 (整理结果)
  nlohmann::json result_json;
  int final_result = 0;
  std::string raw_data = *redis_val;

  try {
    result_json = nlohmann::json::parse(raw_data);
    // 假设从 JSON 中提取关键字段
    final_result = result_json.value("result", 0);
  } catch (const std::exception &e) {
    LOG(ERROR) << "Failed to parse Redis JSON: " << e.what();
    return absl::InternalError("Failed to parse Redis JSON");
  }

  // 3. 获取 MySQL 客户端并保存
  auto *mysql = db::MySqlClient::GetInstance();
  if (mysql == nullptr) {
    LOG(ERROR) << "MySQL client is not ready";
    return absl::InternalError("MySQL client is not ready");
  }

  // 保存到 MySQL (表名: auto_check_records)
  // 字段: equip_no, check_result, raw_data, created_at
  const std::string sql =
      "INSERT INTO auto_check_records (equip_no, check_result, raw_data, "
      "created_at) VALUES (?, ?, ?, NOW())";

  std::vector<db::DbValue> params = {device_id, final_result, raw_data};

  auto db_res = mysql->executeUpdate(sql, params);
  if (!db_res.ok()) {
    LOG(ERROR) << "Failed to save check result to MySQL: "
               << db_res.status().ToString();
    return db_res.status();
  }
  LOG(INFO) << "Successfully saved check result for " << device_id;

  return absl::OkStatus();
}

} // namespace EAutoCheck
