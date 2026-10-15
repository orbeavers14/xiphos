// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/headings.hpp"
#include "xiphos/core/entry_attribute.hpp"
#include <swmodule.h>
#include <utility>
namespace xiphos { namespace core {
namespace {
template<class Append> void collect_preverse(sword::SWModule &module, Append append) {
  for (size_t index = 0; ; ++index) {
    // BackEnd::get_entry_attribute defaults to render=true for each heading.
    // Refresh entry attributes after any previous buffer render.
    const auto position = std::to_string(index);
    const auto heading = entry_attribute(module, "Heading", "Preverse", position, true);
    if (heading.empty()) break;
    append(std::string(module.renderText(heading.c_str()).c_str()));
  }
}
}
std::vector<std::string> render_preverse_headings(sword::SWModule &module) {
  std::vector<std::string> result;
  collect_preverse(module, [&](std::string html) { result.push_back(std::move(html)); });
  return result;
}
std::vector<HeadingFragment> render_preverse_heading_fragments(sword::SWModule &module) {
  std::vector<HeadingFragment> result;
  collect_preverse(module, [&](std::string html) {
    auto label = entry_attribute(module, "Footnote", "1", "n", false);
    result.push_back({std::move(html), std::move(label)});
  });
  return result;
}
}}
