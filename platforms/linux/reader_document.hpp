// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <string>
namespace xiphos::linux_preview {
// Core CSS is copied from the same owned snapshot as body HTML. This host adds
// only its existing theme/resource policy; it does not choose lexical layout.
// Trusted isolated-world host program. Anchor is a copied data string, never
// module script. Preserves UTF-8 and escapes JSON string delimiters/controls.
std::string anchor_scroll_script(const std::string &anchor);
std::string document(const std::string &body, const std::string &css = {}, bool inline_images = false);
}
