// SPDX-License-Identifier: GPL-2.0-or-later
#include "book_resource_source.hpp"
#include "book_tree_source.hpp"
#include "bounded_file.hpp"
#include <filesystem>
#include <swmodule.h>
#include <stdexcept>
namespace xiphos::core::detail {
BookResourceContext capture_book_resource_context(sword::SWModule &module) {
  if (!indexed_book(module)) throw std::invalid_argument("Resource context requires an indexed general book");
  const auto copy = [&](const char *name) -> std::optional<std::string> {
    const auto *value = module.getConfigEntry(name);
    return value ? std::make_optional(std::string(value)) : std::nullopt;
  };
  return {module.getName(), copy("DataPath"), copy("AbsoluteDataPath"), copy("PrefixPath"), copy("PreferredCSSXHTML")};
}
BookImageResource capture_book_image_resource(sword::SWModule &module, BookImageLink command, std::size_t maximum_bytes) {
  namespace fs = std::filesystem;
  if (!maximum_bytes || maximum_bytes > maximum_book_image_bytes)
    throw std::invalid_argument("Unsupported book image byte limit");
  auto context = capture_book_resource_context(module);
  if (!context.absolute_data_path || context.absolute_data_path->empty())
    throw std::invalid_argument("Book has no resource data directory");
  auto filename = command.filename;
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
  std::error_code error;
  const auto base = fs::canonical(fs::u8path(*context.absolute_data_path), error);
  if (error || !fs::is_directory(base)) throw std::invalid_argument("Book resource directory is unavailable");
  const auto requested = fs::u8path(filename);
  if (requested.has_root_name() && !requested.is_absolute()) throw std::invalid_argument("Drive-relative book image is unsupported");
  const auto path = fs::canonical(requested.is_absolute() ? requested : base / requested, error);
  if (error) throw std::invalid_argument("Book image cannot be resolved");
  bool contained = false;
  for (auto parent = path.parent_path(); !parent.empty();) {
    if (fs::equivalent(base, parent)) { contained = true; break; }
    const auto next = parent.parent_path();
    if (next == parent) break;
    parent = next;
  }
  if (!contained || !fs::is_regular_file(path))
    throw std::invalid_argument("Book image must be a regular file inside its data directory");
  const auto size = fs::file_size(path);
  const auto stamp = fs::last_write_time(path);
  if (size > maximum_bytes) throw std::invalid_argument("Book image exceeds the byte limit");
  auto data = read_bounded_file(path, maximum_bytes, "Book image");
  // Detect observed replacement/size/time changes; this is not an interprocess
  // lock and does not freeze a concurrently modified library across requests.
  if (fs::canonical(path) != path || fs::file_size(path) != size || fs::last_write_time(path) != stamp || data.size() != size)
    throw std::runtime_error("Book image changed during reading");
  return {{std::move(context), std::move(command)}, std::move(data)};
}

}
