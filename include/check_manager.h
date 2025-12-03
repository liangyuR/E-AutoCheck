#pragma once

#include <QHash>
#include <QObject>
#include <QString>
#include <absl/status/status.h>
#include <qtmetamacros.h>

namespace EAutoCheck {

class CheckManager : public QObject {
  Q_OBJECT

public:
  explicit CheckManager(QObject *parent = nullptr);
  ~CheckManager() override;

  Q_INVOKABLE absl::Status CheckDevice(const QString &equip_no);

  absl::Status Subscribe();

signals:
  void checkStatusUpdated(QString pileId, QString desc, int result,
                          bool isFinished);

private:
  void ReceiveMessage(const std::string &message);

  struct PendingCheck {
    QString deviceId;
    QString checkType;
  };

  QHash<QString, PendingCheck> onSelfCheckResultReadypending_checks_;
  std::atomic<uint64_t> sequence_index_{1};
};

} // namespace EAutoCheck
