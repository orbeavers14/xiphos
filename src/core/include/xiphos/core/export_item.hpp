// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <string>
namespace xiphos { namespace core {
// Retained item template. Metadata escapes only in HTML; supplied scripture is
// already-rendered HTML or stripped text. Does not read modules or write files.
// Owned inputs reject embedded NUL rather than silently truncating metadata.
std::string format_export_item(const std::string &description,const std::string &module,
                              const std::string &key,const std::string &scripture,bool html,bool scripture_present=true);
}}
