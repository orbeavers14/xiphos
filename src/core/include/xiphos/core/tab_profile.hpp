// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <cstddef>
#include <optional>
#include <string>
#include <vector>
namespace xiphos::core {
inline constexpr std::size_t maximum_saved_tabs=256;
inline constexpr std::size_t maximum_saved_tab_text_bytes=32768;
inline constexpr std::size_t maximum_saved_tab_snapshot_bytes=16*1024*1024;
struct TabVisibility {
  bool texts=true,preview=true,commentaries=true,dictionaries=true;
};
struct SavedTab {
  std::optional<std::string> text_module,commentary_module,dictionary_module,book_module;
  std::optional<std::string> reference,dictionary_key,book_offset_text;
  std::optional<std::string> commentary_showing,show_texts,show_preview,show_commentaries,show_dictionaries,show_parallel;
};
struct SavedTabSession {
  std::optional<std::string> version;
  std::vector<SavedTab> tabs;
  std::size_t snapshot_bytes=0;
};
// Exact legacy yes token; absent/mixed-case/other values are false. Per-tab pane
// visibility inherits all four global flags only when showtexts is absent.
bool saved_tab_yes(const std::optional<std::string> &text);
TabVisibility saved_tab_visibility(const SavedTab &tab,TabVisibility fallback);
// Explicit read-only Xiphos_Tabs XML. All tabs sections contribute in order;
// raw offsets, missing/empty attributes and unknown module identities are retained.
// No default tab creation, selected tab, home path, write or rendering side effect.
SavedTabSession parse_saved_tab_session(const std::string &xml);
SavedTabSession read_saved_tab_session(const std::string &path);
// Positional legacy export: reuse existing tab nodes/unknown data at each index,
// remove surplus owned tab nodes and append new ones to the last tabs section.
// Raw optional attributes round trip; no effective-default rewrite is implied.
std::string serialize_saved_tab_session(const SavedTabSession &session,const std::string &original = "<Xiphos_Tabs/>");
SavedTabSession save_saved_tab_session(const std::string &path,const SavedTabSession &session);
}
