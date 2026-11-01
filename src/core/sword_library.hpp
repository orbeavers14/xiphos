// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "xiphos/core/library.hpp"
#include <swmgr.h>
namespace xiphos::core::detail {
LibraryCatalog sword_library_catalog(sword::SWMgr &manager,const std::string &source,bool installed);
LibraryCatalog local_sword_library_catalog(const std::string &root);
}
