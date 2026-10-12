// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/headings.hpp"
#include <swmodule.h>
#include <utf8html.h>
namespace xiphos { namespace core {
std::vector<std::string> render_preverse_headings(sword::SWModule &module) {
  std::vector<std::string> result;
  sword::UTF8HTML conversion;
  for (size_t index = 0; ; ++index) {
    // BackEnd::get_entry_attribute defaults to render=true for each heading.
    // Refresh entry attributes after any previous buffer render.
    module.renderText();
    const auto position = std::to_string(index);
    sword::SWBuf heading = module.getEntryAttributes()["Heading"]["Preverse"][position.c_str()].c_str();
    conversion.processText(heading);
    if (!heading.length()) break;
    result.push_back(module.renderText(heading.c_str()).c_str());
  }
  return result;
}
}}
