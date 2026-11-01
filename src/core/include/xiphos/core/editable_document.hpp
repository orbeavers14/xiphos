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

#include <vector>
namespace xiphos::core {
// Shared bounded source undo/redo; native views own caret/layout only.
class EditorBuffer {
  std::string text_,saved_;
  std::vector<std::string> undo_,redo_;
  static void push(std::vector<std::string> &stack,const std::string &text);
public:
  explicit EditorBuffer(std::string text);
  const std::string &text() const {return text_;}
  bool dirty() const {return text_!=saved_;}
  bool can_undo() const {return !undo_.empty();}
  bool can_redo() const {return !redo_.empty();}
  void replace(std::string text);
  void undo();void redo();void mark_saved();
};
}
