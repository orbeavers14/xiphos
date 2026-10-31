// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "history.hpp"
#include "dictionary_history.hpp"
#include "book_selection.hpp"
#include "lexical.hpp"
#include "parallel.hpp"
#include "presentation.hpp"
#include "search.hpp"
#include "tab_profile.hpp"
namespace xiphos::core {
inline constexpr std::size_t maximum_session_modules=1024;
inline constexpr std::size_t maximum_session_bytes=16*1024*1024;
// Host capabilities are not preferences and are never serialized.
struct SessionCapabilities {
  bool lexical_layout=false,note_numbering=false,heading_note_links=false;
};
struct SessionBookState {
  BookSelectionTarget requested;
  std::uint64_t offset=0;
  std::string path;
  bool document=false;
};
// Owned application intent only. No SDK keys, rendered HTML, assets, active
// jobs, search results/generation IDs or migration capability flags.
struct StudySessionState {
  std::optional<Location> reading;
  std::optional<DictionaryLocation> dictionary;
  std::vector<Location> history;
  std::size_t history_cursor=0;
  std::vector<DictionaryLocation> dictionary_history;
  std::size_t dictionary_cursor=0;
  std::optional<SessionBookState> book;
  std::optional<ParallelSelection> parallel;
  std::optional<EngineOptions> engine;
  ModuleEnginePreferences module_engine;
  ModulePresentationPreferences presentation;
  bool module_defaults=false,cross_reference_preview_list=true;
  std::optional<LexiconPreferences> lexicons;
  std::optional<SearchTextOptions> search_text;
  SearchRetentionLimits search_retention;
};
struct NativeSessionTab {
  std::string key; // Opaque host identity, independent of live worker IDs.
  StudySessionState state;
  TabVisibility visibility;
  bool commentary_showing=false,parallel_showing=false;
};
struct NativeSessionSnapshot {
  unsigned version=1;
  std::vector<NativeSessionTab> tabs;
  std::optional<std::string> active_key;
};
// Full structural bounds validation without requiring installed modules.
void validate_session_state(const StudySessionState &state);
void validate_native_session(const NativeSessionSnapshot &snapshot);
std::string serialize_native_session(const NativeSessionSnapshot &snapshot);
NativeSessionSnapshot parse_native_session(const std::string &xml);
NativeSessionSnapshot read_native_session(const std::string &path);
NativeSessionSnapshot save_native_session(const std::string &path,const NativeSessionSnapshot &snapshot);
}
