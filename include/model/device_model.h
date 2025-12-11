#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "device/pile_device.h"
#include <QAbstractListModel>
#include <absl/status/status.h>

namespace qml_model {

// 充电桩设备

class DeviceModel : public QAbstractListModel {
  Q_OBJECT

public:
  enum DeviceRoles {
    NameRole = Qt::UserRole + 1,
    NameEnRole,
    EquipNoRole,
    StationNoRole,
    TypeRole,
    IpAddrRole,
    StatusRole,
    IsOnlineRole,
    StatusTextRole,
    IsCheckingRole,
    LastCheckTimeRole
  };

  using PileDevicePtr = std::shared_ptr<device::PileDevice>;

  explicit DeviceModel(QObject *parent = nullptr);
  ~DeviceModel() override = default;

  DeviceModel(const DeviceModel &) = delete;
  DeviceModel &operator=(const DeviceModel &) = delete;

  // ============ QAbstractListModel 接口 ============
  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index,
                int role = Qt::DisplayRole) const override;
  QHash<int, QByteArray> roleNames() const override;

  // ============ 设备增删查 ============
  // 从属性创建一台设备并接管其生命周期。
  PileDevicePtr addDevice(const device::PileAttr &attrs);

  // 按业务 ID（equip_no）获取设备，找不到返回 nullptr
  PileDevicePtr getDeviceByEquipNo(const std::string &equip_no) const;

  // 是否存在某设备
  bool hasDevice(const std::string &equip_no) const;

  // 返回当前所有设备的快照列表
  std::vector<PileDevicePtr> allDevices() const;

  // ============ 状态更新便捷接口 ============
  void updateStatus(const std::string &equip_no,
                    const device::DeviceStatus &status);
  void updateSelfCheck(const std::string &equip_no,
                       const device::SelfCheckResult &result);
  void updateSelfCheckProgress(const std::string &equip_no,
                               const std::string &desc, bool is_checking);

  // 更新单个设备的在线状态
  void updateOnlineStatus(const std::string &equip_no, bool is_online);

private:
  mutable std::mutex mutex_;
  std::vector<PileDevicePtr> device_list_;
  std::unordered_map<std::string, PileDevicePtr> device_map_; // key: equip_no
};

} // namespace qml_model
