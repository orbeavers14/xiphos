// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/render.hpp"
#include <swmodule.h>
namespace xiphos { namespace core {
std::string render_entry(sword::SWModule &module, const char *key) {
  module.setKey(key);
  return module.renderText().c_str();
}
}}
