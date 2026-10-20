// SPDX-License-Identifier: GPL-2.0-or-later
#include "book_resource_source.hpp"
#include "book_tree_source.hpp"
#include "module_resource.hpp"
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
BookImageResource capture_book_image_resource(sword::SWModule &module, BookImageLink command, std::size_t maximum_bytes) {
  if (!maximum_bytes || maximum_bytes > maximum_book_image_bytes)
    throw std::invalid_argument("Unsupported book image byte limit");
  auto context = capture_book_resource_context(module);
  const auto filename = normalize_module_image_filename(command.filename);
  const auto path = module_resource_path(context.absolute_data_path, filename, false);
  auto data = read_module_resource_bytes(path, maximum_bytes, "Book image");
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
  const auto path = module_resource_path(context.absolute_data_path, configured.get(), true);
  if (path.empty()) return std::nullopt;
  auto css = read_module_resource_bytes(path, maximum_bytes, "Book stylesheet");
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
