#pragma once

#include "device/device_object.h"
#include "device/pile_object.h"
#include <QAbstractListModel>
#include <QString>
#include <qtmetamacros.h>
#include <utility>
#include <vector>

namespace qml_model {

class PileModel : public QAbstractListModel {
  Q_OBJECT
  Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
  Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
  Q_PROPERTY(QString lastError READ lastError NOTIFY errorChanged)
  Q_PROPERTY(QString deviceName READ deviceName NOTIFY countChanged)
  Q_PROPERTY(QString deviceId READ deviceId NOTIFY countChanged)
  Q_PROPERTY(QString deviceType READ deviceType NOTIFY countChanged)

public:
  enum Roles {
    CcuIndexRole = Qt::UserRole + 1,
    DeviceIdRole,
    DeviceNameRole,
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

  // 一次只会加载一条记录
  Q_INVOKABLE void loadFromHistory(const QString &recordId);

  /**
   * @brief Loads the latest check result for the specified device ID.
   *
   * 这个接口会读取当前设备ID的最新检查结果。
   * @param deviceId 设备的唯一标识符
   */
  Q_INVOKABLE void loadFromDevice(const QString &deviceId);
  bool loading() const { return loading_; }
  QString lastError() const { return last_error_; }

  QString deviceName() const {
    return QString::fromStdString(pile_attrs_.name);
  }
  QString deviceId() const {
    return QString::fromStdString(pile_attrs_.equip_no);
  }
  QString deviceType() const {
    return QString::fromStdString(device::DeviceTypeToString(pile_attrs_.type));
  }
  QString lastCheckTime() const {
    return QString::fromStdString(pile_attrs_.last_check_time);
  }

signals:
  void countChanged();
  void loadingChanged();
  void errorChanged(const QString &message);

private:
  device::PileAttributes pile_attrs_;
  bool loading_{false};
  QString last_error_;

  void resetWith(device::PileAttributes pile_attrs);
  void setLoading(bool loading);
  void setLastError(const QString &error);
  void loadAsyncByRecordId(const QString &recordId);
  void loadAsyncByDeviceId(const QString &deviceId);
};

} // namespace qml_model
