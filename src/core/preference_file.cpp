// SPDX-License-Identifier: GPL-2.0-or-later
#include "preference_file.hpp"
#include "bounded_file.hpp"
#include "publication.hpp"
#include <stdexcept>
namespace xiphos::core::detail {
std::string read_preference_file(const std::filesystem::path &path, std::size_t maximum_bytes) {
  return read_bounded_file(path, maximum_bytes, "Preference file");
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
  PrivateStagingDirectory temporary(destination.parent_path(), ".xiphos-preferences-");
  const auto file = temporary.path() / "preferences.tmp";
  prepare(file, original, existed);
  std::filesystem::permissions(file, permissions);
  if (!existed) {
    publish_new_path(file, destination);
    return;
  }
  if (!std::filesystem::exists(destination) || read_preference_file(destination, maximum_bytes) != original)
    throw std::runtime_error("Preference destination changed during save");
  std::filesystem::rename(file, destination);
}
}
