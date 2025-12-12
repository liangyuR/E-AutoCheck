#pragma once

#include "device/device_object.h"
#include "device/pile_object.h"
#include <chrono>
#include <iomanip>
#include <sstream>
#include <string>

namespace device {
// 充电桩设备
class PileDevice {
public:
  explicit PileDevice(const DeviceAttr &attrs)
      : attrs_(static_cast<const PileAttributes &>(attrs)) {}
  ~PileDevice() = default;

  const PileAttributes &Attributes() const noexcept { return attrs_; }

  void UpdateStatus(const DeviceStatus &status) {
    attrs_.device_state = status;
  }

  void UpdateSelfCheck(const device::SelfCheckResult &result) {
    attrs_.self_check_result = result;
  }

  // TODO(@liangyu) 暂不支持自检进度的 check
  void UpdateSelfCheckProgress(const std::string &desc, bool is_checking) {}

  bool IsOnline() const {
    return attrs_.device_state.online_state == OnlineState::Online;
  }

  bool HasCriticalFault() const {
    return attrs_.device_state.fault_level == FaultLevel::Critical;
  }

  void RequestRefresh() {}

  void RequestSelfCheck() {}

private:
  PileAttributes attrs_;
};

} // namespace device
