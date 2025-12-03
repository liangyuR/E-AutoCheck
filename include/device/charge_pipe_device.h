#pragma once

#include "device/device_object.h"
#include <chrono>
#include <iomanip>
#include <sstream>
#include <string>

namespace device {
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
  void UpdateStatus(const DeviceStatus &status) {} // TODO
  void UpdateSelfCheck(const SelfCheckResult &result) {
    last_self_check_ = result;
    // 更新最后自检时间字符串
    if (!result.last_check_time_str.empty()) {
      last_check_time_str_ = result.last_check_time_str;
    } else {
      // 如果没有字符串时间，从 finish_time 转换
      if (result.finish_time.time_since_epoch().count() > 0) {
        auto time_t = std::chrono::system_clock::to_time_t(result.finish_time);
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
