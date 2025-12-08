#pragma once

#include <QHash>
#include <QObject>
#include <QString>
#include <absl/status/status.h>
#include <absl/status/statusor.h>
#include <device/device_object.h>
#include <qtmetamacros.h>
#include <vector>

namespace device {
class DeviceManager;
}

namespace EAutoCheck {

class CheckManager : public QObject {
  Q_OBJECT

public:
  explicit CheckManager(device::DeviceManager *device_manager,
                        QObject *parent = nullptr);
  ~CheckManager() override;

  Q_INVOKABLE absl::Status CheckDevice(const QString &equip_no);

  absl::Status SubMsg();

private:
  void recvMsg(const std::string &message);

  absl::Status processAndSaveResult(const std::string &device_id);

  absl::StatusOr<std::vector<device::CCUAttributes>>
  getResultFromRedis(const std::string &device_id);
  void saveResultToMysql(const std::string &device_id,
                         const std::string &result);
  absl::StatusOr<std::vector<std::string>>
  getRedisKey(const std::string &device_id);

  struct PendingCheck {
    QString deviceId;
    QString checkType;
  };

  std::atomic<uint64_t> sequence_index_{1};
  device::DeviceManager *device_manager_;
};

} // namespace EAutoCheck
