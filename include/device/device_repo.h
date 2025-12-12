#pragma once

#include "absl/status/statusor.h"
#include "db/db_row.h"
#include "device/pile_device.h"
#include "device/pile_object.h"
#include "model/history_model.h"
#include "model/pile_model.h"
#include <vector>

namespace device {

class DeviceRepo {
public:
  static absl::StatusOr<std::vector<DeviceAttr>> GetAllDevices();

  static absl::StatusOr<std::vector<qml_model::HistoryItem>>
  GetHistoryItems(const QString &deviceId, int limit = 10);

  static absl::StatusOr<device::PileAttributes>
  GetPileItems(const QString &recordId);

  /**
   * @brief 通过设备ID获取最新的检查记录详情
   *
   * 先查询该设备最新的检查记录ID，然后获取其详细的 CCU 模块数据。
   * @param deviceId 设备的唯一标识符 (EquipNo)
   * @return 最新检查记录的 CCU 属性列表，若无记录则返回空列表
   */
  static absl::StatusOr<device::PileAttributes>
  GetLatestPileItems(const QString &deviceId);

private:
  static DeviceAttr PileDeviceFromDbRow(const db::DbRow &row);
};

} // namespace device
