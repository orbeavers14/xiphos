// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <array>
#include <cstddef>
#include <optional>
#include <string>
#include <vector>
namespace xiphos::core {
inline constexpr std::size_t maximum_saved_parallel_sets = 1024;
inline constexpr std::size_t maximum_saved_parallel_modules = 4096;
inline constexpr std::size_t maximum_saved_parallel_key_bytes = 4096;
inline constexpr std::size_t maximum_saved_parallel_snapshot_bytes = 16 * 1024 * 1024;
// Exact legacy comma split (including internal/trailing empties); empty text ->
// no tokens. Preserve order/duplicates/spaces; no module lookup or rendering.
std::vector<std::string> saved_parallel_module_names(const std::string &text);
// Legacy byte substitutions, not a lossless display-name encoding. These helpers
// preserve empty text; native edit validation is a separate boundary.
std::string parallel_set_key(const std::string &display_name);
std::string parallel_set_display_name(const std::string &key);
struct SavedParallelSet {
  std::string key, display_name;
  std::optional<std::string> module_text;
  std::optional<std::vector<std::string>> modules; // NULL/empty text -> absent.
};
enum class ParallelStartupSource { none, configured, legacy_slots, named_set };
struct SavedParallelProfile {
  std::optional<std::string> names_text, current_key, active_modules_text, startup_modules_text;
  std::array<std::optional<std::string>,5> legacy_slots;
  std::vector<SavedParallelSet> sets;
  std::optional<std::vector<std::string>> startup_modules;
  ParallelStartupSource startup_source = ParallelStartupSource::none;
  std::size_t snapshot_bytes = 0;
};
// Explicit read-only legacy XML subset, including startup fallback ordering.
// Named/active token counts have storage bounds distinct from the32-column
// render bound; never silently trim stored lists. Unknown XML is not modified.
SavedParallelProfile parse_saved_parallel_profile(const std::string &xml);
SavedParallelProfile read_saved_parallel_profile(const std::string &path);
enum class ParallelProfileAction { save, rename, remove, activate };
struct ParallelProfileEdit {
  ParallelProfileAction action = ParallelProfileAction::save;
  std::string key, replacement_key; // Raw stored identity, never a display label.
  std::vector<std::string> modules; // Used only by save; commas are not escapable.
  bool make_current = false; // Save only; activation is explicit otherwise.
};
// Mutate only owned parallel fields; preserve unrelated nodes/attributes/comments
// and shadowed duplicates. New keys must be bounded XML local names after the
// legacy prefix; reserved metadata suffixes names/current reject. No implicit
// naming conversion, module resolution, rendering or home/profile discovery.
void validate_parallel_profile_edit(const ParallelProfileEdit &edit);
std::string serialize_parallel_profile_edit(const std::string &xml, const ParallelProfileEdit &edit);
// Verified same-directory atomic replacement; missing files may be created only
// by save. Prepared owned snapshot is returned without post-replacement reads.
SavedParallelProfile edit_parallel_profile(const std::string &path, const ParallelProfileEdit &edit);

}
