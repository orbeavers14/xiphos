// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/dictionary.hpp"
#include <swmodule.h>
#include <stdexcept>
namespace xiphos { namespace core {
std::string dictionary_step_key(sword::SWModule &module, int direction) {
  if (direction == -1) module--;
  else if (direction == 1) module++;
  else throw std::invalid_argument("Invalid dictionary direction");
  return module.getKeyText();
}
}}
