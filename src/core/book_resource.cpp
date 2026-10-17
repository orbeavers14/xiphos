// SPDX-License-Identifier: GPL-2.0-or-later
#include "book_resource_source.hpp"
#include "book_tree_source.hpp"
#include "bounded_file.hpp"
#include <filesystem>
#include <swmodule.h>
#include <glib.h>
#include <stdexcept>
#include <memory>
namespace xiphos::core::detail {
BookResourceContext capture_book_resource_context(sword::SWModule &module) {
  if (!indexed_book(module)) throw std::invalid_argument("Resource context requires an indexed general book");
  const auto copy = [&](const char *name) -> std::optional<std::string> {
    const auto *value = module.getConfigEntry(name);
    return value ? std::make_optional(std::string(value)) : std::nullopt;
  };
  return {module.getName(), copy("DataPath"), copy("AbsoluteDataPath"), copy("PrefixPath"), copy("PreferredCSSXHTML")};
}
namespace {
std::filesystem::path resource_path(const BookResourceContext &context, const std::string &filename, bool missing_is_absent) {
  namespace fs = std::filesystem;
  if (!context.absolute_data_path || context.absolute_data_path->empty())
    throw std::invalid_argument("Book has no resource data directory");

  std::error_code error;
  const auto base = fs::canonical(fs::u8path(*context.absolute_data_path), error);
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
std::string resource_bytes(const std::filesystem::path &path, std::size_t maximum_bytes, const char *kind) {
  namespace fs = std::filesystem;
  const auto size = fs::file_size(path);
  const auto stamp = fs::last_write_time(path);
  if (size > maximum_bytes) throw std::invalid_argument(std::string(kind) + " exceeds the byte limit");
  auto data = read_bounded_file(path, maximum_bytes, kind);
  // Detect observed replacement/size/time changes; this is not an interprocess
  // lock and does not freeze a concurrently modified library across requests.
  if (fs::canonical(path) != path || fs::file_size(path) != size || fs::last_write_time(path) != stamp || data.size() != size)
    throw std::runtime_error(std::string(kind) + " changed during reading");
  return data;
}
}
BookImageResource capture_book_image_resource(sword::SWModule &module, BookImageLink command, std::size_t maximum_bytes) {
  if (!maximum_bytes || maximum_bytes > maximum_book_image_bytes)
    throw std::invalid_argument("Unsupported book image byte limit");
  auto context = capture_book_resource_context(module);
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
  const auto path = resource_path(context, filename, false);
  auto data = resource_bytes(path, maximum_bytes, "Book image");
  return {{std::move(context), std::move(command)}, std::move(data)};
}
std::optional<BookStylesheetResource> capture_book_stylesheet_resource(sword::SWModule &module, std::size_t maximum_bytes) {
  if (!maximum_bytes || maximum_bytes > maximum_book_stylesheet_bytes)
    throw std::invalid_argument("Unsupported book stylesheet byte limit");
  auto context = capture_book_resource_context(module);
  if (!context.absolute_data_path || context.absolute_data_path->empty())
    throw std::invalid_argument("Book has no resource data directory");
  const auto filename = context.preferred_css_xhtml.value_or("style.css");
  if (filename.find('\0') != std::string::npos) throw std::invalid_argument("Invalid book stylesheet filename");
  // Match legacy g_build_filename, including its handling of absolute-looking
  // later components. Config paths are filenames, not URL query parameters.
  std::unique_ptr<char, decltype(&g_free)> configured(g_build_filename(context.absolute_data_path->c_str(), filename.c_str(), nullptr), g_free);
  if (!configured) throw std::bad_alloc();
  const auto path = resource_path(context, configured.get(), true);
  if (path.empty()) return std::nullopt;
  auto css = resource_bytes(path, maximum_bytes, "Book stylesheet");
  if (css.find('\0') != std::string::npos || !g_utf8_validate(css.data(), static_cast<gssize>(css.size()), nullptr))
    throw std::invalid_argument("Book stylesheet must be UTF-8 without NUL");
  std::string inline_css;
  inline_css.reserve(css.size());
  for (std::size_t i = 0; i < css.size(); ++i) {
    // A style element ends at a matching HTML end tag, even inside a CSS
    // string/comment. Escape only its slash, which preserves CSS string/URL
    // semantics without rewriting ordinary <, escaped characters or <!-- CDO.
    if (i + 7 <= css.size() && g_ascii_strncasecmp(css.c_str() + i, "</style", 7) == 0) {
      inline_css += "<\\/"; ++i;
    } else inline_css += css[i];
  }
  return BookStylesheetResource{std::move(context), filename, std::move(css), std::move(inline_css)};
}
}
