// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "library.hpp"
#include <optional>
namespace xiphos::core {
struct LibraryOrganizationEntry {
  LibraryIdentity identity;
  bool favorite=false,hidden=false;
  std::string group;
};
struct LibraryOrganizationSnapshot {
  std::string revision;
  std::vector<LibraryOrganizationEntry> entries;
  std::size_t ignored=0;
};
// Explicit metadata file only. Provider/item identity is exact; preferences do
// not alter installation, access grants, catalog order or module source files.
LibraryOrganizationSnapshot parse_library_organization(const std::string &xml);
LibraryOrganizationSnapshot read_library_organization(const std::string &path);
// Null revision creates only a missing file. Existing files require the exact
// snapshot revision. Unknown XML survives; setting defaults resets preferences.
LibraryOrganizationSnapshot set_library_organization(const std::string &path,const std::optional<std::string> &revision,const LibraryOrganizationEntry &entry);
}
