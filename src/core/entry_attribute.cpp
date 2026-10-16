// SPDX-License-Identifier: GPL-2.0-or-later
#include "sdk_render.hpp"
// Xiphos BackEnd attribute reader, Copyright (C) 2000-2026 Xiphos Developer Team.
#include "xiphos/core/entry_attribute.hpp"
#include <swmodule.h>
#include <utf8html.h>
#include <stdexcept>
namespace xiphos { namespace core {
std::string entry_attribute(sword::SWModule &module, const std::string &group,
                             const std::string &item, const std::string &field, bool refresh) {
  if (group.find('\0') != std::string::npos || item.find('\0') != std::string::npos || field.find('\0') != std::string::npos)
    throw std::invalid_argument("Entry attribute path contains NUL");
  if (refresh) detail::render_sdk_entry(module);
  sword::SWBuf value = module.getEntryAttributes()[group.c_str()][item.c_str()][field.c_str()].c_str();
  sword::UTF8HTML conversion;
  conversion.processText(value);
  return value.c_str();
}
}}
