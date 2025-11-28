#include "self_check_manager.h"

#include <QDateTime>
#include <QTimer>
#include <QUuid>
#include <random>

namespace selfcheck {

namespace {

QString GenerateRequestId() {
  return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

std::string NowIsoTimestamp() {
  return QDateTime::currentDateTimeUtc()
      .toString(Qt::ISODateWithMs)
      .toStdString();
}

StatusLevel ModuleStatusFromIndex(int index) {
  if (index == 0) {
    return StatusLevel::kWarn;
  }
  return StatusLevel::kOk;
}

} // namespace

SelfCheckManager::SelfCheckManager(QObject *parent) : QObject(parent) {}

SelfCheckManager::~SelfCheckManager() = default;

QString SelfCheckManager::triggerChargerBoxSelfCheck(const QString &deviceId,
                                                     const QString &checkType) {
  const QString requestId = GenerateRequestId();

  pending_checks_.insert(requestId, PendingCheck{deviceId, checkType});
  emit selfCheckStarted(requestId, deviceId, checkType);

  constexpr int kDelayMs = 1200;
  QTimer::singleShot(kDelayMs, this,
                     [this, requestId]() { onFakeCheckCompleted(requestId); });

  return requestId;
}

void SelfCheckManager::onFakeCheckCompleted(const QString &requestId) {
  const auto pendingIt = pending_checks_.find(requestId);
  if (pendingIt == pending_checks_.end()) {
    return;
  }

  const QString deviceId = pendingIt->deviceId;
  const QString checkType = pendingIt->checkType;
  pending_checks_.erase(pendingIt);

  SelfCheckResult result =
      createFakeChargerBoxResult(requestId, deviceId, checkType);
  emit selfCheckResultReady(result);
}

SelfCheckResult
SelfCheckManager::createFakeChargerBoxResult(const QString &requestId,
                                             const QString &deviceId,
                                             const QString &checkType) const {
  SelfCheckResult result;
  result.device_type = "charger_box";
  result.device_id = deviceId.toStdString();
  result.request_id = requestId.toStdString();
  result.timestamp = NowIsoTimestamp();
  result.overall_status =
      checkType.toLower() == "quick" ? StatusLevel::kWarn : StatusLevel::kOk;

  const std::vector<std::pair<QString, QString>> modules = {
      {"communication", "COMM_OK"}, {"main_control", "MCU_OK"},
      {"power", "POWER_OK"},        {"gun_head", "GUN_OK"},
      {"safety", "SAFETY_OK"},
  };

  int index = 0;
  for (const auto &module : modules) {
    ModuleStatus moduleStatus;
    moduleStatus.module_name = module.first.toStdString();
    moduleStatus.status =
        index == 3 && result.overall_status == StatusLevel::kWarn
            ? StatusLevel::kWarn
            : ModuleStatusFromIndex(index);
    moduleStatus.code = module.second.toStdString();
    moduleStatus.message = module.first == "gun_head"
                               ? "Gun head temperature normal"
                               : "Module operating normally";
    moduleStatus.timestamp = result.timestamp;
    moduleStatus.details = {{"check_type", checkType.toStdString()},
                            {"sequence", std::to_string(index + 1)}};
    result.modules.push_back(moduleStatus);
    ++index;
  }

  return result;
}

} // namespace selfcheck
