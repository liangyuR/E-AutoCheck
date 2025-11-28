#include "ui/self_check_controller.h"

#include <QVariantMap>

namespace ui {
namespace {

QString StatusToQString(selfcheck::StatusLevel status) {
  return QString::fromUtf8(selfcheck::ToString(status));
}

QVariantMap DetailsToVariantMap(const selfcheck::DetailsMap &details) {
  QVariantMap map;
  for (const auto &entry : details) {
    map.insert(QString::fromStdString(entry.first),
               QString::fromStdString(entry.second));
  }
  return map;
}

}  // namespace

SelfCheckController::SelfCheckController(
    selfcheck::SelfCheckManager *manager, QObject *parent)
    : QObject(parent), manager_(manager) {
  if (manager_ == nullptr) {
    return;
  }

  connect(manager_, &selfcheck::SelfCheckManager::selfCheckStarted, this,
          &SelfCheckController::onSelfCheckStarted);
  connect(manager_, &selfcheck::SelfCheckManager::selfCheckResultReady, this,
          &SelfCheckController::onSelfCheckResultReady);
}

SelfCheckController::~SelfCheckController() = default;

QString SelfCheckController::triggerChargerBox(const QString &deviceId) {
  if (manager_ == nullptr) {
    return {};
  }
  return manager_->triggerChargerBoxSelfCheck(deviceId, "full");
}

QVariantList SelfCheckController::lastModules() const {
  QVariantList modules;
  for (const auto &module : lastResult_.modules) {
    QVariantMap moduleMap;
    moduleMap.insert("name", QString::fromStdString(module.module_name));
    moduleMap.insert("status", StatusToQString(module.status));
    moduleMap.insert("code", QString::fromStdString(module.code));
    moduleMap.insert("message", QString::fromStdString(module.message));
    moduleMap.insert("timestamp", QString::fromStdString(module.timestamp));
    moduleMap.insert("severity", selfcheck::StatusSeverity(module.status));
    moduleMap.insert("details", DetailsToVariantMap(module.details));
    modules.push_back(moduleMap);
  }
  return modules;
}

QString SelfCheckController::lastRequestId() const { return lastRequestId_; }

QString SelfCheckController::lastOverallStatus() const {
  return StatusToQString(lastResult_.overall_status);
}

QString SelfCheckController::lastDeviceId() const { return lastDeviceId_; }

void SelfCheckController::onSelfCheckStarted(const QString &requestId,
                                             const QString &deviceId,
                                             const QString &checkType) {
  Q_UNUSED(checkType);

  lastRequestId_ = requestId;
  lastDeviceId_ = deviceId;
  if (!isChecking_) {
    isChecking_ = true;
    emit isCheckingChanged();
  }
}

void SelfCheckController::onSelfCheckResultReady(
    const selfcheck::SelfCheckResult &result) {
  lastResult_ = result;
  isChecking_ = false;
  emit lastResultChanged();
  emit isCheckingChanged();
}

}  // namespace ui


