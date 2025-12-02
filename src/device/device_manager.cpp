#include "device/device_manager.h"
#include <glog/logging.h>

namespace device {

DeviceManager::DevicePtr DeviceManager::addDevice(ChargerBoxAttributes attrs) {
  auto device = std::make_shared<ChargerBoxDevice>(std::move(attrs));
  const auto &key = device->Id(); // 默认返回 equip_no

  {
    std::lock_guard<std::mutex> lock(mutex_);
    devices_[key] = device; // 覆盖同 key 设备
  }

  LOG(INFO) << "Device added: " << key;
  return device;
}

DeviceManager::DevicePtr
DeviceManager::getDeviceByEquipNo(const std::string &equip_no) const {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = devices_.find(equip_no);
  if (it == devices_.end()) {
    return nullptr;
  }
  return it->second;
}

bool DeviceManager::hasDevice(const std::string &equip_no) const {
  std::lock_guard<std::mutex> lock(mutex_);
  return devices_.find(equip_no) != devices_.end();
}

std::vector<DeviceManager::DevicePtr> DeviceManager::allDevices() const {
  std::vector<DevicePtr> result;
  std::lock_guard<std::mutex> lock(mutex_);
  result.reserve(devices_.size());
  for (const auto &kv : devices_) {
    result.push_back(kv.second);
  }
  return result;
}

void DeviceManager::updateStatus(const std::string &equip_no,
                                 const DeviceStatus &status) {
  auto dev = getDeviceByEquipNo(equip_no);
  if (!dev) {
    LOG(WARNING) << "updateStatus: device not found, equip_no=" << equip_no;
    return;
  }
  dev->UpdateStatus(status);
}

void DeviceManager::updateSelfCheck(const std::string &equip_no,
                                    const SelfCheckResult &result) {
  auto dev = getDeviceByEquipNo(equip_no);
  if (!dev) {
    LOG(WARNING) << "updateSelfCheck: device not found, equip_no=" << equip_no;
    return;
  }
  dev->UpdateSelfCheck(result);
}

} // namespace device
