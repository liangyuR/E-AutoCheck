#include "device/device_repo.h"
#include "client/mysql_client.h"
#include "db/db_row.h"
#include <absl/strings/str_format.h>
#include <nlohmann/json.hpp>

namespace device {

PileAttr DeviceRepo::PileDeviceFromDbRow(const db::DbRow &row) {
  PileAttr attrs;
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

absl::StatusOr<std::vector<PileAttr>> DeviceRepo::GetAllPipeDevices() {
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
  std::vector<PileAttr> attrs;
  attrs.reserve(rows.size());
  for (const auto &row : rows) {
    attrs.push_back(DeviceRepo::PileDeviceFromDbRow(row));
  }
  return attrs;
}

absl::StatusOr<std::vector<qml_model::HistoryItem>>
DeviceRepo::GetHistoryItems(const QString &deviceId, int limit) {
  auto *client = db::MySqlClient::GetInstance();
  if (client == nullptr) {
    return absl::InternalError("MySQL client not initialized");
  }

  // 表结构参考 doc/db.md: self_check_record
  auto rows_result = client->executeQuery(
      "SELECT ID, EquipNo, CreatedAt, Status, Summary "
      "FROM self_check_record "
      "WHERE EquipNo = ? "
      "ORDER BY CreatedAt DESC LIMIT ?",
      {deviceId.toStdString(), static_cast<int64_t>(limit)});

  if (!rows_result.ok()) {
    return rows_result.status();
  }

  std::vector<qml_model::HistoryItem> items;
  const auto &rows = rows_result.value();
  items.reserve(rows.size());

  for (const auto &row : rows) {
    qml_model::HistoryItem item;
    item.recordId = QString::fromStdString(row.getString("ID"));
    item.deviceId = QString::fromStdString(row.getString("EquipNo"));

    const auto ts_str = QString::fromStdString(row.getString("CreatedAt"));
    item.timestamp = QDateTime::fromString(ts_str, Qt::ISODate);
    if (!item.timestamp.isValid()) {
      // Fallback to common MySQL DATETIME format
      item.timestamp = QDateTime::fromString(ts_str, "yyyy-MM-dd HH:mm:ss");
    }

    item.status = QString::fromStdString(row.getString("Status"));
    item.summary = QString::fromStdString(row.getString("Summary"));
    items.push_back(std::move(item));
  }

  return items;
}

absl::StatusOr<std::vector<device::CCUAttributes>>
DeviceRepo::GetPileItems(const QString &recordId) {
  auto *client = db::MySqlClient::GetInstance();
  if (client == nullptr) {
    return absl::InternalError("MySQL client not initialized");
  }

  auto rows_result = client->executeQuery(
      "SELECT * FROM self_check_record WHERE ID = ?", {recordId.toStdString()});

  if (!rows_result.ok()) {
    return rows_result.status();
  }

  const auto &rows = rows_result.value();
  if (rows.empty()) {
    return absl::NotFoundError(
        absl::StrFormat("record not found for id: %s", recordId.toStdString()));
  }

  const auto detail_json_str = rows.front().getString("DetailsJSON");

  nlohmann::json detail_json; // NOLINT
  try {
    detail_json = nlohmann::json::parse(detail_json_str);
  } catch (const nlohmann::json::exception &e) {
    return absl::InvalidArgumentError(
        absl::StrFormat("failed to parse DetailsJSON: %s", e.what()));
  }

  if (!detail_json.contains("ccuModules") ||
      !detail_json["ccuModules"].is_array()) {
    return absl::InvalidArgumentError("DetailsJSON missing ccuModules array");
  }

  auto get_string = [](const nlohmann::json &obj,
                       const std::string &key) -> std::string {
    auto it = obj.find(key);
    if (it == obj.end() || !it->is_string()) {
      return "";
    }
    return it->get<std::string>();
  };

  const std::string device_id = get_string(detail_json, "deviceId");
  const std::string device_name = get_string(detail_json, "deviceName");
  const std::string device_type = get_string(detail_json, "deviceType");
  const auto create_at = rows.front().getString("CreatedAt");

  const auto &modules = detail_json["ccuModules"];
  std::vector<device::CCUAttributes> attributes;
  attributes.reserve(modules.size());

  auto get_nested_bool = [](const nlohmann::json &obj,
                            const std::string &key) -> bool {
    if (!obj.is_object()) {
      return false;
    }
    auto it = obj.find(key);
    if (it == obj.end() || !it->is_boolean()) {
      return false;
    }
    return it->get<bool>();
  };

  for (const auto &module : modules) {
    if (!module.is_object()) {
      continue;
    }

    device::CCUAttributes attr;
    attr.index = module.value("index", 0);
    attr.device_id = device_id;
    attr.device_name = device_name;
    attr.device_type = device_type;
    attr.last_check_time = create_at;

    const auto &ac1 = module.value("acContactor1", nlohmann::json::object());
    attr.ac_contactor_1.contactor1_stuck = get_nested_bool(ac1, "stuck");
    attr.ac_contactor_1.contactor1_refuse = get_nested_bool(ac1, "refuse");

    const auto &ac2 = module.value("acContactor2", nlohmann::json::object());
    attr.ac_contactor_2.contactor1_stuck = get_nested_bool(ac2, "stuck");
    attr.ac_contactor_2.contactor1_refuse = get_nested_bool(ac2, "refuse");

    const auto &parallel =
        module.value("parallelContactor", nlohmann::json::object());
    attr.parallel_contactor.positive_stuck =
        get_nested_bool(parallel, "positiveStuck");
    attr.parallel_contactor.positive_refuse =
        get_nested_bool(parallel, "positiveRefuse");
    attr.parallel_contactor.negative_stuck =
        get_nested_bool(parallel, "negativeStuck");
    attr.parallel_contactor.negative_refuse =
        get_nested_bool(parallel, "negativeRefuse");

    const auto &fan1 = module.value("fan1", nlohmann::json::object());
    attr.fan_1.stopped = get_nested_bool(fan1, "stopped");
    attr.fan_1.rotating = get_nested_bool(fan1, "rotating");

    const auto &fan2 = module.value("fan2", nlohmann::json::object());
    attr.fan_2.stopped = get_nested_bool(fan2, "stopped");
    attr.fan_2.rotating = get_nested_bool(fan2, "rotating");

    const auto &fan3 = module.value("fan3", nlohmann::json::object());
    attr.fan_3.stopped = get_nested_bool(fan3, "stopped");
    attr.fan_3.rotating = get_nested_bool(fan3, "rotating");

    const auto &fan4 = module.value("fan4", nlohmann::json::object());
    attr.fan_4.stopped = get_nested_bool(fan4, "stopped");
    attr.fan_4.rotating = get_nested_bool(fan4, "rotating");

    const auto &gun_a = module.value("gunA", nlohmann::json::object());
    attr.gun_a.positive_contactor_stuck =
        get_nested_bool(gun_a, "positiveContactorStuck");
    attr.gun_a.positive_contactor_refuse =
        get_nested_bool(gun_a, "positiveContactorRefuse");
    attr.gun_a.negative_contactor_stuck =
        get_nested_bool(gun_a, "negativeContactorStuck");
    attr.gun_a.negative_contactor_refuse =
        get_nested_bool(gun_a, "negativeContactorRefuse");
    attr.gun_a.unlocked = get_nested_bool(gun_a, "unlocked");
    attr.gun_a.locked = get_nested_bool(gun_a, "locked");
    attr.gun_a.aux_power_12v = get_nested_bool(gun_a, "auxPower12v");
    attr.gun_a.aux_power_24v = get_nested_bool(gun_a, "auxPower24v");

    const auto &gun_b = module.value("gunB", nlohmann::json::object());
    attr.gun_b.positive_contactor_stuck =
        get_nested_bool(gun_b, "positiveContactorStuck");
    attr.gun_b.positive_contactor_refuse =
        get_nested_bool(gun_b, "positiveContactorRefuse");
    attr.gun_b.negative_contactor_stuck =
        get_nested_bool(gun_b, "negativeContactorStuck");
    attr.gun_b.negative_contactor_refuse =
        get_nested_bool(gun_b, "negativeContactorRefuse");
    attr.gun_b.unlocked = get_nested_bool(gun_b, "unlocked");
    attr.gun_b.locked = get_nested_bool(gun_b, "locked");
    attr.gun_b.aux_power_12v = get_nested_bool(gun_b, "auxPower12v");
    attr.gun_b.aux_power_24v = get_nested_bool(gun_b, "auxPower24v");

    attributes.emplace_back(std::move(attr));
  }

  return attributes;
}

absl::StatusOr<std::vector<device::CCUAttributes>>
DeviceRepo::GetLatestPileItems(const QString &deviceId) {
  auto *client = db::MySqlClient::GetInstance();
  if (client == nullptr) {
    return absl::InternalError("MySQL client not initialized");
  }

  // 查询该设备最新的检查记录ID
  auto rows_result = client->executeQuery("SELECT ID FROM self_check_record "
                                          "WHERE EquipNo = ? "
                                          "ORDER BY CreatedAt DESC LIMIT 1",
                                          {deviceId.toStdString()});

  if (!rows_result.ok()) {
    return rows_result.status();
  }

  const auto &rows = rows_result.value();
  if (rows.empty()) {
    // 设备没有检查记录，返回空列表
    return std::vector<device::CCUAttributes>{};
  }

  const auto recordId = QString::fromStdString(rows.front().getString("ID"));
  return GetPileItems(recordId);
}

} // namespace device
