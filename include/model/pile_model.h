#pragma once

#include "device/device_object.h"
#include <QAbstractListModel>
#include <QString>
#include <qtmetamacros.h>
#include <vector>

namespace qml_model {

class PileModel : public QAbstractListModel {
  Q_OBJECT
  Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
  enum Roles {
    CcuIndexRole = Qt::UserRole + 1,
    DeviceIdRole,
    DeviceTypeRole,
    LastCheckTimeRole,
    Ac1StuckRole,
    Ac1RefuseRole,
    Ac2StuckRole,
    Ac2RefuseRole,
    ParPosStuckRole,
    ParPosRefuseRole,
    ParNegStuckRole,
    ParNegRefuseRole,
    Fan1StoppedRole,
    Fan1RotatingRole,
    Fan2StoppedRole,
    Fan2RotatingRole,
    Fan3StoppedRole,
    Fan3RotatingRole,
    Fan4StoppedRole,
    Fan4RotatingRole,
    GunAPosStuckRole,
    GunAPosRefuseRole,
    GunANegStuckRole,
    GunANegRefuseRole,
    GunAUnlockedRole,
    GunALockedRole,
    GunAAux12Role,
    GunAAux24Role,
    GunBPosStuckRole,
    GunBPosRefuseRole,
    GunBNegStuckRole,
    GunBNegRefuseRole,
    GunBUnlockedRole,
    GunBLockedRole,
    GunBAux12Role,
    GunBAux24Role
  };
  Q_ENUM(Roles)

  explicit PileModel(QObject *parent = nullptr);

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index, int role) const override;
  QHash<int, QByteArray> roleNames() const override;

  Q_INVOKABLE void loadDemo();
  Q_INVOKABLE void loadFromHistory(const QString &recordId);

  /**
   * @brief Loads the latest check result for the specified device ID.
   *
   * 这个接口会读取当前设备ID的最新检查结果。
   * @param deviceId 设备的唯一标识符
   */
  Q_INVOKABLE void loadFromDevice(const QString &deviceId);

signals:
  void countChanged();

private:
  std::vector<device::CCUAttributes> items_;
  void resetWith(std::vector<device::CCUAttributes> &&items);
};

} // namespace qml_model
