#include "check_manager.h"

#include <QDateTime>
#include <QTimer>
#include <QUuid>
#include <absl/status/status.h>
#include <glog/logging.h>
#include <nlohmann/json.hpp>
#include <random>

#include "client/rabbitmq_client.h"

namespace EAutoCheck {
CheckManager::CheckManager(QObject *parent) : QObject(parent) {}

CheckManager::~CheckManager() = default;

absl::Status CheckManager::Subscribe() {
  auto *rabbit_client = client::RabbitMqClient::GetInstance();
  if (rabbit_client == nullptr) {
    return absl::InternalError("Failed to get RabbitMQ client");
  }
  rabbit_client->Subscribe(
      "DataServer.AutoCheck.notice",
      [this](const std::string &message) { ReceiveMessage(message); });
  rabbit_client->Subscribe(
      "DataServer.AutoCheck.resp",
      [this](const std::string &message) { ReceiveMessage(message); });
  return absl::OkStatus();
}

absl::Status CheckManager::CheckDevice(const QString &equip_no) {
  auto *rabbit_client = client::RabbitMqClient::GetInstance();
  if (rabbit_client == nullptr) {
    return absl::InternalError("Failed to get RabbitMQ client");
  }

  // 自增索引
  uint64_t index = sequence_index_.fetch_add(1);

  // 构建 JSON
  nlohmann::json j;
  j["ReqType"] = 1010;
  j["Index"] = index;
  j["Content"] = {{"pileId", equip_no.toStdString()}};

  std::string payload = j.dump();

  // Routing Key: {appName}.{foreName}.req
  // 这里暂定 appName="auto_charge" (与 queue_name 一致), foreName="DataServer"
  std::string routing_key = "AutoCheck.DataServer.req"; // TODO(@liangyu) fixme
  rabbit_client->Publish(routing_key, payload);
  LOG(INFO) << "Sent SelfCheck req: key=" << routing_key << " idx=" << index
            << " pile=" << equip_no.toStdString();
  LOG(INFO) << "json: " << j.dump();
  return absl::OkStatus();
}

void CheckManager::ReceiveMessage(const std::string &message) {
  try {
    nlohmann::json j = nlohmann::json::parse(message);

    if (!j.contains("Data")) {
      LOG(WARNING) << "Invalid message format, missing Data field";
      return;
    }

    auto data = j["Data"];
    if (!data.contains("pileId")) {
      LOG(WARNING) << "Invalid Data field, missing pileId";
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
        LOG(WARNING) << "Unknown state: " << state;
        return;
      }

      LOG(INFO) << "SelfCheck progress: pileId=" << pile_id << " desc=" << desc
                << " state=" << state << " result=" << result
                << " checking=" << is_checking << " finished=" << is_finished;

      // 发射进度更新信号
      emit checkStatusUpdated(QString::fromStdString(pile_id),
                              QString::fromStdString(desc), result, is_finished,
                              is_checking);

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

      // 发射最终结果信号
      emit checkStatusUpdated(QString::fromStdString(pile_id),
                              QString::fromStdString(final_desc), result,
                              is_finished, is_checking);
      emit checkResultReady(QString::fromStdString(pile_id), result,
                            success_count, fail_count, code);

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

} // namespace EAutoCheck
