#include "model/pile_model.h"
#include "device/device_object.h"
#include "device/device_repo.h"

#include <QFutureWatcher>
#include <QVariant>
#include <QtConcurrent>
#include <glog/logging.h>
#include <utility>

namespace qml_model {

PileModel::PileModel(QObject *parent) : QAbstractListModel(parent) {}

// 计算所有 CCU 的总数
int PileModel::rowCount(const QModelIndex &parent) const {
  if (parent.isValid())
    return 0;

  int total = 0;
  for (const auto &pile : items_) {
    total += static_cast<int>(pile.ccu_attributes.size());
  }
  return total;
}

// 根据 row 找到对应的 (PileAttributes, CCUAttributes)
std::pair<const device::PileAttributes *, const device::CCUAttributes *>
PileModel::itemAt(int row) const {
  int offset = 0;
  for (const auto &pile : items_) {
    const int ccu_count = static_cast<int>(pile.ccu_attributes.size());
    if (row < offset + ccu_count) {
      return {&pile, &pile.ccu_attributes[row - offset]};
    }
    offset += ccu_count;
  }
  return {nullptr, nullptr};
}

QVariant PileModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid())
    return QVariant();

  const int row = index.row();
  auto [pile, ccu] = itemAt(row);
  if (pile == nullptr || ccu == nullptr) {
    return QVariant();
  }

  switch (role) {
  case CcuIndexRole:
    return ccu->index;
  case DeviceIdRole:
    return QString::fromStdString(pile->equip_no);
  case DeviceNameRole:
    return QString::fromStdString(pile->name);
  case DeviceTypeRole:
    return QString::fromStdString(device::DeviceTypeToString(pile->type));
  case LastCheckTimeRole:
    return QString::fromStdString(pile->last_check_time);
  case Ac1StuckRole:
    return ccu->ac_contactor_1.contactor1_stuck;
  case Ac1RefuseRole:
    return ccu->ac_contactor_1.contactor1_refuse;
  case Ac2StuckRole:
    return ccu->ac_contactor_2.contactor1_stuck;
  case Ac2RefuseRole:
    return ccu->ac_contactor_2.contactor1_refuse;
  case ParPosStuckRole:
    return ccu->parallel_contactor.positive_stuck;
  case ParPosRefuseRole:
    return ccu->parallel_contactor.positive_refuse;
  case ParNegStuckRole:
    return ccu->parallel_contactor.negative_stuck;
  case ParNegRefuseRole:
    return ccu->parallel_contactor.negative_refuse;
  case Fan1StoppedRole:
    return ccu->fan_1.stopped;
  case Fan1RotatingRole:
    return ccu->fan_1.rotating;
  case Fan2StoppedRole:
    return ccu->fan_2.stopped;
  case Fan2RotatingRole:
    return ccu->fan_2.rotating;
  case Fan3StoppedRole:
    return ccu->fan_3.stopped;
  case Fan3RotatingRole:
    return ccu->fan_3.rotating;
  case Fan4StoppedRole:
    return ccu->fan_4.stopped;
  case Fan4RotatingRole:
    return ccu->fan_4.rotating;
  case GunAPosStuckRole:
    return ccu->gun_a.positive_contactor_stuck;
  case GunAPosRefuseRole:
    return ccu->gun_a.positive_contactor_refuse;
  case GunANegStuckRole:
    return ccu->gun_a.negative_contactor_stuck;
  case GunANegRefuseRole:
    return ccu->gun_a.negative_contactor_refuse;
  case GunAUnlockedRole:
    return ccu->gun_a.unlocked;
  case GunALockedRole:
    return ccu->gun_a.locked;
  case GunAAux12Role:
    return ccu->gun_a.aux_power_12v;
  case GunAAux24Role:
    return ccu->gun_a.aux_power_24v;
  case GunBPosStuckRole:
    return ccu->gun_b.positive_contactor_stuck;
  case GunBPosRefuseRole:
    return ccu->gun_b.positive_contactor_refuse;
  case GunBNegStuckRole:
    return ccu->gun_b.negative_contactor_stuck;
  case GunBNegRefuseRole:
    return ccu->gun_b.negative_contactor_refuse;
  case GunBUnlockedRole:
    return ccu->gun_b.unlocked;
  case GunBLockedRole:
    return ccu->gun_b.locked;
  case GunBAux12Role:
    return ccu->gun_b.aux_power_12v;
  case GunBAux24Role:
    return ccu->gun_b.aux_power_24v;
  default:
    return QVariant();
  }
}

QHash<int, QByteArray> PileModel::roleNames() const {
  QHash<int, QByteArray> roles;
  roles[CcuIndexRole] = "ccuIndex";
  roles[DeviceIdRole] = "deviceId";
  roles[DeviceNameRole] = "deviceName";
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
  device::PileAttributes demo_pile;
  demo_pile.equip_no = "DEMO-0001";
  demo_pile.name = "演示设备";
  demo_pile.type = device::DeviceTypes::kPILE;
  demo_pile.last_check_time = "2025-01-01 00:00:00";

  device::CCUAttributes first;
  first.index = 1;
  first.ac_contactor_2.contactor1_stuck = true;
  first.parallel_contactor.negative_refuse = true;
  first.fan_3.stopped = true;
  first.gun_b.positive_contactor_refuse = true;
  demo_pile.ccu_attributes.push_back(first);

  device::CCUAttributes second;
  second.index = 2;
  second.ac_contactor_1.contactor1_stuck = true;
  second.ac_contactor_1.contactor1_refuse = true;
  second.gun_b.unlocked = true;
  demo_pile.ccu_attributes.push_back(second);

  std::vector<device::PileAttributes> items;
  items.push_back(std::move(demo_pile));
  resetWith(std::move(items));
}

void PileModel::loadFromHistory(const QString &recordId) {
  loadAsyncByRecordId(recordId);
}

void PileModel::loadFromDevice(const QString &deviceId) {
  loadAsyncByDeviceId(deviceId);
}

void PileModel::resetWith(std::vector<device::PileAttributes> &&items) {
  beginResetModel();
  items_ = std::move(items);
  endResetModel();
  emit countChanged();
}

void PileModel::setLoading(bool loading) {
  if (loading_ == loading)
    return;

  loading_ = loading;
  emit loadingChanged();
}

void PileModel::setLastError(const QString &error) {
  last_error_ = error;
  emit errorChanged(error);
}

void PileModel::loadAsyncByRecordId(const QString &recordId) {
  if (loading_) {
    return;
  }

  setLastError({});
  setLoading(true);

  auto *watcher =
      new QFutureWatcher<absl::StatusOr<device::PileAttributes>>(this);

  connect(watcher,
          &QFutureWatcher<absl::StatusOr<device::PileAttributes>>::finished,
          this, [this, watcher]() {
            auto result = watcher->future().result();
            if (!result.ok()) {
              setLastError(QString::fromStdString(result.status().ToString()));
              setLoading(false);
              watcher->deleteLater();
              return;
            }

            std::vector<device::PileAttributes> items;
            items.push_back(std::move(result).value());
            resetWith(std::move(items));
            setLoading(false);
            watcher->deleteLater();
          });

  watcher->setFuture(QtConcurrent::run(
      [recordId]() { return device::DeviceRepo::GetPileItems(recordId); }));
}

void PileModel::loadAsyncByDeviceId(const QString &deviceId) {
  if (loading_) {
    return;
  }

  setLastError({});
  setLoading(true);

  auto *watcher =
      new QFutureWatcher<absl::StatusOr<device::PileAttributes>>(this);

  connect(watcher,
          &QFutureWatcher<absl::StatusOr<device::PileAttributes>>::finished,
          this, [this, watcher]() {
            auto result = watcher->future().result();
            if (!result.ok()) {
              setLastError(QString::fromStdString(result.status().ToString()));
              setLoading(false);
              watcher->deleteLater();
              return;
            }

            std::vector<device::PileAttributes> items;
            items.push_back(std::move(result).value());
            resetWith(std::move(items));
            setLoading(false);
            watcher->deleteLater();
          });

  watcher->setFuture(QtConcurrent::run([deviceId]() {
    return device::DeviceRepo::GetLatestPileItems(deviceId);
  }));
}

} // namespace qml_model
