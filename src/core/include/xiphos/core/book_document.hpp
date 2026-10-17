// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "book_tree.hpp"
#include "content.hpp"
#include "book_resource.hpp"
#include <cstddef>
namespace xiphos::core {
// Resource bounds reject the whole result rather than publishing a partial book.
// Hosts may choose smaller bounds; the defaults are also the supported maxima.
struct BookDocumentLimits {
  std::uint32_t max_depth = 64, max_nodes = 4096;
  std::size_t max_html_bytes = 8 * 1024 * 1024;
};
struct BookDocumentSection {
  BookEntry entry;
  std::uint32_t depth = 1; // Selected node is level one, as in legacy display.
  bool rendered = false; // Empty raw sections are skipped when DisplayLevel >1.
};
struct BookDocumentPresentation {
  std::string html;
  LexicalPresentation lexical;
  std::vector<ContentCleanupDiagnostic> cleanup_diagnostics;
  ContentPreference image_content = ContentPreference::unknown;
  ContentPreference respect_font_faces = ContentPreference::unknown;
  bool double_space = false;
  std::optional<BookInlineImages> inline_images;
  std::optional<BookStylesheetResource> stylesheet;
  std::optional<std::string> stylesheet_issue;
};
// Legacy combined CleanupContent reads the live final render's Footnote/1/n
// after traversal restores the selected key. Empty skipped nodes do not replace
// that metadata. Absent source means the Reader capture capability was not used.
struct BookDocumentNoteSource {
  std::string reported_label;
  std::optional<std::uint64_t> last_rendered_offset;
};
struct BookDocument {
  BookTree tree; // Selected source, distinct from descendant/linked section sources.
  std::uint32_t display_level = 1;
  std::vector<BookDocumentSection> sections; // Visited nodes, SDK depth-first order.
  std::string html; // Raw combined SDK HTML remains available for comparison.
  // Only application document selection adds presentation; raw reads leave it absent.
  std::optional<BookDocumentPresentation> presentation;
  std::optional<BookDocumentNoteSource> note_source;
};
// Legacy atoi-defined prefix/default/sign behavior, with explicit overflow/NUL
// rejection. Values <=1 (including missing/nonnumeric) mean single-entry display.
std::uint32_t book_display_level(const std::optional<std::string> &configured);
}
