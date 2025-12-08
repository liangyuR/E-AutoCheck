#include <absl/status/statusor.h>
#include <device/device_object.h>
#include <optional>
#include <string>
#include <unordered_map>

namespace utils {
absl::StatusOr<device::CCUAttributes> ToCCUAttr(
    std::optional<std::unordered_map<std::string, std::string>> hash_opt) {
  if (!hash_opt.has_value()) {
    return absl::NotFoundError("Redis hash is empty");
  }

  std::array<bool, 32> seen{};
  device::CCUAttributes attributes;
  attributes.index = std::stoi(hash_opt.value().at("index"));

  for (const auto &item : hash_opt.value()) {
    const std::string &field = item.first;
    const std::string &value_str = item.second;

    auto delimiter_pos = field.find(';');
    if (delimiter_pos == std::string::npos) {
      // Field missing index delimiter, skip
      continue;
    }

    int index = -1;
    try {
      index = std::stoi(field.substr(0, delimiter_pos));
    } catch (...) {
      continue;
    }

    if (index < 0 || index >= static_cast<int>(seen.size())) {
      continue;
    }

    int numeric_value = 0;
    try {
      numeric_value = std::stoi(value_str);
    } catch (...) {
      continue;
    }

    bool status = false;
    if (numeric_value == 1) {
      status = true;
    } else if (numeric_value == 2) {
      status = false;
    } else {
      continue;
    }

    seen[index] = true;
    switch (index) {
    case 0:
      attributes.ac_contactor_1.contactor1_refuse = status;
      break;
    case 1:
      attributes.ac_contactor_1.contactor1_stuck = status;
      break;
    case 2:
      attributes.ac_contactor_2.contactor1_refuse = status;
      break;
    case 3:
      attributes.ac_contactor_2.contactor1_stuck = status;
      break;
    case 4:
      attributes.parallel_contactor.positive_refuse = status;
      break;
    case 5:
      attributes.parallel_contactor.positive_stuck = status;
      break;
    case 6:
      attributes.parallel_contactor.negative_refuse = status;
      break;
    case 7:
      attributes.parallel_contactor.negative_stuck = status;
      break;
    case 8:
      attributes.fan_1.stopped = status;
      break;
    case 9:
      attributes.fan_1.rotating = status;
      break;
    case 10:
      attributes.fan_2.stopped = status;
      break;
    case 11:
      attributes.fan_2.rotating = status;
      break;
    case 12:
      attributes.fan_3.stopped = status;
      break;
    case 13:
      attributes.fan_3.rotating = status;
      break;
    case 14:
      attributes.fan_4.stopped = status;
      break;
    case 15:
      attributes.fan_4.rotating = status;
      break;
    case 16:
      attributes.gun_a.positive_contactor_refuse = status;
      break;
    case 17:
      attributes.gun_a.positive_contactor_stuck = status;
      break;
    case 18:
      attributes.gun_a.negative_contactor_refuse = status;
      break;
    case 19:
      attributes.gun_a.negative_contactor_stuck = status;
      break;
    case 20:
      attributes.gun_a.unlocked = status;
      break;
    case 21:
      attributes.gun_a.locked = status;
      break;
    case 22:
      attributes.gun_a.aux_power_12v = status;
      break;
    case 23:
      attributes.gun_a.aux_power_24v = status;
      break;
    case 24:
      attributes.gun_b.positive_contactor_refuse = status;
      break;
    case 25:
      attributes.gun_b.positive_contactor_stuck = status;
      break;
    case 26:
      attributes.gun_b.negative_contactor_refuse = status;
      break;
    case 27:
      attributes.gun_b.negative_contactor_stuck = status;
      break;
    case 28:
      attributes.gun_b.unlocked = status;
      break;
    case 29:
      attributes.gun_b.locked = status;
      break;
    case 30:
      attributes.gun_b.aux_power_12v = status;
      break;
    case 31:
      attributes.gun_b.aux_power_24v = status;
      break;
    default:
      break;
    }
  }
  return attributes;
}
} // namespace utils