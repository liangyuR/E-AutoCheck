#pragma once

#include <chrono>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace device {

enum DeviceTypes {
  kPILE = 1,
  kSTACK = 2,
};

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
  Idle,     // 空闲
  Charging, // 充电中
  Busy,     // 忙碌（占用但不一定在充电）
  Fault     // 故障
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
  std::string last_check_time_str;

  // 最后自检结果：0=成功，非0=失败
  int last_check_result = 0;

  // 成功和失败的模块数量（用于统计）
  int success_count = 0;
  int fail_count = 0;
};

// 交流接触器
struct ACContactorStatus {
  bool contactor1_stuck = false;  // 交流接触器粘连
  bool contactor1_refuse = false; // 交流接触器拒动

  friend std::ostream &operator<<(std::ostream &output_stream,
                                  const ACContactorStatus &attr) {
    output_stream << "ACContactorStatus{"
                  << "contactor_stuck=" << attr.contactor1_stuck
                  << ", contactor_refuse=" << attr.contactor1_refuse << "}";
    return output_stream;
  }
};

// 并联接触器相关
struct ParallelContactorStatus {
  bool positive_stuck = false;  // 并联接触器正极粘连
  bool positive_refuse = false; // 并联接触器正极拒动
  bool negative_stuck = false;  // 并联接触器负极粘连
  bool negative_refuse = false; // 并联接触器负极拒动

  friend std::ostream &operator<<(std::ostream &output_stream,
                                  const ParallelContactorStatus &attr) {
    output_stream << "ParallelContactorStatus{"
                  << "positive_stuck=" << attr.positive_stuck
                  << ", positive_refuse=" << attr.positive_refuse
                  << ", negative_stuck=" << attr.negative_stuck
                  << ", negative_refuse=" << attr.negative_refuse << "}";
    return output_stream;
  }
};

struct FanStatus {
  bool stopped = false;  // 停转状态反馈
  bool rotating = false; // 转动状态反馈

  friend std::ostream &operator<<(std::ostream &output_stream,
                                  const FanStatus &attr) {
    output_stream << "FanStatus{"
                  << "stopped=" << attr.stopped
                  << ", rotating=" << attr.rotating << "}";
    return output_stream;
  }
};

// 4. 枪状态 (A/B 枪通用结构)
struct GunStatus {
  bool positive_contactor_stuck = false;  // 正极接触器粘连
  bool positive_contactor_refuse = false; // 正极接触器拒动
  bool negative_contactor_stuck = false;  // 负极接触器粘连
  bool negative_contactor_refuse = false; // 负极接触器拒动
  bool unlocked = false;                  // 解锁状态反馈
  bool locked = false;                    // 上锁状态反馈
  bool aux_power_12v = false;             // 12V 辅源状态
  bool aux_power_24v = false;             // 24V 辅源状态

  friend std::ostream &operator<<(std::ostream &output_stream,
                                  const GunStatus &attr) {
    output_stream
        << "GunStatus{"
        << "positive_contactor_stuck=" << attr.positive_contactor_stuck
        << ", positive_contactor_refuse=" << attr.positive_contactor_refuse
        << ", negative_contactor_stuck=" << attr.negative_contactor_stuck
        << ", negative_contactor_refuse=" << attr.negative_contactor_refuse
        << ", unlocked=" << attr.unlocked << ", locked=" << attr.locked
        << ", aux_power_12v=" << attr.aux_power_12v
        << ", aux_power_24v=" << attr.aux_power_24v << "}";
    return output_stream;
  }
};

// CCU 属性结构体
struct CCUAttributes {

  ACContactorStatus ac_contactor_1;
  ACContactorStatus ac_contactor_2;

  ParallelContactorStatus parallel_contactor;

  FanStatus fan_1;
  FanStatus fan_2;
  FanStatus fan_3;
  FanStatus fan_4;

  GunStatus gun_a;
  GunStatus gun_b;

  friend std::ostream &operator<<(std::ostream &output_stream,
                                  const CCUAttributes &attr) {
    output_stream << "CCUAttributes{"
                  << "ac_contactor_1=" << attr.ac_contactor_1
                  << ", ac_contactor_2=" << attr.ac_contactor_2
                  << ", parallel_contactor=" << attr.parallel_contactor
                  << ", fan_1=" << attr.fan_1 << ", fan_2=" << attr.fan_2
                  << ", fan_3=" << attr.fan_3 << ", fan_4=" << attr.fan_4
                  << ", gun_a=" << attr.gun_a << ", gun_b=" << attr.gun_b
                  << "}";
    return output_stream;
  }
};

struct ChargerBoxAttributes {
  int db_id = 0;

  std::string station_no; // StationNo：站点编号
  std::string equip_no; // EquipNo：设备编号（通常也是业务上的唯一 ID）
  std::string name;    // EquipName：终端1/终端2...
  std::string name_en; // EquipNameEn：No.1 TCU...
  std::string type;    // Type：PILE / SWAP / 其他

  std::string ip_addr; // IPAddr：192.168.x.x
  int gun_count = 0;   // GunCount：枪数量
  int equip_order = 0; // EquipOrder：排序

  int encrypt = 0;        // Encrypt：是否加密
  std::string secret_key; // SecretKey
  std::string secret_iv;  // SecretIV

  int data1 = 0; // Data1：按现有表结构保留
  int data3 = 0; // Data3：通常是 24 等含义，后续可用枚举封装
  std::string data2_json; // Data2：目前是 <null>，预留
  std::string data4_json; // Data4：[{"no":9,"gun":"1,2"}] 之类的 JSON 串

  std::vector<CCUAttributes> ccu_attributes; // CCU 信息，需要从 Redis 中获取

  friend std::ostream &operator<<(std::ostream &output_stream,
                                  const ChargerBoxAttributes &attr) {
    output_stream << "ChargerBoxAttributes{"
                  << "db_id=" << attr.db_id << ", "
                  << "station_no='" << attr.station_no << "', "
                  << "equip_no='" << attr.equip_no << "', "
                  << "name='" << attr.name << "', "
                  << "name_en='" << attr.name_en << "', "
                  << "type='" << attr.type << "', "
                  << "ip_addr='" << attr.ip_addr << "', "
                  << "gun_count=" << attr.gun_count << ", "
                  << "equip_order=" << attr.equip_order << ", "
                  << "encrypt=" << attr.encrypt << ", "
                  << "secret_key='" << attr.secret_key << "', "
                  << "secret_iv='" << attr.secret_iv << "', "
                  << "data1=" << attr.data1 << ", "
                  << "data3=" << attr.data3 << ", "
                  << "data2_json='" << attr.data2_json << "', "
                  << "data4_json='" << attr.data4_json << "'"
                  << "}";
    return output_stream;
  }
};
} // namespace device
