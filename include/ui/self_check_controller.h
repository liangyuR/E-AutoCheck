#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>

#include "self_check_manager.h"

namespace ui {

class SelfCheckController : public QObject {
  Q_OBJECT
  Q_PROPERTY(bool isChecking READ isChecking NOTIFY isCheckingChanged)

public:
  explicit SelfCheckController(selfcheck::SelfCheckManager *manager,
                               QObject *parent = nullptr);
  ~SelfCheckController() override;

  Q_INVOKABLE QString triggerChargerBox(const QString &deviceId);
  Q_INVOKABLE QVariantList lastModules() const;
  Q_INVOKABLE QString lastRequestId() const;
  Q_INVOKABLE QString lastOverallStatus() const;
  Q_INVOKABLE QString lastDeviceId() const;

  bool isChecking() const { return isChecking_; }

signals:
  void isCheckingChanged();
  void lastResultChanged();

private:
  void onSelfCheckStarted(const QString &requestId, const QString &deviceId,
                          const QString &checkType);
  void onSelfCheckResultReady(const selfcheck::SelfCheckResult &result);

  selfcheck::SelfCheckManager *manager_;
  bool isChecking_{false};
  QString lastRequestId_;
  QString lastDeviceId_;
  selfcheck::SelfCheckResult lastResult_;
};

} // namespace ui


