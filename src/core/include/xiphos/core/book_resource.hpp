// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <optional>
#include <cstddef>
#include <string>
#include <vector>
#include <functional>
namespace xiphos::core {
// Exact copied SDK configuration, not a filesystem authorization or resolved
// image path. Missing and explicitly empty values remain distinct.
struct BookResourceContext {
  std::string module;
  std::optional<std::string> data_path, absolute_data_path, prefix_path, preferred_css_xhtml;
};
// Legacy showImage strips a single leading file: and accepts known lowercase
// suffixes. Other URI fields do not rebase the filename; declared module is
// retained as metadata because the legacy image branch does not use it.
struct BookImageLink {
  std::string filename;
  std::optional<std::string> declared_module;
  std::string media_type; // Filename-extension hint, not a decoder/validation result.
};
struct BookImageTarget {
  BookResourceContext context;
  BookImageLink command;
};
inline constexpr std::size_t maximum_book_image_bytes = 16 * 1024 * 1024;
struct BookImageResource {
  BookImageTarget target;
  std::string data; // Binary bytes, including NUL; no retained file/SDK handles.
};

inline constexpr std::size_t maximum_book_stylesheet_bytes = 1024 * 1024;
struct BookStylesheetResource {
  BookResourceContext context;
  std::string filename; // Raw config path; no URI/query percent decoding.
  std::string css; // Valid UTF-8 without NUL, including explicitly empty files.
  std::string inline_css; // Style closing-tag openers escaped for HTML embedding.
};
// Resource-prepared output is distinct from raw SDK and ordinary presentation
// HTML. libxml HTML recovery/serialization can normalize markup; retain both.
struct BookInlineImageIssue { std::string source, message; };
struct BookInlineImages {
  std::string html;
  std::vector<BookInlineImageIssue> issues;
  std::size_t embedded = 0;
};
struct BookInlineImageLimits {
  std::size_t max_images = 64;
  std::size_t max_image_bytes = maximum_book_image_bytes;
  std::size_t max_html_bytes = 32 * 1024 * 1024;
};
// Loader runs synchronously on the owning engine worker, returns copied bytes,
// and must apply the module-directory policy. No DOM/node/file handles escape.
BookInlineImages prepare_book_inline_images(const std::string &html,
    const std::function<BookImageResource(const std::string &, std::size_t)> &load,
    const BookInlineImageLimits &limits = {});
}
