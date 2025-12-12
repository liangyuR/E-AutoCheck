#pragma once

#include <chrono>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace device {

enum class DeviceTypes : int { kUNKNOWN = 0, kPILE = 1, kSTACK = 2 };

inline std::string DeviceTypeToString(DeviceTypes type) {
  switch (type) {
  case DeviceTypes::kPILE:
    return "PILE";
  case DeviceTypes::kSTACK:
    return "STACK";
  default:
    return "UNKNOWN";
  }
}

inline DeviceTypes DeviceTypeFromString(const std::string &type) {
  if (type == "PILE") {
    return DeviceTypes::kPILE;
  }
  if (type == "STACK") {
    return DeviceTypes::kSTACK;
  }
  return DeviceTypes::kUNKNOWN;
}

// 故障等级
enum class FaultLevel {
  None = 0, // 无故障
  Warning,  // 轻微告警，可继续运行
  Error,    // 一般故障，建议人工关注
  Critical  // 严重故障，应停止运行
};

// 在线状态
enum class OnlineState { Unknown = 0, Online, Offline };

// 工作状态（根据业务后续可扩展）
enum class WorkState {
  Unknown = 0,
  Idle,    // 空闲
  Busy,    // 忙碌（占用但不一定在充电）
  Fault,   // 故障
  Checking // 自检中
};

// 设备当前运行状态（实时）
struct DeviceStatus {
  OnlineState online_state = OnlineState::Unknown; // 在线/离线
  WorkState work_state = WorkState::Unknown;       // 当前工作状态
  FaultLevel fault_level = FaultLevel::None;       // 当前综合故障等级

  // 通讯相关
  bool comm_ok = false;                                   // 通讯是否正常
  std::chrono::system_clock::time_point last_heartbeat{}; // 最近心跳时间
  std::chrono::system_clock::time_point last_update{}; // 最近一次状态更新

  // 最近一次错误信息（可选）
  std::string last_error_code;    // 错误码（来自设备协议）
  std::string last_error_message; // 错误描述（人读得懂的）

  // 版本信息（可选）
  std::string firmware_version; // 固件版本
  std::string software_version; // 软件版本
};

// ================= 自检结果 =================

// 单个子模块自检结果
enum class ModuleCheckStatus { Unknown = 0, OK, Warning, Failed };

struct ModuleCheckResult {
  std::string module_name; // 模块名，如 "comm", "power", "gun1", "safety"
  ModuleCheckStatus status = ModuleCheckStatus::Unknown;
  std::string code;    // 设备上报的错误/状态码
  std::string message; // 人类可读的信息（解析后的）
};

// 自检整体状态
enum class SelfCheckStatus {
  NotRun = 0, // 从未执行过
  Running,    // 正在执行
  Passed,     // 全部通过
  Failed,     // 至少有一个模块失败
  Partial     // 部分成功（有 Warning）
};

// 一次完整的自检结果
struct SelfCheckResult {
  SelfCheckStatus status = SelfCheckStatus::NotRun;

  // 一次自检的时间信息
  std::chrono::system_clock::time_point start_time{};  // 开始时间
  std::chrono::system_clock::time_point finish_time{}; // 结束时间

  // 各子模块的结果列表：
  // 例如：
  // - 通讯：comm
  // - 主控：main_ctrl
  // - 电源：power
  // - 枪1：gun1, 枪2：gun2
  // - 安全保护：safety
  std::vector<ModuleCheckResult> modules;

  // 本次自检的整体错误码/描述（可选）
  std::string summary_code;
  std::string summary_message;

  // 自检序列号 / 任务 ID（方便链路追踪）
  std::string task_id;

  // 最后自检时间字符串（用于 UI 显示）
  std::string last_check_time_str = "未查询到记录"; // 默认值

  // 最后自检结果：0=成功，非0=失败
  int last_check_result = 0;

  // 成功和失败的模块数量（用于统计）
  int success_count = 0;
  int fail_count = 0;

  friend std::ostream &operator<<(std::ostream &output_stream,
                                  const SelfCheckResult &result) {
    output_stream << "自检结果{"
                  << "状态=" << static_cast<int>(result.status) << ", "
                  << "开始时间=" << result.start_time.time_since_epoch().count()
                  << ", "
                  << "结束时间="
                  << result.finish_time.time_since_epoch().count() << ", "
                  << "模块数量=" << result.modules.size() << ", "
                  << "成功数量=" << result.success_count << ", "
                  << "失败数量=" << result.fail_count << ", "
                  << "最后自检时间=" << result.last_check_time_str << ", "
                  << "最后自检结果=" << result.last_check_result << "}";
    return output_stream;
  }
};

// 设备属性（通用）
struct DeviceAttr {
  int db_id = 0;

  std::string station_no; // StationNo：站点编号
  std::string equip_no; // EquipNo：设备编号（通常也是业务上的唯一 ID）
  std::string name;    // EquipName：终端1/终端2...
  std::string ip_addr; // IPAddr：192.168.x.x
  int equip_order = 0; // EquipOrder：排序

  DeviceTypes type = DeviceTypes::kPILE; // 设备类型：PILE / STACK

  // 当 type 是 PILE 时，data4_json 格式: [{"no":9,"gun":"1,2"}]
  // no 代表该站的 CCU 中含有的编号
  std::string data4_json;

  // 运行时状态
  DeviceStatus device_state;
  SelfCheckResult self_check_result;

  friend std::ostream &operator<<(std::ostream &output_stream,
                                  const DeviceAttr &attr) {
    output_stream << "设备属性{"
                  << "数据库ID=" << attr.db_id << ", "
                  << "站点编号='" << attr.station_no << "', "
                  << "设备编号='" << attr.equip_no << "', "
                  << "设备名称='" << attr.name << "', "
                  << "设备类型=" << DeviceTypeToString(attr.type) << ", "
                  << "IP地址='" << attr.ip_addr << "', "
                  << "排序=" << attr.equip_order << ", "
                  << "数据4='" << attr.data4_json << "', "
                  << "自检结果=" << attr.self_check_result << "}";
    return output_stream;
  }
};
} // namespace device
