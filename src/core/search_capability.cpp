// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2004-2026 Xiphos Developer Team
// Shared from BackEnd::check_for_optimal_search; existing SDK probe retained.
#include "xiphos/core/search_policy.hpp"
#include <swmodule.h>
namespace xiphos { namespace core {
bool supports_indexed_search(sword::SWModule &module) {
  return module.hasSearchFramework()&&module.isSearchOptimallySupported("God",-4,0,0);
}
}}
