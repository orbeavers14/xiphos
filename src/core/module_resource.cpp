// SPDX-License-Identifier: GPL-2.0-or-later
#include "module_resource.hpp"
#include "bounded_file.hpp"
#include <stdexcept>
namespace xiphos::core::detail {
std::string normalize_module_image_filename(std::string filename) {
  // file:///local/path retains three slashes after legacy file: stripping.
  // Authority/UNC forms are not implicitly contacted by this initial local reader.
  if (filename.compare(0, 3, "///") == 0) {
    const auto first = filename.find_first_not_of('/');
    filename = first == std::string::npos ? "/" : "/" + filename.substr(first);
  } else if (filename.compare(0, 2, "//") == 0 || filename.compare(0, 2, "\\\\") == 0) {
    throw std::invalid_argument("Book image authorities are unsupported");
  }
#ifdef _WIN32
  if (filename.size() > 3 && filename[0] == '/' && filename[2] == ':' &&
      ((filename[1] >= 'A' && filename[1] <= 'Z') || (filename[1] >= 'a' && filename[1] <= 'z')))
    filename.erase(0, 1);
#endif
  return filename;
}
std::filesystem::path module_resource_path(const std::optional<std::string> &directory, const std::string &filename, bool missing_is_absent) {
  namespace fs = std::filesystem;
  if (!directory || directory->empty())
    throw std::invalid_argument("Book has no resource data directory");
  // Native filesystem calls consume terminated paths. Never let an owned
  // string with an embedded NUL silently address a different resource.
  if (directory->find('\0') != std::string::npos || filename.find('\0') != std::string::npos)
    throw std::invalid_argument("Resource path contains NUL");

  std::error_code error;
  const auto base = fs::canonical(fs::u8path(*directory), error);
  if (error || !fs::is_directory(base)) throw std::invalid_argument("Book resource directory is unavailable");
  const auto requested = fs::u8path(filename);
  if (requested.has_root_name() && !requested.is_absolute()) throw std::invalid_argument("Drive-relative book resource is unsupported");
  const auto path = fs::canonical(requested.is_absolute() ? requested : base / requested, error);
  if (error) {
    if (missing_is_absent && (error == std::errc::no_such_file_or_directory || error == std::errc::not_a_directory)) return {};
    throw std::invalid_argument("Book resource cannot be resolved");
  }
  bool contained = false;
  for (auto parent = path.parent_path(); !parent.empty();) {
    if (fs::equivalent(base, parent)) { contained = true; break; }
    const auto next = parent.parent_path();
    if (next == parent) break;
    parent = next;
  }
  if (!contained || !fs::is_regular_file(path))
    throw std::invalid_argument("Book resource must be a regular file inside its data directory");
  return path;
}
std::string read_module_resource_bytes(const std::filesystem::path &path, std::size_t maximum_bytes, const char *kind) {
  namespace fs = std::filesystem;
  const auto size = fs::file_size(path);
  const auto stamp = fs::last_write_time(path);
  if (size > maximum_bytes) throw BoundedFileLimitError(std::string(kind) + " exceeds the byte limit");
  auto data = read_bounded_file(path, maximum_bytes, kind);
  // Detect observed replacement/size/time changes; this is not an interprocess
  // lock and does not freeze a concurrently modified library across requests.
  if (fs::canonical(path) != path || fs::file_size(path) != size || fs::last_write_time(path) != stamp || data.size() != size)
    throw std::runtime_error(std::string(kind) + " changed during reading");
  return data;
}
}
