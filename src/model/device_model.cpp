#include "model/device_model.h"
#include "device/device_object.h"
#include <glog/logging.h>

namespace qml_model {

DeviceModel::DeviceModel(QObject *parent) : QAbstractListModel(parent) {}

int DeviceModel::rowCount(const QModelIndex &parent) const {
  if (parent.isValid())
    return 0;
  std::lock_guard<std::mutex> lock(mutex_);
  return static_cast<int>(device_list_.size());
}

QVariant DeviceModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid())
    return {};

  std::lock_guard<std::mutex> lock(mutex_);
  const int row = index.row();
  if (row < 0 || row >= static_cast<int>(device_list_.size()))
    return {};

  const auto &device = device_list_[row];
  const auto &attrs = device->Attributes();

  switch (role) {
  case NameRole:
    return QString::fromStdString(attrs.name);
  case EquipNoRole:
    return QString::fromStdString(attrs.equip_no);
  case StationNoRole:
    return QString::fromStdString(attrs.station_no);
  case TypeRole:
    return QString::fromStdString(device::DeviceTypeToString(attrs.type));
  case IpAddrRole:
    return QString::fromStdString(attrs.ip_addr);
  case StatusRole:
    // TODO: Return status code or enum value
    return 0;
  case IsOnlineRole:
    return device->IsOnline();
  case StatusTextRole:
    // 如果有模块检测结果，返回最后一个模块的消息；否则返回默认文本
    if (!attrs.self_check_result.modules.empty()) {
      return QString::fromStdString(
          attrs.self_check_result.modules.back().message);
    }
    return QString("等待自检");
  case IsCheckingRole:
    return attrs.self_check_result.status == device::SelfCheckStatus::Running;
  case LastCheckTimeRole:
    return QString::fromStdString(attrs.self_check_result.last_check_time_str);
  }

  return {};
}

QHash<int, QByteArray> DeviceModel::roleNames() const {
  QHash<int, QByteArray> roles;
  roles[NameRole] = "name";
  roles[EquipNoRole] = "equipNo";
  roles[StationNoRole] = "stationNo";
  roles[TypeRole] = "type";
  roles[IpAddrRole] = "ipAddr";
  roles[StatusRole] = "status";
  roles[IsOnlineRole] = "isOnline";
  roles[StatusTextRole] = "statusText";
  roles[IsCheckingRole] = "isChecking";
  roles[LastCheckTimeRole] = "lastCheckTime";
  return roles;
}

DeviceModel::PileDevicePtr
DeviceModel::addDevice(const device::DeviceAttr &attrs) {
  DLOG(INFO) << "addDevice: " << attrs;
  auto device = std::make_shared<device::PileDevice>(attrs);
  const auto &equip_no = device->Attributes().equip_no;

  {
    // 先检查是否存在，如果存在则是更新操作
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = device_map_.find(equip_no);
    if (it != device_map_.end()) {
      for (size_t i = 0; i < device_list_.size(); ++i) {
        if (device_list_[i]->Attributes().equip_no == equip_no) {
          device_list_[i] = device;
          device_map_[equip_no] = device;
          QModelIndex idx = index(static_cast<int>(i));
          const_cast<DeviceModel *>(this)->dataChanged(idx, idx);
          LOG(INFO) << "Device updated: " << equip_no;
          return device;
        }
      }
    }
  }

  // 是新设备
  int row = 0;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    row = static_cast<int>(device_list_.size());
  }

  beginInsertRows(QModelIndex(), row, row);
  {
    std::lock_guard<std::mutex> lock(mutex_);
    device_list_.push_back(device);
    device_map_[equip_no] = device;
  }
  endInsertRows();

  LOG(INFO) << "Device added: " << equip_no;
  return device;
}

DeviceModel::PileDevicePtr
DeviceModel::getDeviceByEquipNo(const std::string &equip_no) const {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = device_map_.find(equip_no);
  if (it == device_map_.end()) {
    return nullptr;
  }
  return it->second;
}

bool DeviceModel::hasDevice(const std::string &equip_no) const {
  std::lock_guard<std::mutex> lock(mutex_);
  return device_map_.find(equip_no) != device_map_.end();
}

std::vector<DeviceModel::PileDevicePtr> DeviceModel::allDevices() const {
  std::lock_guard<std::mutex> lock(mutex_);
  // 直接拷贝 vector
  return device_list_;
}

void DeviceModel::updateStatus(const std::string &equip_no,
                               const device::DeviceStatus &status) {
  int row = -1;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = device_map_.find(equip_no);
    if (it == device_map_.end()) {
      LOG(WARNING) << "updateStatus: device not found, equip_no=" << equip_no;
      return;
    }
    it->second->UpdateStatus(status);

    // 查找 row index 以发送信号
    // 简单的线性查找
    for (size_t i = 0; i < device_list_.size(); ++i) {
      if (device_list_[i]->Attributes().equip_no == equip_no) {
        row = static_cast<int>(i);
        break;
      }
    }
  }

  if (row >= 0) {
    QModelIndex idx = index(row);
    emit dataChanged(idx, idx, {StatusRole, IsOnlineRole});
  }
}

void DeviceModel::updateSelfCheck(const std::string &equip_no,
                                  const device::SelfCheckResult &result) {
  int row = -1;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = device_map_.find(equip_no);
    if (it == device_map_.end()) {
      LOG(WARNING) << "updateSelfCheck: device not found, equip_no="
                   << equip_no;
      return;
    }
    it->second->UpdateSelfCheck(result);

    // 查找 row index 以发送信号
    for (size_t i = 0; i < device_list_.size(); ++i) {
      if (device_list_[i]->Attributes().equip_no == equip_no) {
        row = static_cast<int>(i);
        break;
      }
    }
  }

  if (row >= 0) {
    QModelIndex idx = index(row);
    // 通知最后自检时间相关的 Role 发生了变化
    emit dataChanged(idx, idx, {LastCheckTimeRole, StatusTextRole});
  }
}

void DeviceModel::updateSelfCheckProgress(const std::string &equip_no,
                                          const std::string &desc,
                                          bool is_checking) {
  int row = -1;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = device_map_.find(equip_no);
    if (it == device_map_.end()) {
      LOG(WARNING) << "updateSelfCheckProgress: device not found, equip_no="
                   << equip_no;
      return;
    }
    it->second->UpdateSelfCheckProgress(desc, is_checking);

    // 查找 row index 以发送信号
    for (size_t i = 0; i < device_list_.size(); ++i) {
      if (device_list_[i]->Attributes().equip_no == equip_no) {
        row = static_cast<int>(i);
        break;
      }
    }
  }

  if (row >= 0) {
    QModelIndex idx = index(row);
    // 通知状态文本和检查状态相关的 Role 发生了变化
    emit dataChanged(idx, idx, {StatusTextRole, IsCheckingRole});
  }
}

void DeviceModel::updateOnlineStatus(const std::string &equip_no,
                                     bool is_online) {
  int row = -1;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = device_map_.find(equip_no);
    if (it == device_map_.end()) {
      LOG(WARNING) << "updateOnlineStatus: device not found, equip_no="
                   << equip_no;
      return;
    }

    // 获取当前状态，仅修改在线字段
    device::DeviceStatus status = it->second->Attributes().device_state;
    device::OnlineState new_state =
        is_online ? device::OnlineState::Online : device::OnlineState::Offline;

    // 只有状态变化时才更新
    if (status.online_state == new_state) {
      return;
    }

    status.online_state = new_state;
    status.last_update = std::chrono::system_clock::now();
    it->second->UpdateStatus(status);

    // 查找 row index 以发送信号
    for (size_t i = 0; i < device_list_.size(); ++i) {
      if (device_list_[i]->Attributes().equip_no == equip_no) {
        row = static_cast<int>(i);
        break;
      }
    }
  }

  if (row >= 0) {
    QModelIndex idx = index(row);
    emit dataChanged(idx, idx, {IsOnlineRole, StatusTextRole});
    DLOG(INFO) << "Device " << equip_no << " online status changed to: "
               << (is_online ? "Online" : "Offline");
  }
}

} // namespace qml_model
