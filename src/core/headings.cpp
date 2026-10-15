// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/headings.hpp"
#include "xiphos/core/entry_attribute.hpp"
#include <swmodule.h>
namespace xiphos { namespace core {
std::vector<std::string> render_preverse_headings(sword::SWModule &module) {
  std::vector<std::string> result;
  for (size_t index = 0; ; ++index) {
    // BackEnd::get_entry_attribute defaults to render=true for each heading.
    // Refresh entry attributes after any previous buffer render.
    const auto position = std::to_string(index);
    const auto heading = entry_attribute(module, "Heading", "Preverse", position, true);
    if (heading.empty()) break;
    result.push_back(module.renderText(heading.c_str()).c_str());
  }
  return result;
}
}}
