// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <optional>
#include <cstddef>
#include <string>
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

}
