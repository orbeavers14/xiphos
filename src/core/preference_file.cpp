// SPDX-License-Identifier: GPL-2.0-or-later
#include "preference_file.hpp"
#include <atomic>
#include <chrono>
#include <fstream>
#include <stdexcept>
namespace xiphos::core::detail {
std::string read_preference_file(const std::filesystem::path &path, std::size_t maximum_bytes) {
  std::ifstream input(path, std::ios::binary);
  if (!input) throw std::invalid_argument("Preference file cannot be read");
  std::string result;
  char buffer[8192];
  while (input.read(buffer, sizeof buffer) || input.gcount()) {
    const auto length = static_cast<std::size_t>(input.gcount());
    if (length > maximum_bytes - result.size()) throw std::invalid_argument("Preference file is too large");
    result.append(buffer, length);
  }
  if (input.bad()) throw std::runtime_error("Preference file read failed");
  return result;
}
namespace {
struct TemporaryPreferenceFile {
  std::filesystem::path directory, file;
  explicit TemporaryPreferenceFile(const std::filesystem::path &parent) {
    static std::atomic<unsigned long long> sequence{0};
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    for (unsigned attempt = 0; attempt < 64; ++attempt) {
      auto candidate = parent / (".xiphos-preferences-" + std::to_string(stamp) + "-" + std::to_string(sequence++));
      if (!std::filesystem::create_directory(candidate)) continue;
      directory = std::move(candidate);
      file = directory / "preferences.tmp";
      return;
    }
    throw std::runtime_error("Cannot create temporary preference file");
  }
  ~TemporaryPreferenceFile() {
    std::error_code ignored;
    std::filesystem::remove(file, ignored);
    std::filesystem::remove(directory, ignored);
  }
  TemporaryPreferenceFile(const TemporaryPreferenceFile &) = delete;
  TemporaryPreferenceFile &operator=(const TemporaryPreferenceFile &) = delete;
};
}
void replace_preference_file(const std::string &path,
    const std::function<void(const std::filesystem::path &, const std::string &, bool)> &prepare,
    std::size_t maximum_bytes) {
  if (path.empty() || path.find('\0') != std::string::npos)
    throw std::invalid_argument("Preference file path is invalid");
  auto destination = std::filesystem::absolute(std::filesystem::u8path(path));
  const bool existed = std::filesystem::exists(destination);
  if (existed) {
    if (!std::filesystem::is_regular_file(destination))
      throw std::invalid_argument("Preference destination must be a regular file");
    destination = std::filesystem::canonical(destination);
  } else if (std::filesystem::is_symlink(std::filesystem::symlink_status(destination))) {
    throw std::invalid_argument("Preference destination symlink has no target");
  }
  const auto original = existed ? read_preference_file(destination, maximum_bytes) : std::string();
  const auto permissions = existed ? std::filesystem::status(destination).permissions()
      : std::filesystem::perms::owner_read | std::filesystem::perms::owner_write;
  const auto write_bits = std::filesystem::perms::owner_write | std::filesystem::perms::group_write | std::filesystem::perms::others_write;
  if (existed && (permissions & write_bits) == std::filesystem::perms::none)
    throw std::runtime_error("Preference destination is read-only");
  TemporaryPreferenceFile temporary(destination.parent_path());
  std::filesystem::permissions(temporary.directory, std::filesystem::perms::owner_all);
  prepare(temporary.file, original, existed);
  std::filesystem::permissions(temporary.file, permissions);
  if (std::filesystem::exists(destination) != existed || (existed && read_preference_file(destination, maximum_bytes) != original))
    throw std::runtime_error("Preference destination changed during save");
  std::filesystem::rename(temporary.file, destination);
}
}
