// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <string>
namespace sword { class VerseKey; }
namespace xiphos { namespace core {
// Retained get_valid_key operation. Mutates the supplied key. force_normalize
// enables auto-normalization; false preserves its existing flag. Only exact
// locale "en" selects the short label; null/other locales select full text.
// Reference is a non-null borrowed SDK input. Caller serializes module access.
std::string navigation_key_text(sword::VerseKey &key, const char *reference,
                                bool force_normalize, const char *locale);
}}
