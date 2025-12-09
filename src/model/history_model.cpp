#include "model/history_model.h"
#include "device/device_repo.h"

#include <QVariant>

namespace qml_model {
HistoryModel::HistoryModel(QObject *parent) : QAbstractListModel(parent) {}

int HistoryModel::rowCount(const QModelIndex &parent) const {
  if (parent.isValid())
    return 0;
  return static_cast<int>(items_.size());
}

QVariant HistoryModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid())
    return QVariant();

  const int row = index.row();
  if (row < 0 || row >= static_cast<int>(items_.size()))
    return QVariant();

  const auto &item = items_[row];
  switch (role) {
  case RecordIdRole:
    return item.recordId;
  case DeviceIdRole:
    return item.deviceId;
  case TimestampRole:
    return item.timestamp;
  case TimestampDisplayRole:
    return item.timestamp.toString(Qt::ISODate);
  case StatusRole:
    return item.status;
  case SummaryRole:
    return item.summary;
  default:
    return QVariant();
  }
}

QHash<int, QByteArray> HistoryModel::roleNames() const {
  QHash<int, QByteArray> roles;
  roles[RecordIdRole] = "recordId";
  roles[DeviceIdRole] = "deviceId";
  roles[TimestampRole] = "timestamp";
  roles[TimestampDisplayRole] = "timestampDisplay";
  roles[StatusRole] = "status";
  roles[SummaryRole] = "summary";
  return roles;
}

void HistoryModel::load(const QString &deviceId, int limit) {
  if (loading_)
    return;

  setLoading(true);
  auto result = device::DeviceRepo::GetHistoryItems(deviceId, limit);
  if (!result.ok()) {
    emit errorChanged(QString::fromStdString(result.status().ToString()));
    setLoading(false);
    return;
  }

  setItems(std::move(result.value()));
  setLoading(false);
}

QVariant HistoryModel::get(int row) const {
  if (row < 0 || row >= static_cast<int>(items_.size()))
    return QVariant();

  const auto &item = items_[row];
  QVariantMap map;
  map["recordId"] = item.recordId;
  map["deviceId"] = item.deviceId;
  map["timestamp"] = item.timestamp;
  map["timestampDisplay"] = item.timestamp.toString(Qt::ISODate);
  map["status"] = item.status;
  map["summary"] = item.summary;
  return map;
}

void HistoryModel::setLoading(bool v) {
  loading_ = v;
  emit loadingChanged();
}

void HistoryModel::setItems(std::vector<HistoryItem> &&items) {
  beginResetModel();
  items_ = std::move(items);
  endResetModel();
}
} // namespace qml_model