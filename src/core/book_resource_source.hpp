// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "xiphos/core/book_resource.hpp"
namespace sword { class SWModule; }
namespace xiphos::core::detail {
BookResourceContext capture_book_resource_context(sword::SWModule &module);
}
