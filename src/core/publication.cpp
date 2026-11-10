// SPDX-License-Identifier: GPL-2.0-or-later
#include "publication.hpp"
#include <atomic>
#include <cerrno>
#include <chrono>
#include <stdexcept>
#include <system_error>
#if defined(__APPLE__)
#include <stdio.h>
#elif defined(__linux__)
#include <fcntl.h>
#include <linux/fs.h>
#include <sys/syscall.h>
#include <unistd.h>
#elif defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace xiphos::core::detail {
namespace fs = std::filesystem;
void publish_new_path(const fs::path &staged, const fs::path &destination) {
#if defined(__APPLE__)
  if (renamex_np(staged.c_str(), destination.c_str(), RENAME_EXCL) == 0) return;
  const std::error_code error(errno, std::generic_category());
#elif defined(__linux__) && defined(SYS_renameat2)
  if (syscall(SYS_renameat2, AT_FDCWD, staged.c_str(), AT_FDCWD, destination.c_str(), RENAME_NOREPLACE) == 0) return;
  const std::error_code error(errno, std::generic_category());
#elif defined(_WIN32)
  if (MoveFileW(staged.c_str(), destination.c_str())) return;
  const std::error_code error(static_cast<int>(GetLastError()), std::system_category());
#else
  const auto error = std::make_error_code(std::errc::operation_not_supported);
#endif
  if (error == std::errc::file_exists || error == std::errc::directory_not_empty)
    throw std::invalid_argument("Destination already exists");
  throw fs::filesystem_error("Cannot publish a new path", staged, destination, error);
}

PrivateStagingDirectory::PrivateStagingDirectory(const fs::path &parent, const char *prefix) {
  static std::atomic<unsigned long long> sequence{0};
  const auto stamp = std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
  for (unsigned attempt = 0; attempt < 64; ++attempt) {
    auto candidate = parent / (prefix + stamp + "-" + std::to_string(sequence++));
    if (!fs::create_directory(candidate)) continue;
    directory_ = std::move(candidate);
    try {
      fs::permissions(directory_, fs::perms::owner_all, fs::perm_options::replace);
    } catch (...) {
      cleanup();
      throw;
    }
    return;
  }
  throw std::runtime_error("Cannot create private staging directory");
}

PrivateStagingDirectory::~PrivateStagingDirectory() { cleanup(); }

void PrivateStagingDirectory::cleanup() noexcept {
  try {
    if (!directory_.empty()) {
      std::error_code ignored;
      fs::remove_all(directory_, ignored);
    }
  } catch (...) {
    // Cleanup must not terminate during an allocation failure.
  }
}

void PrivateStagingDirectory::publish(const fs::path &destination) {
  publish_new_path(directory_, destination);
  directory_.clear();
}
}
