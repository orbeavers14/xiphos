// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <cstdint>
#include <string>
#include <vector>
namespace xiphos::core {
// Provider-scoped identities: an edition from another provider never replaces
// a SWORD module merely because their display names happen to match.
struct LibraryIdentity { std::string provider, item; };
enum class LibraryAccess { available=0, sign_in_required, license_required, locked, unavailable };
enum class LibraryDelivery { none=0, local_sword, provider_package };
enum class LibraryPermission { unknown=0, allowed, denied };
enum LibraryCapability : std::uint32_t {
 library_online_read=1, library_offline_read=2, library_local_search=4,
 library_provider_search=8, library_annotations=16, library_cross_references=32,
 library_text_export=64, library_local_install=128, library_offline_package=256
};
struct LibraryItem {
 LibraryIdentity identity;
 std::string title, language, type, translation, edition, version, versification;
 std::string attribution, license;
 std::uint32_t capabilities=0;
 LibraryAccess access=LibraryAccess::unavailable;
 LibraryDelivery delivery=LibraryDelivery::none;
 bool installed=false;
 // Technical capabilities do not imply permission to download/store offline/export.
 // Adapters must explicitly supply verified grants; absence stays unknown.
 LibraryPermission download=LibraryPermission::unknown, offline_storage=LibraryPermission::unknown,
                   text_export=LibraryPermission::unknown;
};
struct LibraryCatalog { std::string source; std::vector<LibraryItem> items; };
// Catalog acquisition only. No alternate navigation/history/rendering engine,
// credential storage or callback execution is introduced by this boundary.
class LibraryProvider {
public:
 virtual ~LibraryProvider()=default;
 virtual LibraryCatalog catalog()=0;
};
void validate_library_catalog(const LibraryCatalog &catalog);
std::string library_catalog_revision(const LibraryCatalog &catalog);
enum class LibraryInstallDecision {
 install=0, replace_confirmation_required, already_installed, sign_in_required,
 license_required, unavailable, unsupported, permission_unknown, permission_denied
};
struct LibraryInstallPlan {
 LibraryIdentity identity;
 LibraryInstallDecision decision=LibraryInstallDecision::unsupported;
 LibraryDelivery delivery=LibraryDelivery::none;
 std::string source_revision, destination_revision, source_version, installed_version;
};
// Pure planning, no network/filesystem/SDK mutation. Versions are opaque: a
// differing/unknown version requires explicit replacement, never guessed ordering.
LibraryInstallPlan plan_library_install(const LibraryCatalog &source,
 const LibraryCatalog &installed,const LibraryIdentity &identity);
}
