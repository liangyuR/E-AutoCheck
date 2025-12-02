#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "device/charge_pipe_device.h"
#include <QObject>
#include <absl/status/status.h>

namespace device {

// 前置声明：你的 DbRow 类型来自哪儿，就按实际库改
struct DbRow;

// 设备管理器：负责持有和查询 ChargerBoxDevice
class DeviceManager : public QObject {
  Q_OBJECT

public:
  using DevicePtr = std::shared_ptr<ChargerBoxDevice>;

  DeviceManager(QObject *parent = nullptr) : QObject(parent) {}
  ~DeviceManager() = default;

  DeviceManager(const DeviceManager &) = delete;
  DeviceManager &operator=(const DeviceManager &) = delete;

  // ============ 设备增删查 ============
  // 从属性创建一台设备并接管其生命周期。
  // 如果 equip_no 已存在，可以选择：
  //  - 覆盖旧设备（当前实现这么做），或
  //  - 忽略 / 抛异常（可根据需要调整）。
  DevicePtr addDevice(ChargerBoxAttributes attrs);

  // 按业务 ID（equip_no）获取设备，找不到返回 nullptr
  DevicePtr getDeviceByEquipNo(const std::string &equip_no) const;

  // 是否存在某设备
  bool hasDevice(const std::string &equip_no) const;

  // 返回当前所有设备的快照列表
  std::vector<DevicePtr> allDevices() const;

  // ============ 状态更新便捷接口 ============

  // 根据 equip_no 更新运行状态 / 自检结果
  // 如果设备不存在，当前选择静默忽略，也可以改成返回 bool 或抛异常
  void updateStatus(const std::string &equip_no, const DeviceStatus &status);
  void updateSelfCheck(const std::string &equip_no,
                       const SelfCheckResult &result);

private:
  using DeviceMap = std::unordered_map<std::string, DevicePtr>; // key: equip_no

  mutable std::mutex mutex_;
  DeviceMap devices_;
};

} // namespace device
