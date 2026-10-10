// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <string>
namespace xiphos::core {
struct PassageLink { std::string module, reference; };
// Scoped single-reference subset of main/url.cc sword_uri + hex_decode.
// Module abbreviations, ranges, fragments and passagestudy actions are pending.
PassageLink passage_link(const std::string &url, const std::string &current_module);
std::string passage_uri(const std::string &reference);
}
