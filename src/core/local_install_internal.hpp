// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "xiphos/core/local_install.hpp"
#include <memory>
namespace xiphos::core::detail {
std::string local_library_revision(const std::string &root,const std::string &workspace_parent);
// Entire explicit library root swaps as one staged unit. Two renames, with
// rollback/backup preservation; not crash-atomic or interprocess locked.
class LocalInstallStorage {
 struct Impl;std::unique_ptr<Impl> impl_;
public:
 LocalInstallStorage(const std::string &source,const std::string &destination,const std::string &module);
 ~LocalInstallStorage();
 LocalInstallPreview preview();
 LocalInstallResult stage(const LocalInstallRequest &request);
 void install();
 void rollback();
 void finish() noexcept;
};
}
