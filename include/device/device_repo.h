#pragma once

#include "absl/status/statusor.h"
#include "db/db_row.h"
#include "device/charge_pipe_device.h"
#include "model/history_model.h"
#include "model/pile_model.h"
#include <vector>

namespace device {

class DeviceRepo {
public:
  static absl::StatusOr<std::vector<ChargerBoxAttributes>> GetAllPipeDevices();

  static absl::StatusOr<std::vector<qml_model::HistoryItem>>
  GetHistoryItems(const QString &deviceId, int limit = 10);

  static absl::StatusOr<std::vector<device::CCUAttributes>>
  GetPileItems(const QString &recordId);

private:
  static ChargerBoxAttributes
  ChargerBoxAttributesFromDbRow(const db::DbRow &row);
};

} // namespace device
