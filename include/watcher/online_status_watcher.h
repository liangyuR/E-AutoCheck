#pragma once

#include "model/device_model.h"
#include <QObject>
#include <QTimer>
#include <atomic>
#include <thread>

namespace watcher {

// OnlineStatusWatcher: 周期性从 Redis 查询设备在线状态，更新 DeviceModel
// 设计原则:
// - 单一职责: 只负责在线状态的监控和同步
// - 线程安全: Redis 查询在子线程，UI 更新在主线程
// - 可配置: 轮询间隔可调
class OnlineStatusWatcher : public QObject {
  Q_OBJECT

  // 轮询间隔（毫秒），QML 可读写
  Q_PROPERTY(int pollIntervalMs READ pollIntervalMs WRITE setPollIntervalMs
                 NOTIFY pollIntervalMsChanged)
  // 是否正在运行
  Q_PROPERTY(bool running READ isRunning NOTIFY runningChanged)

public:
  explicit OnlineStatusWatcher(qml_model::DeviceModel *device_model,
                               QObject *parent = nullptr);
  ~OnlineStatusWatcher() override;

  OnlineStatusWatcher(const OnlineStatusWatcher &) = delete;
  OnlineStatusWatcher &operator=(const OnlineStatusWatcher &) = delete;

  // 启动/停止监控
  Q_INVOKABLE void start();
  Q_INVOKABLE void stop();

  // 手动触发一次刷新
  Q_INVOKABLE void refreshNow();

  int pollIntervalMs() const { return poll_interval_ms_; }
  void setPollIntervalMs(int ms);

  bool isRunning() const { return running_.load(); }

signals:
  void pollIntervalMsChanged();
  void runningChanged();
  // 当状态更新完成时发出（可选，用于 UI 显示刷新时间等）
  void statusRefreshed(int updatedCount);

private slots: // NOLINT(readability-redundant-access-specifiers)
  void onPollTimeout();

private:
  // 从 Redis 查询单个设备的在线状态（目前仅支持 PILE 类型）
  // Redis Key 格式: objects:STA#{station_no}:PILE#{equip_order}
  // Field: comm，值为 "true" 表示在线
  static bool queryOnlineStatus(const std::string &station_no, int equip_order);

  // 批量查询所有设备的在线状态（在子线程执行）
  void pollAllDevices();

  qml_model::DeviceModel *device_model_;
  QTimer *poll_timer_;
  int poll_interval_ms_ = 5000; // 默认 5 秒
  std::atomic<bool> running_{false};
};

} // namespace watcher
