// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <string>
namespace xiphos::linux_preview {
// Core CSS is copied from the same owned snapshot as body HTML. This host adds
// only its existing theme/resource policy; it does not choose lexical layout.
std::string document(const std::string &body, const std::string &css = {});
}
