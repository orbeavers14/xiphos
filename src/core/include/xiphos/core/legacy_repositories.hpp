// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "repository_store.hpp"
#include <cstddef>
namespace xiphos::core {
enum class LegacyRepositoryDisposition {https_review=0,insecure_transport,local_directory,credentials,unsupported,malformed};
struct LegacyRepositoryCandidate {
 std::size_t line=0;
 LegacyRepositoryDisposition disposition=LegacyRepositoryDisposition::malformed;
 // Only credential-free, validated HTTPS candidates publish endpoint fields.
 std::string name,host,catalog_directory;
};
struct LegacyRepositoryPreview {std::vector<LegacyRepositoryCandidate> candidates;};
inline constexpr std::size_t maximum_legacy_repository_bytes=1024*1024;
// Read-only explicit input. Unknown sections are ignored; recognized source rows
// retain order/line numbers. Credentials and raw rejected entries never escape.
// No home discovery, network access, legacy writes or silent FTP/HTTP upgrade.
LegacyRepositoryPreview preview_legacy_repositories(const std::string &bytes);
LegacyRepositoryPreview read_legacy_repositories(const std::string &path);
// Explicit caller selection and package directory are required; the old raw
// catalog directory does not establish where HTTPS packages are hosted. Returns
// a value only, disabled/uncached. Save through the normal revision-checked store.
RepositorySnapshot accept_legacy_repository(RepositorySnapshot snapshot,
 const LegacyRepositoryCandidate &candidate,std::string new_id,std::string package_directory);
}
