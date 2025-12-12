#include "watcher/online_status_watcher.h"
#include "client/redis_client.h"
#include <glog/logging.h>

namespace watcher {

OnlineStatusWatcher::OnlineStatusWatcher(qml_model::DeviceModel *device_model,
                                         QObject *parent)
    : QObject(parent), device_model_(device_model),
      poll_timer_(new QTimer(this)) {
  connect(poll_timer_, &QTimer::timeout, this,
          &OnlineStatusWatcher::onPollTimeout);
}

OnlineStatusWatcher::~OnlineStatusWatcher() { stop(); }

void OnlineStatusWatcher::start() {
  if (running_.load()) {
    LOG(WARNING) << "[OnlineStatusWatcher] Already running";
    return;
  }

  running_.store(true);
  poll_timer_->start(poll_interval_ms_);
  emit runningChanged();

  LOG(INFO) << "[OnlineStatusWatcher] Started with interval "
            << poll_interval_ms_ << "ms";

  // 启动时立即执行一次查询
  refreshNow();
}

void OnlineStatusWatcher::stop() {
  if (!running_.load()) {
    return;
  }

  poll_timer_->stop();
  running_.store(false);
  emit runningChanged();

  LOG(INFO) << "[OnlineStatusWatcher] Stopped";
}

void OnlineStatusWatcher::refreshNow() {
  // 在子线程中执行 Redis 查询，避免阻塞 UI
  std::thread([this]() { pollAllDevices(); }).detach();
}

void OnlineStatusWatcher::setPollIntervalMs(int ms) {
  if (ms < 1000) {
    ms = 1000; // 最小 1 秒
  }
  if (poll_interval_ms_ != ms) {
    poll_interval_ms_ = ms;
    if (running_.load()) {
      poll_timer_->setInterval(ms);
    }
    emit pollIntervalMsChanged();
  }
}

void OnlineStatusWatcher::onPollTimeout() {
  // 定时器触发，启动子线程查询
  std::thread([this]() { pollAllDevices(); }).detach();
}

void OnlineStatusWatcher::pollAllDevices() {
  if (device_model_ == nullptr) {
    LOG(ERROR) << "[OnlineStatusWatcher] DeviceModel is null";
    return;
  }

  auto *redis = client::RedisClient::GetInstance();
  if (redis == nullptr || !redis->IsConnected()) {
    DLOG(WARNING) << "[OnlineStatusWatcher] Redis not connected, skip polling";
    return;
  }

  // 获取所有设备的快照
  auto devices = device_model_->allDevices();
  if (devices.empty()) {
    return;
  }

  int updated_count = 0;

  for (const auto &device : devices) {
    const auto &attrs = device->Attributes();
    // TODO(@liangyu) 目前只处理充电桩 PILE
    if (attrs.type != device::DeviceTypes::kPILE) {
      continue;
    }

    bool is_online = queryOnlineStatus(attrs.station_no, attrs.equip_order);
    bool was_online = device->IsOnline();

    // 只在状态发生变化时才更新
    if (is_online != was_online) {
      const std::string equip_no = attrs.equip_no; // 拷贝用于 lambda 捕获
      // 通过 invokeMethod 在主线程更新 Model
      QMetaObject::invokeMethod(
          device_model_,
          [this, equip_no, is_online]() {
            device_model_->updateOnlineStatus(equip_no, is_online);
          },
          Qt::QueuedConnection);

      ++updated_count;
      LOG(INFO) << "[OnlineStatusWatcher] Device " << attrs.equip_no
                << " status changed: " << (is_online ? "Online" : "Offline");
    }
  }

  // 发出刷新完成信号
  if (updated_count > 0) {
    QMetaObject::invokeMethod(
        this, [this, updated_count]() { emit statusRefreshed(updated_count); },
        Qt::QueuedConnection);
  }
}

// static
bool OnlineStatusWatcher::queryOnlineStatus(const std::string &station_no,
                                            int equip_order) {
  auto *redis = client::RedisClient::GetInstance();
  if (redis == nullptr || !redis->IsConnected()) {
    return false;
  }

  // Redis Key 格式: objects:STA#{station_no}:PILE#{equip_order}
  // 示例: objects:STA#155261:PILE#1
  // Field: comm，值为 "true" 表示在线
  std::string key =
      "objects:STA#" + station_no + ":PILE#" + std::to_string(equip_order);

  auto value = redis->HGet(key, "comm");
  if (!value.has_value()) {
    // Key 或 Field 不存在，视为离线
    return false;
  }

  // "true" 表示在线，其他值视为离线
  return value.value() == "true";
}

} // namespace watcher
