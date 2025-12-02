#pragma once

#include "absl/status/statusor.h"
#include "db/db_row.h"
#include "device/charge_pipe_device.h"
#include <vector>

namespace device {

class DeviceRepo {
public:
  static absl::StatusOr<std::vector<ChargerBoxAttributes>> GetAllPipeDevices();

private:
  static ChargerBoxAttributes
  ChargerBoxAttributesFromDbRow(const db::DbRow &row);
};

} // namespace device