// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "session_state.hpp"
#include "editable_document.hpp"
#include "module_edit.hpp"
namespace xiphos::core {
enum class RecoveryTarget {untitled=0,file,module};
struct RecoveryDraft {
 std::string id,title;
 RecoveryTarget target=RecoveryTarget::untitled;
 std::string path;
 ModuleEditTarget module;
 std::optional<std::string> origin_revision;
 std::string saved_text,text; // exact UTF-8 source, no rendered HTML conversion
 std::size_t selection_start=0,selection_end=0; // UTF-8 byte offsets
};
struct RecoverySnapshot {
 NativeSessionSnapshot session;
 std::vector<RecoveryDraft> drafts;
 std::string revision;
 bool recovered_from_previous=false;
};
inline constexpr std::size_t maximum_recovery_bytes=40*1024*1024;
void validate_recovery_snapshot(const RecoverySnapshot &snapshot);
std::string serialize_recovery_snapshot(const RecoverySnapshot &snapshot);
RecoverySnapshot parse_recovery_snapshot(const std::string &bytes);
// Reads primary, then validated .previous on missing/corrupt primary. Does not
// restore tabs, overwrite files, or write module sources. Caller presents recovery.
RecoverySnapshot read_recovery_snapshot(const std::string &path);
// One verified profile replacement; retains the last valid primary in .previous.
// A fallback cannot silently overwrite a damaged/future primary. Save it to a new
// path or explicitly resolve the existing file outside this operation.
RecoverySnapshot save_recovery_snapshot(const std::string &path,
 const std::optional<std::string> &expected_revision,const RecoverySnapshot &snapshot);
// Compare with freshly read source revision before offering a normal Save.
// A conflict leaves the recovered text available for Save As/manual resolution.
bool recovery_draft_source_matches(const RecoveryDraft &draft,const std::optional<std::string> &current_revision);
}
