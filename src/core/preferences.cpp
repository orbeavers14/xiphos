// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/preferences.hpp"
#include <swconfig.h>
#include <filesystem>
#include <fstream>
#include <atomic>
#include <chrono>
#include <stdexcept>
namespace xiphos::core {
namespace {
std::string bytes(const std::filesystem::path &path) {
  std::ifstream input(path, std::ios::binary);
  if (!input) throw std::invalid_argument("Preference file cannot be read");
  std::string result((std::istreambuf_iterator<char>(input)), {});
  if (input.bad()) throw std::runtime_error("Preference file read failed");
  return result;
}
struct TemporaryPreferenceFile {
  std::filesystem::path directory, file;
  explicit TemporaryPreferenceFile(const std::filesystem::path &parent) {
    static std::atomic<unsigned long long> sequence{0};
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    for (unsigned attempt = 0; attempt < 64; ++attempt) {
      auto candidate = parent / (".xiphos-preferences-" + std::to_string(stamp) + "-" + std::to_string(sequence++));
      if (!std::filesystem::create_directory(candidate)) continue;
      directory = std::move(candidate);
      file = directory / "modops.conf";
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
void save_module_engine_preferences(const std::string &path, const ModuleEnginePreferences &settings) {
  if (path.empty()) throw std::invalid_argument("Preference file path is empty");
  auto destination = std::filesystem::absolute(std::filesystem::u8path(path));
  const bool existed = std::filesystem::exists(destination);
  if (existed) {
    if (!std::filesystem::is_regular_file(destination))
      throw std::invalid_argument("Preference destination must be a regular file");
    destination = std::filesystem::canonical(destination);
  } else if (std::filesystem::is_symlink(std::filesystem::symlink_status(destination))) {
    throw std::invalid_argument("Preference destination symlink has no target");
  }
  const auto original = existed ? bytes(destination) : std::string();
  const auto permissions = existed ? std::filesystem::status(destination).permissions()
      : std::filesystem::perms::owner_read | std::filesystem::perms::owner_write;
  const auto write_bits = std::filesystem::perms::owner_write | std::filesystem::perms::group_write | std::filesystem::perms::others_write;
  if (existed && (permissions & write_bits) == std::filesystem::perms::none)
    throw std::runtime_error("Preference destination is read-only");
  TemporaryPreferenceFile temporary(destination.parent_path());
  std::filesystem::permissions(temporary.directory, std::filesystem::perms::owner_all);
  {
    std::ofstream snapshot(temporary.file, std::ios::binary);
    snapshot.exceptions(std::ios::badbit | std::ios::failbit);
    snapshot << original;
    snapshot.close();
  }
  const auto temporary_path = temporary.file.u8string();
  sword::SWConfig configuration(temporary_path.c_str());
  configuration.load();
  for (const auto &module : settings)
    for (const auto &preference : engine_options_to_preferences(module.second))
      configuration[module.first.c_str()][preference.first.c_str()] = preference.second.c_str();
  configuration.save();
  // SWORD save has no failure result. Verify actual written bytes and decoding
  // before replacing the user's file, rather than treating save() as success.
  (void)bytes(temporary.file);
  sword::SWConfig verified(temporary_path.c_str()); verified.load();
  auto expected = configuration.getSections();
  auto actual = verified.getSections();
  // SWConfig adds its own ordering metadata on reload. It changes when new
  // keys are written, plus comment-spacing metadata. Compare every actual
  // preference section/value, excluding only those two reserved SDK indices.
  expected.erase("_ConfOrder"); actual.erase("_ConfOrder");
  expected.erase("_ConfComments"); actual.erase("_ConfComments");
  if (actual != expected)
    throw std::runtime_error("Written preference configuration failed verification");
  std::filesystem::permissions(temporary.file, permissions);
  if (std::filesystem::exists(destination) != existed || (existed && bytes(destination) != original))
    throw std::runtime_error("Preference destination changed during save");
  std::filesystem::rename(temporary.file, destination);
}
}
