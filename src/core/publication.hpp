// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <filesystem>

namespace xiphos::core::detail {
// Atomically move a sibling file or directory into an absent destination.
void publish_new_path(const std::filesystem::path &staged, const std::filesystem::path &destination);

class PrivateStagingDirectory {
public:
  explicit PrivateStagingDirectory(const std::filesystem::path &parent, const char *prefix);
  ~PrivateStagingDirectory();
  PrivateStagingDirectory(const PrivateStagingDirectory &) = delete;
  PrivateStagingDirectory &operator=(const PrivateStagingDirectory &) = delete;
  const std::filesystem::path &path() const { return directory_; }
  void publish(const std::filesystem::path &destination);
private:
  std::filesystem::path directory_;
  void cleanup() noexcept;
};
}
