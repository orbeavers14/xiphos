// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <swmgr.h>
#include <vector>
namespace xiphos { namespace core {
// The native reader's single owned manager exposes its protected filter set
// for exact scoped restoration. This does not create an additional engine.
class EngineManager : public sword::SWMgr {
public:
  using sword::SWMgr::SWMgr;
  std::vector<sword::SWOptionFilter *> option_filters();
};
} }
