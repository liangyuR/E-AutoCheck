#pragma once

#include <QHash>
#include <QObject>
#include <QString>

#include "utils/self_check_result.h"

namespace selfcheck {

class SelfCheckManager : public QObject {
  Q_OBJECT

public:
  explicit SelfCheckManager(QObject *parent = nullptr);
  ~SelfCheckManager() override;

  Q_INVOKABLE QString triggerChargerBoxSelfCheck(const QString &deviceId,
                                                 const QString &checkType);

signals:
  void selfCheckStarted(const QString &requestId, const QString &deviceId,
                        const QString &checkType);
  void selfCheckResultReady(const selfcheck::SelfCheckResult &result);

private:
  struct PendingCheck {
    QString deviceId;
    QString checkType;
  };

  void onFakeCheckCompleted(const QString &requestId);
  selfcheck::SelfCheckResult
  createFakeChargerBoxResult(const QString &requestId, const QString &deviceId,
                             const QString &checkType) const;

  QHash<QString, PendingCheck> pending_checks_;
};

} // namespace selfcheck
