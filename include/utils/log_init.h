#pragma once

#include <absl/status/status.h>
#include <absl/strings/str_format.h>
#include <glog/logging.h>

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>

namespace {

std::string GetHomeDir() {
  if (const char *h = std::getenv("HOME"); h && *h)
    return h;
  return "/tmp"; // 兜底
}

std::string ExpandUser(const std::string &path) {
  if (!path.empty() && path[0] == '~') {
    return GetHomeDir() + path.substr(1);
  }
  return path;
}

std::string BasenameOfProc() {
  char buf[4096] = {0};
  ssize_t n = ::readlink("/proc/self/exe", buf, sizeof(buf) - 1);
  std::string full = (n > 0) ? std::string(buf, static_cast<size_t>(n)) : "app";
  auto pos = full.find_last_of('/');
  return (pos == std::string::npos) ? full : full.substr(pos + 1);
}

// 每24小时清理过期日志
void StartLogCleanupThread(std::filesystem::path dir, int keep_days) {
  std::thread([dir = std::move(dir), keep_days]() {
    using namespace std::chrono;
    const auto interval = hours(24);
    while (true) {
      std::error_code ec;
      const auto now = std::filesystem::file_time_type::clock::now();
      const auto deadline = now - hours(24 * keep_days);

      for (auto it = std::filesystem::directory_iterator(dir, ec);
           !ec && it != std::filesystem::end(it); it.increment(ec)) {
        if (ec)
          break;
        const auto &p = it->path();
        if (!it->is_regular_file(ec))
          continue;
        if (p.extension() != ".log")
          continue; // glog 后缀 ".log"
        std::error_code tec;
        auto t = std::filesystem::last_write_time(p, tec);
        if (tec)
          continue;
        if (t < deadline) {
          std::error_code dec;
          std::filesystem::remove(p, dec);
          if (!dec) {
            LOG(INFO) << "Removed old log: " << p.string();
          }
        }
      }
      std::this_thread::sleep_for(interval);
    }
  }).detach();
}

int EnvToInt(const char *name, int def_v) {
  if (const char *v = std::getenv(name)) {
    try {
      return std::stoi(v);
    } catch (...) {
      return def_v;
    }
  }
  return def_v;
}

} // namespace

absl::Status InitLogging() {
  // 1) 目录
  const std::string log_dir_str = ExpandUser("~/ECheckAuto/logs");
  std::error_code ec;
  if (!std::filesystem::create_directories(log_dir_str, ec) && ec) {
    return absl::InternalError(absl::StrFormat(
        "create_directories(%s) failed: %s", log_dir_str, ec.message()));
  }

  // 2) 程序名
  const std::string prog = BasenameOfProc();

  // 3) 初始化 glog（只需一次；如多次调用可加静态保护）
  static bool inited = false;
  if (!inited) {
    google::InitGoogleLogging(prog.c_str());
    google::InstallFailureSignalHandler(); // 崩溃栈
    inited = true;
  }

  // 4) 分级别文件
  const std::string base = log_dir_str + "/" + prog;
  google::SetLogDestination(google::GLOG_INFO, (base + ".INFO.").c_str());
  google::SetLogDestination(google::GLOG_WARNING, (base + ".WARNING.").c_str());
  google::SetLogDestination(google::GLOG_ERROR, (base + ".ERROR.").c_str());
  google::SetLogDestination(google::GLOG_FATAL, (base + ".FATAL.").c_str());

  // 可选：最新symlink（方便 tail）
  google::SetLogSymlink(google::GLOG_INFO, (base + ".INFO").c_str());
  google::SetLogSymlink(google::GLOG_WARNING, (base + ".WARNING").c_str());
  google::SetLogSymlink(google::GLOG_ERROR, (base + ".ERROR").c_str());
  google::SetLogSymlink(google::GLOG_FATAL, (base + ".FATAL").c_str());

  // 5) flags（大小限制、磁盘保护、stderr策略、彩色）
  FLAGS_max_log_size = 10;                // MB；超过自动滚动到新文件
  FLAGS_stop_logging_if_full_disk = true; // 磁盘满停止写入
#ifdef GLOG_EXPORT_FLAGS
  // 一些构建选项下才有彩色与stderr颜色控制
  FLAGS_colorlogtostderr = true;
#endif
  google::SetStderrLogging(google::GLOG_WARNING); // WARNING+ 同时打到stderr

  // 6) verbose（支持 VLOG）
  // 优先 ECA_V，然后兼容 GLOG_v；默认 0
  int v = EnvToInt("ECA_V", EnvToInt("GLOG_v", 0));
  FLAGS_v = v;

  // 7) 后台清理（默认 30 天）
  StartLogCleanupThread(log_dir_str, /*keep_days=*/30);

  // 8) 成功
  LOG(INFO) << "glog initialized. dir=" << log_dir_str
            << " max_log_size(MB)=" << FLAGS_max_log_size << " v=" << FLAGS_v;
  return absl::OkStatus();
}
