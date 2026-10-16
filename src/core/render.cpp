// SPDX-License-Identifier: GPL-2.0-or-later
#include "sdk_render.hpp"
#include "xiphos/core/render.hpp"
#include <swmodule.h>
namespace xiphos { namespace core {
std::string render_entry(sword::SWModule &module, const char *key) {
  module.setKey(key);
  return detail::render_sdk_entry(module).c_str();
}
}}
