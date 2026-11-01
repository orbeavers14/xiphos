// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "library.hpp"
namespace xiphos::core {
struct LocalInstallPreview {
 std::string module,source_revision,destination_revision,source_version,installed_version;
 LibraryInstallDecision decision=LibraryInstallDecision::unsupported;
};
struct LocalInstallRequest {
 std::string source,module,source_revision,destination_revision;
 bool replace=false;
};
struct LocalInstallResult {
 std::string module,version,source_revision,destination_revision;
 bool replaced=false;
};
bool local_install_arguments(const LocalInstallRequest &request) noexcept;
}
