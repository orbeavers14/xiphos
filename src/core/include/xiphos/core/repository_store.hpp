// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "remote_install.hpp"
#include <optional>
#include <vector>
namespace xiphos::core {
// Explicit profile file; no implicit home-directory discovery or network access.
// Cached metadata is for offline browsing. It is not proof of freshness, access
// rights, or remote-access acknowledgement. Installation rechecks its destination.
struct SavedRepository {
 HttpsRepository endpoint;
 std::string name;
 bool enabled=true;
 std::optional<RemoteCatalog> cached;
 std::uint64_t fetched_at=0; // caller-supplied Unix seconds, zero means unknown
};
struct RepositorySnapshot {std::vector<SavedRepository> repositories;std::string revision;};
inline constexpr std::size_t maximum_repository_store_bytes=32*1024*1024;
// Value edits retain the loaded file revision. Endpoint changes discard its cache.
RepositorySnapshot put_repository(RepositorySnapshot snapshot,HttpsRepository endpoint,std::string name,bool enabled);
RepositorySnapshot remove_repository(RepositorySnapshot snapshot,const std::string &id);
RepositorySnapshot move_repository(RepositorySnapshot snapshot,const std::string &id,std::size_t position);
RepositorySnapshot cache_repository(RepositorySnapshot snapshot,RemoteCatalog catalog,std::uint64_t fetched_at);
void validate_repository_snapshot(const RepositorySnapshot &snapshot);
std::string serialize_repository_snapshot(const RepositorySnapshot &snapshot);
RepositorySnapshot parse_repository_snapshot(const std::string &bytes);
RepositorySnapshot read_repository_snapshot(const std::string &path);
// null revision means create only. Unknown/future formats are never overwritten.
RepositorySnapshot save_repository_snapshot(const std::string &path,
 const std::optional<std::string> &expected_revision,const RepositorySnapshot &snapshot);
}
