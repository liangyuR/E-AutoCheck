#pragma once

#include "device/device_object.h"
#include <chrono>
#include <iomanip>
#include <sstream>
#include <string>

namespace device {
// ==== 充电箱设备实例 ====
class PileDevice {
public:
  explicit PileDevice(const PileAttr &attrs);
  ~PileDevice() = default;

  const std::string &Id() const noexcept { return attrs_.equip_no; }
  const PileAttr &Attributes() const noexcept { return attrs_; }
  const DeviceStatus &Status() const noexcept { return status_; }
  const SelfCheckResult &LastSelfCheck() const noexcept {
    return last_self_check_;
  }
  const std::string &CurrentSelfCheckDesc() const noexcept {
    return current_self_check_desc_;
  }
  bool IsSelfChecking() const noexcept { return is_self_checking_; }
  const std::string &LastCheckTime() const noexcept {
    return last_check_time_str_;
  }

  void UpdateStatus(const DeviceStatus &status) { status_ = status; }
  void UpdateSelfCheck(const SelfCheckResult &result);
  void UpdateSelfCheckProgress(const std::string &desc, bool is_checking);

  // 一些便捷判断接口（具体逻辑在 .cpp 中实现）
  bool IsOnline() const { return status_.online_state == OnlineState::Online; }

  bool HasCriticalFault() const {
    return status_.fault_level == FaultLevel::Critical;
  }

private:
  PileAttr attrs_;
  DeviceStatus status_{};               // 默认 Unknown/Offline 等
  SelfCheckResult last_self_check_{};   // 默认空结果
  std::string current_self_check_desc_; // 当前自检进度描述
  bool is_self_checking_ = false;       // 是否正在自检
  std::string last_check_time_str_; // 最后自检时间字符串（用于 UI 显示）
};

} // namespace device
