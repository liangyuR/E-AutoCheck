#pragma once

#include <QHash>
#include <QObject>
#include <QString>
#include <absl/status/status.h>
#include <absl/status/statusor.h>
#include <device/device_object.h>
#include <qtmetamacros.h>
#include <vector>

namespace qml_model {
class DeviceModel;
}

namespace EAutoCheck {

class CheckManager : public QObject {
  Q_OBJECT

public:
  explicit CheckManager(qml_model::DeviceModel *device_model,
                        QObject *parent = nullptr);
  ~CheckManager() override;

  Q_INVOKABLE absl::Status CheckDevice(const QString &equip_no);

  absl::Status SubMsg();

private:
  void recvMsg(const std::string &message);

  absl::StatusOr<std::vector<device::CCUAttributes>>
  getResultFromRedis(const std::string &device_id);

  absl::Status saveResultToMysql(const std::string &device_id);

  // eg: selfcheck:PILE#0817913362940806:CCU#9
  // selfcheck:{device_type}#{device_id}:{module_name}#{module_id}
  absl::StatusOr<std::vector<std::string>>
  getRedisKey(const std::string &device_id);

  struct PendingCheck {
    QString deviceId;
    QString checkType;
  };

  std::atomic<uint64_t> sequence_index_{1};
  qml_model::DeviceModel *device_model_;
};

} // namespace EAutoCheck
