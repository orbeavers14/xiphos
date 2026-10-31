// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
namespace xiphos::core {
inline constexpr std::size_t maximum_editable_document_bytes=4*1024*1024;
struct EditableDocument { std::string text,revision; };
// Serialized editor source, not rendered HTML or a SWORD module write. UTF-8
// bytes (including BOM/line endings/markup/%20) are retained without conversion.
bool editable_document_text(std::string_view text) noexcept;
bool editable_document_revision(std::string_view revision) noexcept;
EditableDocument read_editable_document(const std::string &path);
// Expected SHA256 is required for replacement; absent means create only if the
// file is still missing. Verified same-directory atomic replacement, with owned
// result prepared before commit. Observed change checks are not a file lock.
EditableDocument save_editable_document(const std::string &path,const std::optional<std::string> &expected_revision,const std::string &text);
}
