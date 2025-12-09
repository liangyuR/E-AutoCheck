// SelfCheckHistoryModel.h
#pragma once

#include <QAbstractListModel>
#include <QDateTime>
#include <vector>

namespace qml_model {
struct HistoryItem {
  QString recordId;    // MySQL 主键 ID（推荐）
  QString deviceId;    // 设备编号
  QDateTime timestamp; // 自检时间
  QString status;      // "normal" / "warning" / "error"
  QString summary;     // 简要描述，比如 "正常: 5, 异常: 1"
};

class HistoryModel : public QAbstractListModel {
  Q_OBJECT

public:
  enum Roles {
    RecordIdRole = Qt::UserRole + 1,
    DeviceIdRole,
    TimestampRole,
    TimestampDisplayRole,
    StatusRole,
    SummaryRole
  };
  Q_ENUM(Roles)

  explicit HistoryModel(QObject *parent = nullptr);

  // QAbstractListModel 必备
  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index, int role) const override;
  QHash<int, QByteArray> roleNames() const override;

  // 给 QML 用的懒加载接口
  Q_INVOKABLE void load(const QString &deviceId, int limit = 10);
  Q_INVOKABLE QVariant get(int row) const;

signals:
  void loadingChanged();
  void errorChanged(const QString &message);

private:
  std::vector<HistoryItem> items_;
  bool loading_{false};

  void setLoading(bool v);
  void setItems(std::vector<HistoryItem> &&items);
};
} // namespace qml_model
