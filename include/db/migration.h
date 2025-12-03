#pragma once

#include <absl/status/status.h>

namespace db {

// Checks and creates necessary tables if they don't exist.
absl::Status EnsureTablesExist();

} // namespace db

