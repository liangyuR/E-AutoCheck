#pragma once

#include "device/device_object.h"
#include <chrono>
#include <iomanip>
#include <sstream>
#include <string>

namespace device {
struct ChargerBoxAttributes {
  int db_id = 0;

  std::string station_no; // StationNo：站点编号
  std::string equip_no; // EquipNo：设备编号（通常也是业务上的唯一 ID）
  std::string name;    // EquipName：终端1/终端2...
  std::string name_en; // EquipNameEn：No.1 TCU...
  std::string type;    // Type：PILE / SWAP / 其他

  std::string ip_addr; // IPAddr：192.168.x.x
  int gun_count = 0;   // GunCount：枪数量
  int equip_order = 0; // EquipOrder：排序

  int encrypt = 0;        // Encrypt：是否加密
  std::string secret_key; // SecretKey
  std::string secret_iv;  // SecretIV

  int data1 = 0; // Data1：按现有表结构保留
  int data3 = 0; // Data3：通常是 24 等含义，后续可用枚举封装
  std::string data2_json; // Data2：目前是 <null>，预留
  std::string data4_json; // Data4：[{"no":9,"gun":"1,2"}] 之类的 JSON 串

  friend std::ostream &operator<<(std::ostream &os,
                                  const ChargerBoxAttributes &attr) {
    os << "ChargerBoxAttributes{"
       << "db_id=" << attr.db_id << ", "
       << "station_no='" << attr.station_no << "', "
       << "equip_no='" << attr.equip_no << "', "
       << "name='" << attr.name << "', "
       << "name_en='" << attr.name_en << "', "
       << "type='" << attr.type << "', "
       << "ip_addr='" << attr.ip_addr << "', "
       << "gun_count=" << attr.gun_count << ", "
       << "equip_order=" << attr.equip_order << ", "
       << "encrypt=" << attr.encrypt << ", "
       << "secret_key='" << attr.secret_key << "', "
       << "secret_iv='" << attr.secret_iv << "', "
       << "data1=" << attr.data1 << ", "
       << "data3=" << attr.data3 << ", "
       << "data2_json='" << attr.data2_json << "', "
       << "data4_json='" << attr.data4_json << "'"
       << "}";
    return os;
  }
};

// ==== 充电箱设备实例 ====
class ChargerBoxDevice {
public:
  explicit ChargerBoxDevice(ChargerBoxAttributes attrs) {
    attrs_ = std::move(attrs);
    status_ = DeviceStatus();
    last_self_check_ = SelfCheckResult();
  }

  // 业务上的唯一标识：优先使用 equip_no
  const std::string &Id() const noexcept { return attrs_.equip_no; }

  // 只读访问静态属性
  const ChargerBoxAttributes &Attributes() const noexcept { return attrs_; }

  // 当前状态
  const DeviceStatus &Status() const noexcept { return status_; }
  const SelfCheckResult &LastSelfCheck() const noexcept {
    return last_self_check_;
  }

  // 更新状态（由 DeviceManager / SelfCheckManager 调用）
  void UpdateStatus(const DeviceStatus &s) {} // TODO
  void UpdateSelfCheck(const SelfCheckResult &r) {
    last_self_check_ = r;
    // 更新最后自检时间字符串
    if (!r.last_check_time_str.empty()) {
      last_check_time_str_ = r.last_check_time_str;
    } else {
      // 如果没有字符串时间，从 finish_time 转换
      if (r.finish_time.time_since_epoch().count() > 0) {
        auto time_t = std::chrono::system_clock::to_time_t(r.finish_time);
        std::ostringstream oss;
        oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        last_check_time_str_ = oss.str();
      }
    }
  }

  // 更新自检进度（由 SelfCheckManager 通过 DeviceManager 调用）
  void UpdateSelfCheckProgress(const std::string &desc, bool is_checking) {
    current_self_check_desc_ = desc;
    is_self_checking_ = is_checking;
  }

  // 获取当前自检状态
  const std::string &CurrentSelfCheckDesc() const noexcept {
    return current_self_check_desc_;
  }
  bool IsSelfChecking() const noexcept { return is_self_checking_; }
  const std::string &LastCheckTime() const noexcept {
    return last_check_time_str_;
  }

  // 一些便捷判断接口（具体逻辑在 .cpp 中实现）
  bool IsOnline() const {
    return true; // TODO
  }
  bool HasCriticalFault() const {
    return false; // TODO
  }

private:
  ChargerBoxAttributes attrs_;
  DeviceStatus status_{};               // 默认 Unknown/Offline 等
  SelfCheckResult last_self_check_{};   // 默认空结果
  std::string current_self_check_desc_; // 当前自检进度描述
  bool is_self_checking_ = false;       // 是否正在自检
  std::string last_check_time_str_; // 最后自检时间字符串（用于 UI 显示）
};

} // namespace device
