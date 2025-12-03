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
    // 验证必需字段
    if (!j.contains("ReqType") || !j.contains("Result") ||
        !j.contains("Data")) {
      LOG(WARNING) << "Invalid message format, missing required fields";
      return;
    }

    int req_type = j["ReqType"].get<int>();
    int result = j["Result"].get<int>();
    auto data = j["Data"];

    // 只处理自检相关的消息 (ReqType 1001)
    if (req_type != 1001) {
      return;
    }

    if (!data.contains("pileId") || !data.contains("desc")) {
      LOG(WARNING) << "Invalid Data field, missing pileId or desc";
      return;
    }

    std::string pile_id = data["pileId"].get<std::string>();
    std::string desc = data["desc"].get<std::string>();

    // 判断是否完成：
    // 1. Result != 0 表示失败，自检结束
    // 2. desc 包含 "完成" 表示成功完成
    bool is_finished =
        (result != 0) || (desc.find("完成") != std::string::npos);

    LOG(INFO) << "SelfCheck progress: pileId=" << pile_id << " desc=" << desc
              << " result=" << result << " finished=" << is_finished;

    // 发射信号到 QML
    emit checkStatusUpdated(QString::fromStdString(pile_id),
                            QString::fromStdString(desc), result, is_finished);

  } catch (const nlohmann::json::exception &e) {
    LOG(ERROR) << "Failed to parse self-check progress message: " << e.what()
               << " message=" << message;
  } catch (const std::exception &e) {
    LOG(ERROR) << "Error processing self-check progress: " << e.what();
  }
}

} // namespace EAutoCheck
