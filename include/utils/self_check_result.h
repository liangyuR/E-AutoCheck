#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace selfcheck {

// 统一时间戳类型约定：
// - 使用 ISO 8601 格式，UTC 时间，例如："2025-11-28T07:20:05Z"
// - 与云平台 / 日志系统保持一致，便于跨系统解析
using Timestamp = std::string;

enum class StatusLevel {
  kUnknown = 0,
  kOk,
  kWarn,
  kError,
};

// 将 StatusLevel 转为字符串（用于日志 / JSON 序列化）
inline const char *ToString(StatusLevel status) {
  switch (status) {
  case StatusLevel::kOk:
    return "ok";
  case StatusLevel::kWarn:
    return "warn";
  case StatusLevel::kError:
    return "error";
  case StatusLevel::kUnknown:
  default:
    return "unknown";
  }
}

// 从字符串解析 StatusLevel（用于 JSON 反序列化），大小写不敏感
inline StatusLevel StatusFromString(const std::string &s) {
  // 简单实现：统一转小写再比较
  std::string lower;
  lower.reserve(s.size());
  for (char c : s) {
    if (c >= 'A' && c <= 'Z') {
      lower.push_back(static_cast<char>(c - 'A' + 'a'));
    } else {
      lower.push_back(c);
    }
  }

  if (lower == "ok") {
    return StatusLevel::kOk;
  }
  if (lower == "warn" || lower == "warning") {
    return StatusLevel::kWarn;
  }
  if (lower == "error" || lower == "err" || lower == "fail") {
    return StatusLevel::kError;
  }
  return StatusLevel::kUnknown;
}

// 用于 UI 映射 / 统计：数值越大表示严重程度越高
inline int StatusSeverity(StatusLevel status) {
  switch (status) {
  case StatusLevel::kOk:
    return 0;
  case StatusLevel::kWarn:
    return 1;
  case StatusLevel::kError:
    return 2;
  case StatusLevel::kUnknown:
  default:
    return -1; // 未知单独归类
  }
}

// 结构化 details：key-value 形式，便于 UI 展示和 JSON 转换
using DetailsMap = std::unordered_map<std::string, std::string>;

struct ModuleStatus {
  // 模块名称（逻辑标识，可同时用于展示与协议字段）
  std::string module_name;

  StatusLevel status = StatusLevel::kUnknown;

  // 设备侧上报的状态码，例如 COMM_OK / MCU_TEMP_HIGH
  std::string code;

  // 人类可读的描述信息
  std::string message;

  // 该模块状态的时间戳（UTC, ISO8601）
  Timestamp timestamp;

  // 结构化的详情信息，例如：{"temp":"82.1","limit":"85"}
  DetailsMap details;
};

struct SelfCheckResult {
  // 设备类型，例如："charger_box" / "swap_station"
  std::string device_type;

  // 设备唯一标识
  std::string device_id;

  // 本次自检请求的唯一 ID，用于关联命令与结果
  std::string request_id;

  // 总体状态
  StatusLevel overall_status = StatusLevel::kUnknown;

  // 本次自检结果生成时间（UTC, ISO8601）
  Timestamp timestamp;

  // 子模块状态列表
  std::vector<ModuleStatus> modules;
};

} // namespace selfcheck
