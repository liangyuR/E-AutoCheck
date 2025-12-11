#include "model/device_model.h"
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
  case NameEnRole:
    return QString::fromStdString(attrs.name_en);
  case EquipNoRole:
    return QString::fromStdString(attrs.equip_no);
  case StationNoRole:
    return QString::fromStdString(attrs.station_no);
  case TypeRole:
    return QString::fromStdString(attrs.type);
  case IpAddrRole:
    return QString::fromStdString(attrs.ip_addr);
  case StatusRole:
    // TODO: Return status code or enum value
    return 0;
  case IsOnlineRole:
    return device->IsOnline();
  case StatusTextRole:
    return QString::fromStdString(device->CurrentSelfCheckDesc());
  case IsCheckingRole:
    return device->IsSelfChecking();
  case LastCheckTimeRole:
    return QString::fromStdString(device->LastCheckTime());
  }

  return {};
}

QHash<int, QByteArray> DeviceModel::roleNames() const {
  QHash<int, QByteArray> roles;
  roles[NameRole] = "name";
  roles[NameEnRole] = "nameEn";
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
DeviceModel::addDevice(const device::PileAttr &attrs) {
  // 注意：必须在持有锁之前创建 shared_ptr，但锁的范围需要覆盖 map 和 vector
  // 操作 因为 addDevice 可能在后台线程调用，而 UI 线程在读取 QAbstractListModel
  // 的 begin/endInsertRows 只能在 UI 线程调用吗？
  // 不，它们发出的信号是线程安全的，但是 Model 的内部数据结构必须受保护。
  // 实际上，如果是在后台线程调用 beginInsertRows，信号连接到 UI
  // 可能会有跨线程问题。 但 Qt
  // 的信号槽机制处理了跨线程连接（QueuedConnection）。 最安全的做法是确保
  // addDevice 也是在主线程调用的（现在的 main.cpp 逻辑正是如此，invokeMethod
  // 到了主线程）。

  DLOG(INFO) << "addDevice: " << attrs;
  auto device = std::make_shared<device::PileDevice>(attrs);
  const auto &key = device->Id();

  // 我们需要确保 beginInsertRows/endInsertRows 包裹住数据变更
  // 但是这一步必须在获取锁的情况下进行吗？
  // Qt Model 的惯例是：修改数据前调用 begin，修改后调用 end。
  // 这里的锁是用来保护 device_list_ 和 device_map_ 的。

  // 检查是否存在
  {
    // 先检查是否存在，如果存在则是更新操作
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = device_map_.find(key);
    if (it != device_map_.end()) {
      // 已存在，更新（替换）
      // 找到在 list 中的位置
      for (size_t i = 0; i < device_list_.size(); ++i) {
        if (device_list_[i]->Id() == key) {
          device_list_[i] = device;
          device_map_[key] = device;
          // 释放锁后发送信号？不，dataChanged
          // 是信号，可以在锁内发，但最好尽早释放
          // 这里简单处理，锁内发也没事，因为是 invokeMethod 调用的，在主线程
          QModelIndex idx = index(static_cast<int>(i));
          // const_cast 因为 dataChanged 是非 const 的，但在 const
          // 成员函数中不能调？ addDevice 不是 const 的，没问题。
          // 但由于我们手动加了锁，要小心死锁。这里没调外部代码，安全。
          const_cast<DeviceModel *>(this)->dataChanged(idx, idx);
          LOG(INFO) << "Device updated: " << key;
          return device;
        }
      }
    }
  }

  // 是新设备
  // beginInsertRows 会触发 View 的响应，View 会尝试 lock rowCount/data
  // 所以 beginInsertRows 应该在 lock 之外？
  // 不，beginInsertRows 只是发信号通知 "将要插入"。
  // 真正的数据插入应该在 begin 和 end 之间。
  // 如果在 begin 和 end 之间，View 试图访问怎么办？
  // 通常 View 只在 endInsertRows 之后（收到 rowsInserted 信号）才会刷新。

  int row = 0;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    row = static_cast<int>(device_list_.size());
  }

  beginInsertRows(QModelIndex(), row, row);
  {
    std::lock_guard<std::mutex> lock(mutex_);
    device_list_.push_back(device);
    device_map_[key] = device;
  }
  endInsertRows();

  LOG(INFO) << "Device added: " << key;
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
  // 这里需要通知 Model 更新
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
      if (device_list_[i]->Id() == equip_no) {
        row = static_cast<int>(i);
        break;
      }
    }
  }

  if (row >= 0) {
    QModelIndex idx = index(row);
    // 仅通知状态相关的 Role 发生了变化
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
      if (device_list_[i]->Id() == equip_no) {
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
      if (device_list_[i]->Id() == equip_no) {
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
    device::DeviceStatus status = it->second->Status();
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
      if (device_list_[i]->Id() == equip_no) {
        row = static_cast<int>(i);
        break;
      }
    }
  }

  if (row >= 0) {
    QModelIndex idx = index(row);
    // 仅通知在线状态相关的 Role 发生了变化
    emit dataChanged(idx, idx, {IsOnlineRole, StatusTextRole});
    DLOG(INFO) << "Device " << equip_no << " online status changed to: "
               << (is_online ? "Online" : "Offline");
  }
}

} // namespace qml_model
