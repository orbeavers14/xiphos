// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <cstdint>
#include <string>
namespace xiphos::core {
// HTTPS endpoints for the download layer, separate from SDK installation.
// Package location is explicit: repositories do not all use raw/packages/.
// No insecure scheme or automatic FTP fallback is represented by this type.
struct HttpsRepository {
 std::string id,host,catalog_directory,package_directory;
 std::uint16_t port=443;
};
std::string repository_catalog_url(const HttpsRepository &repository);
std::string repository_module_url(const HttpsRepository &repository,const std::string &module);
}
