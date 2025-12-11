#include "device/pile_device.h"

namespace device {
PileDevice::PileDevice(const PileAttr &attrs) {
  attrs_ = attrs;
  status_ = DeviceStatus();
  last_self_check_ = SelfCheckResult();
}

void PileDevice::UpdateSelfCheck(const SelfCheckResult &result) {
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

void PileDevice::UpdateSelfCheckProgress(const std::string &desc,
                                         bool is_checking) {
  current_self_check_desc_ = desc;
  is_self_checking_ = is_checking;
}

} // namespace device