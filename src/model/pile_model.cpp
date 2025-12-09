#include "model/pile_model.h"
#include "device/device_object.h"
#include "device/device_repo.h"

#include <QVariant>
#include <glog/logging.h>

namespace qml_model {

PileModel::PileModel(QObject *parent) : QAbstractListModel(parent) {}

int PileModel::rowCount(const QModelIndex &parent) const {
  if (parent.isValid())
    return 0;
  return static_cast<int>(items_.size());
}

QVariant PileModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid())
    return QVariant();

  const int row = index.row();
  if (row < 0 || row >= static_cast<int>(items_.size()))
    return QVariant();

  const auto &item = items_[row];
  switch (role) {
  case CcuIndexRole:
    return item.index;
  case DeviceIdRole:
    return QString::fromStdString(item.device_id);
  case DeviceTypeRole:
    return QString::fromStdString(item.device_type);
  case LastCheckTimeRole:
    return QString::fromStdString(item.last_check_time);
  case Ac1StuckRole:
    return item.ac_contactor_1.contactor1_stuck;
  case Ac1RefuseRole:
    return item.ac_contactor_1.contactor1_refuse;
  case Ac2StuckRole:
    return item.ac_contactor_2.contactor1_stuck;
  case Ac2RefuseRole:
    return item.ac_contactor_2.contactor1_refuse;
  case ParPosStuckRole:
    return item.parallel_contactor.positive_stuck;
  case ParPosRefuseRole:
    return item.parallel_contactor.positive_refuse;
  case ParNegStuckRole:
    return item.parallel_contactor.negative_stuck;
  case ParNegRefuseRole:
    return item.parallel_contactor.negative_refuse;
  case Fan1StoppedRole:
    return item.fan_1.stopped;
  case Fan1RotatingRole:
    return item.fan_1.rotating;
  case Fan2StoppedRole:
    return item.fan_2.stopped;
  case Fan2RotatingRole:
    return item.fan_2.rotating;
  case Fan3StoppedRole:
    return item.fan_3.stopped;
  case Fan3RotatingRole:
    return item.fan_3.rotating;
  case Fan4StoppedRole:
    return item.fan_4.stopped;
  case Fan4RotatingRole:
    return item.fan_4.rotating;
  case GunAPosStuckRole:
    return item.gun_a.positive_contactor_stuck;
  case GunAPosRefuseRole:
    return item.gun_a.positive_contactor_refuse;
  case GunANegStuckRole:
    return item.gun_a.negative_contactor_stuck;
  case GunANegRefuseRole:
    return item.gun_a.negative_contactor_refuse;
  case GunAUnlockedRole:
    return item.gun_a.unlocked;
  case GunALockedRole:
    return item.gun_a.locked;
  case GunAAux12Role:
    return item.gun_a.aux_power_12v;
  case GunAAux24Role:
    return item.gun_a.aux_power_24v;
  case GunBPosStuckRole:
    return item.gun_b.positive_contactor_stuck;
  case GunBPosRefuseRole:
    return item.gun_b.positive_contactor_refuse;
  case GunBNegStuckRole:
    return item.gun_b.negative_contactor_stuck;
  case GunBNegRefuseRole:
    return item.gun_b.negative_contactor_refuse;
  case GunBUnlockedRole:
    return item.gun_b.unlocked;
  case GunBLockedRole:
    return item.gun_b.locked;
  case GunBAux12Role:
    return item.gun_b.aux_power_12v;
  case GunBAux24Role:
    return item.gun_b.aux_power_24v;
  default:
    return QVariant();
  }
}

QHash<int, QByteArray> PileModel::roleNames() const {
  QHash<int, QByteArray> roles;
  roles[CcuIndexRole] = "ccuIndex";
  roles[DeviceIdRole] = "deviceId";
  roles[DeviceTypeRole] = "deviceType";
  roles[LastCheckTimeRole] = "lastCheckTime";
  roles[Ac1StuckRole] = "ac1_stuck";
  roles[Ac1RefuseRole] = "ac1_refuse";
  roles[Ac2StuckRole] = "ac2_stuck";
  roles[Ac2RefuseRole] = "ac2_refuse";
  roles[ParPosStuckRole] = "par_pos_stuck";
  roles[ParPosRefuseRole] = "par_pos_refuse";
  roles[ParNegStuckRole] = "par_neg_stuck";
  roles[ParNegRefuseRole] = "par_neg_refuse";
  roles[Fan1StoppedRole] = "fan1_stopped";
  roles[Fan1RotatingRole] = "fan1_rotating";
  roles[Fan2StoppedRole] = "fan2_stopped";
  roles[Fan2RotatingRole] = "fan2_rotating";
  roles[Fan3StoppedRole] = "fan3_stopped";
  roles[Fan3RotatingRole] = "fan3_rotating";
  roles[Fan4StoppedRole] = "fan4_stopped";
  roles[Fan4RotatingRole] = "fan4_rotating";
  roles[GunAPosStuckRole] = "gunA_pos_stuck";
  roles[GunAPosRefuseRole] = "gunA_pos_refuse";
  roles[GunANegStuckRole] = "gunA_neg_stuck";
  roles[GunANegRefuseRole] = "gunA_neg_refuse";
  roles[GunAUnlockedRole] = "gunA_unlocked";
  roles[GunALockedRole] = "gunA_locked";
  roles[GunAAux12Role] = "gunA_aux12";
  roles[GunAAux24Role] = "gunA_aux24";
  roles[GunBPosStuckRole] = "gunB_pos_stuck";
  roles[GunBPosRefuseRole] = "gunB_pos_refuse";
  roles[GunBNegStuckRole] = "gunB_neg_stuck";
  roles[GunBNegRefuseRole] = "gunB_neg_refuse";
  roles[GunBUnlockedRole] = "gunB_unlocked";
  roles[GunBLockedRole] = "gunB_locked";
  roles[GunBAux12Role] = "gunB_aux12";
  roles[GunBAux24Role] = "gunB_aux24";
  return roles;
}

void PileModel::loadDemo() {
  std::vector<device::CCUAttributes> demo;

  device::CCUAttributes first;
  first.index = 1;
  first.device_id = "DEMO-0001";
  first.device_type = "PILE";
  first.last_check_time = "";
  first.ac_contactor_2.contactor1_stuck = true;
  first.parallel_contactor.negative_refuse = true;
  first.fan_3.stopped = true;
  first.gun_b.positive_contactor_refuse = true;
  demo.push_back(first);

  device::CCUAttributes second;
  second.index = 2;
  second.device_id = "DEMO-0001";
  second.device_type = "PILE";
  second.last_check_time = "";
  second.ac_contactor_1.contactor1_stuck = true;
  second.ac_contactor_1.contactor1_refuse = true;
  second.gun_b.unlocked = true;
  demo.push_back(second);

  resetWith(std::move(demo));
}

void PileModel::loadFromHistory(const QString &recordId) {
  auto result = device::DeviceRepo::GetPileItems(recordId);
  if (!result.ok()) {
    LOG(ERROR) << "获取历史记录失败: " << result.status().ToString();
    return;
  }
  resetWith(std::move(result.value()));
}

void PileModel::resetWith(std::vector<device::CCUAttributes> &&items) {
  beginResetModel();
  items_ = std::move(items);
  endResetModel();
  emit countChanged();
}

} // namespace qml_model
