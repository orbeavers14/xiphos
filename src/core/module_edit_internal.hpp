// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "xiphos/core/module_edit.hpp"
#include <swmodule.h>
#include <swmgr.h>
#include <filesystem>
#include <memory>
namespace xiphos::core::detail {
// Same-owner serialized transaction. Old SDK remains alive until the caller
// verifies a new manager and stages replacement tab caches. Not crash-atomic or
// an interprocess lock: two directory renames are needed on portable filesystems.
class ModuleEditStorage {
  struct Impl;
  std::unique_ptr<Impl> impl_;
public:
  ModuleEditStorage(sword::SWMgr &manager,const ModuleEditTarget &target);
  ~ModuleEditStorage();
  ModuleEditEntry read();
  ModuleEditEntry stage(const std::string &expected,const std::string &text,bool erase);
  void install();
  void rollback();
  void finish() noexcept;
};
}
