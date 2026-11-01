// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "session_state.hpp"
#include <map>
namespace xiphos::core {
class Reader;
struct LegacyMigrationInput {std::string settings,tabs,module_options;};
struct LegacyMigrationSource {unsigned kind=0;std::string bytes,revision,path;}; // 0 settings,1 tabs,2 modops; path is NOT serialized.
struct LegacyMigrationSetting {std::string section,key;std::optional<std::string> value;bool mapped=false;};
struct LegacyMigrationIssue {std::size_t tab=0;std::string module,reason;};
struct LegacyMigration {
 std::vector<LegacyMigrationSource> sources;
 std::vector<LegacyMigrationSetting> settings;
 std::vector<LegacyMigrationIssue> issues;
 std::optional<NativeSessionSnapshot> session; // Present only for a complete, validated conversion.
 std::string revision;
};
using LegacyModuleMapping=std::map<std::string,std::string>;
inline constexpr std::size_t maximum_legacy_migration_bytes=80*1024*1024;
// All IO/SDK resolution belongs on the existing worker. Sources stay untouched;
// mapped state stages in temporary sessions, never live tabs. No alias/default
// module substitution. Unavailable required content blocks the whole conversion.
LegacyMigration prepare_legacy_migration(Reader &reader,const LegacyMigrationInput &input,const LegacyModuleMapping &mapping={});
LegacyMigration replan_legacy_migration(Reader &reader,LegacyMigration migration,const LegacyModuleMapping &mapping={});
std::string serialize_legacy_migration(const LegacyMigration &migration);
LegacyMigration parse_legacy_migration(const std::string &bytes);
LegacyMigration read_legacy_migration(const std::string &path);
// New file only; stage/read-back verify before replacement. Recheck selected
// source revisions when this is a live preview. Imported bundles have no paths.
LegacyMigration save_legacy_migration(const std::string &path,const LegacyMigration &migration);
// Exact original bytes into a NEW private folder only. Never overwrite originals
// or merge an existing directory. Roll back only files created by this operation.
NativeSessionSnapshot export_legacy_migration_session(const std::string &path,const LegacyMigration &migration);
void restore_legacy_migration_sources(const std::string &folder,const LegacyMigration &migration);
}
