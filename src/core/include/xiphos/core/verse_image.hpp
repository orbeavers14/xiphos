// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <cstddef>
#include <string>
namespace xiphos::core {
inline constexpr std::size_t maximum_verse_image_bytes=16*1024*1024;
// Copied source identity and binary bytes. MIME is an extension hint, not proof
// of decoder support or permission to embed active formats in an export.
struct VerseImageResource {std::string module,source,media_type,data;};
}
