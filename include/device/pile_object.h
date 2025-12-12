#pragma once

#include "device/device_object.h"

namespace device {

// 交流接触器状态
struct ACContactorStatus {
  bool contactor1_stuck = false;  // 交流接触器粘连
  bool contactor1_refuse = false; // 交流接触器拒动

  // 重载输出流运算符，方便打印结构体内容
  friend std::ostream &operator<<(std::ostream &os,
                                  const ACContactorStatus &status) {
    os << "交流接触器状态{"
       << "接触器1粘连=" << (status.contactor1_stuck ? "是" : "否")
       << "，接触器1拒动=" << (status.contactor1_refuse ? "是" : "否") << "}";
    return os;
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
    output_stream << "并联接触器状态{"
                  << "正极粘连=" << (attr.positive_stuck ? "是" : "否")
                  << "，正极拒动=" << (attr.positive_refuse ? "是" : "否")
                  << "，负极粘连=" << (attr.negative_stuck ? "是" : "否")
                  << "，负极拒动=" << (attr.negative_refuse ? "是" : "否")
                  << "}";
    return output_stream;
  }
};

struct FanStatus {
  bool stopped = false;  // 停转状态反馈
  bool rotating = false; // 转动状态反馈

  friend std::ostream &operator<<(std::ostream &output_stream,
                                  const FanStatus &attr) {
    output_stream << "风扇状态{"
                  << "停转=" << (attr.stopped ? "是" : "否")
                  << "，转动=" << (attr.rotating ? "是" : "否") << "}";
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
        << "枪状态{"
        << "正极接触器粘连=" << (attr.positive_contactor_stuck ? "是" : "否")
        << "，正极接触器拒动=" << (attr.positive_contactor_refuse ? "是" : "否")
        << "，负极接触器粘连=" << (attr.negative_contactor_stuck ? "是" : "否")
        << "，负极接触器拒动=" << (attr.negative_contactor_refuse ? "是" : "否")
        << "，解锁=" << (attr.unlocked ? "是" : "否")
        << "，上锁=" << (attr.locked ? "是" : "否")
        << "，12V 辅源=" << (attr.aux_power_12v ? "是" : "否")
        << "，24V 辅源=" << (attr.aux_power_24v ? "是" : "否") << "}";
    return output_stream;
  }
};

struct CCUAttributes {
  int index = 0;

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
    output_stream << "CCU 属性{"
                  << "编号=" << attr.index
                  << "，交流接触器1=" << attr.ac_contactor_1
                  << "，交流接触器2=" << attr.ac_contactor_2
                  << "，并联接触器=" << attr.parallel_contactor
                  << "，风扇1=" << attr.fan_1 << "，风扇2=" << attr.fan_2
                  << "，风扇3=" << attr.fan_3 << "，风扇4=" << attr.fan_4
                  << "，枪A=" << attr.gun_a << "，枪B=" << attr.gun_b << "}";
    return output_stream;
  }
};

struct PileAttributes : public DeviceAttr {
  std::string last_check_time;               // 最后检测时间
  std::vector<CCUAttributes> ccu_attributes; // CCU 模块列表

  friend std::ostream &operator<<(std::ostream &output_stream,
                                  const PileAttributes &attr) {
    output_stream << "充电桩属性{"
                  << "设备属性=" << static_cast<const DeviceAttr &>(attr)
                  << "，最后检测时间='" << attr.last_check_time << "'"
                  << "，CCU 属性=" << attr.ccu_attributes.size() << "个"
                  << "}";
    return output_stream;
  }
};
} // namespace device
