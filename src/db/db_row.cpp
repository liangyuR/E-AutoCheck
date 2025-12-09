#include "db/db_row.h"

#include <charconv>

namespace db {

namespace {

// 小工具：string -> int32
inline std::int32_t parseInt32(const std::string &s, std::int32_t default_val) {
  if (s.empty())
    return default_val;
  std::int32_t v{};
  auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), v);
  if (ec != std::errc())
    return default_val;
  return v;
}

inline std::int64_t parseInt64(const std::string &s, std::int64_t default_val) {
  if (s.empty())
    return default_val;
  std::int64_t v{};
  auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), v);
  if (ec != std::errc())
    return default_val;
  return v;
}

} // namespace

std::int32_t DbRow::getInt(const std::string &col,
                           std::int32_t default_val) const {
  auto it = data_.find(col);
  if (it == data_.end())
    return default_val;
  return parseInt32(it->second, default_val);
}

std::int64_t DbRow::getInt64(const std::string &col,
                             std::int64_t default_val) const {
  auto it = data_.find(col);
  if (it == data_.end())
    return default_val;
  return parseInt64(it->second, default_val);
}

std::string DbRow::getString(const std::string &col,
                             const std::string &default_val) const {
  auto it = data_.find(col);
  if (it == data_.end())
    return default_val;
  return it->second;
}

std::optional<std::string>
DbRow::getNullableString(const std::string &col) const {
  auto it = data_.find(col);
  if (it == data_.end())
    return std::nullopt;
  if (it->second.empty())
    return std::nullopt;
  return it->second;
}

bool DbRow::getBool(const std::string &col, bool default_val) const {
  auto it = data_.find(col);
  if (it == data_.end())
    return default_val;
  const auto &v = it->second;
  if (v == "1" || v == "true" || v == "TRUE" || v == "True" || v == "Y" ||
      v == "y")
    return true;
  if (v == "0" || v == "false" || v == "FALSE" || v == "False" || v == "N" ||
      v == "n")
    return false;
  return default_val;
}

} // namespace db
