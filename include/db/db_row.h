#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>

namespace db {

// 一个简单的“行”抽象：按列名取值
class DbRow {
public:
  using RawMap = std::unordered_map<std::string, std::string>;

  DbRow() = default;
  explicit DbRow(RawMap data) : data_(std::move(data)) {}

  // 是否有某个列
  bool has(const std::string &col) const {
    return data_.find(col) != data_.end();
  }

  // ---- 基本类型取值工具 ----

  // 整型（不存在或解析失败 -> default_val）
  std::int32_t getInt(const std::string &col,
                      std::int32_t default_val = 0) const;

  std::int64_t getInt64(const std::string &col,
                        std::int64_t default_val = 0) const;

  // 字符串（不存在 -> default_val）
  std::string getString(const std::string &col,
                        const std::string &default_val = "") const;

  // 可空字符串（不存在或空串 -> std::nullopt）
  std::optional<std::string> getNullableString(const std::string &col) const;

  // 布尔（常见存储：0/1、true/false、Y/N）
  bool getBool(const std::string &col, bool default_val = false) const;

private:
  RawMap data_; // 所有值先存为 string，解析逻辑由 getter 负责
};
} // namespace db
