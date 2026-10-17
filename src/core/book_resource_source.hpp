// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "xiphos/core/book_resource.hpp"
namespace sword { class SWModule; }
namespace xiphos::core::detail {
BookResourceContext capture_book_resource_context(sword::SWModule &module);
std::optional<BookStylesheetResource> capture_book_stylesheet_resource(sword::SWModule &module, std::size_t maximum_bytes);
BookImageResource capture_book_image_resource(sword::SWModule &module, BookImageLink command, std::size_t maximum_bytes);
}
