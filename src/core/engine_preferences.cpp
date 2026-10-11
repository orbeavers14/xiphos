// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/engine_options.hpp"
#include <swconfig.h>
#include <fstream>
#include <sys/stat.h>
#include <stdexcept>
namespace xiphos { namespace core {
ModuleEnginePreferences read_module_engine_preferences(const std::string &path) {
  if (path.empty()) throw std::invalid_argument("Preference file path is empty");
#ifdef _WIN32
  struct _stat64 info;
  if (_stat64(path.c_str(), &info) != 0 || (info.st_mode & _S_IFMT) != _S_IFREG)
#else
  struct stat info;
  if (::stat(path.c_str(), &info) != 0 || (info.st_mode & S_IFMT) != S_IFREG)
#endif
    throw std::invalid_argument("Preference path must be an existing regular file");
  // SWConfig silently accepts missing/unreadable files as empty configuration.
  // A deliberate import must report failure instead of resetting to defaults.
  std::ifstream input(path.c_str(), std::ios::binary);
  if (!input) throw std::invalid_argument("Preference file cannot be read");
  // Check read access before allowing SWConfig's permissive empty defaults.
  input.peek();
  if (input.bad()) throw std::invalid_argument("Preference file cannot be read");
  sword::SWConfig configuration(path.c_str());
  configuration.load();
  ModuleEnginePreferences result;
  for (const auto &section : configuration.getSections()) {
    EngineOptionPreferences values;
    for (const auto &item : section.second)
      values.emplace(item.first.c_str(), item.second.c_str());
    result.emplace(section.first.c_str(), engine_options_from_preferences(values));
  }
  return result;
}
} }
