#include "device/device_repo.h"
#include "client/mysql_client.h"
#include "db/db_row.h"

namespace device {

ChargerBoxAttributes
DeviceRepo::ChargerBoxAttributesFromDbRow(const db::DbRow &row) {
  ChargerBoxAttributes attrs;
  attrs.db_id = row.getInt("ID");
  attrs.station_no = row.getString("StationNo");
  attrs.equip_no = row.getString("EquipNo");
  attrs.name = row.getString("EquipName");
  attrs.name_en = row.getString("EquipNameEn");
  attrs.type = row.getString("Type");
  attrs.ip_addr = row.getString("IPAddr");
  attrs.gun_count = row.getInt("GunCount");
  attrs.equip_order = row.getInt("EquipOrder");
  attrs.encrypt = row.getInt("Encrypt");
  attrs.secret_key = row.getString("SecretKey");
  attrs.secret_iv = row.getString("SecretIV");
  attrs.data1 = row.getInt("Data1");
  attrs.data3 = row.getInt("Data3");
  attrs.data2_json = row.getNullableString("Data2").value_or("");
  attrs.data4_json = row.getNullableString("Data4").value_or("");
  return attrs;
}

absl::StatusOr<std::vector<ChargerBoxAttributes>>
DeviceRepo::GetAllPipeDevices() {
  auto *client = db::MySqlClient::GetInstance();
  if (client == nullptr) {
    return absl::InternalError("MySQL client not initialized");
  }

  auto rows_result = client->executeQuery(
      "SELECT ID, StationNo, EquipNo, EquipName, EquipNameEn, Type, "
      "IPAddr, GunCount, EquipOrder, Encrypt, SecretKey, SecretIV, "
      "Data1, Data2, Data3, Data4 "
      "FROM equipment_info "
      "WHERE Type = 'PILE'");

  if (!rows_result.ok()) {
    return rows_result.status();
  }

  const auto &rows = rows_result.value();
  std::vector<ChargerBoxAttributes> attrs;
  attrs.reserve(rows.size());
  for (const auto &row : rows) {
    attrs.push_back(DeviceRepo::ChargerBoxAttributesFromDbRow(row));
  }
  return attrs;
}
} // namespace device
