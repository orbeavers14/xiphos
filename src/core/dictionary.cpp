// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/dictionary.hpp"
#include <swmodule.h>
#include <stdexcept>
namespace xiphos { namespace core {
std::vector<std::string> dictionary_nearby_keys(sword::SWModule &module) {
  std::vector<std::string> keys; keys.reserve(9);
  for (int i = 0; i < 5; ++i) module--;
  for (int i = 0; i < 9; ++i) { module++; keys.emplace_back(module.getKeyText()); }
  return keys;
}
std::string dictionary_step_key(sword::SWModule &module, int direction) {
  if (direction == -1) module--;
  else if (direction == 1) module++;
  else throw std::invalid_argument("Invalid dictionary direction");
  return module.getKeyText();
}
}}
