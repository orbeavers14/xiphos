// SPDX-License-Identifier: GPL-2.0-or-later
// Extracted from backend/sword_main.cc, Copyright (C) 2000-2026 Xiphos Developer Team.
#include "xiphos/core/navigation_key.hpp"
#include <versekey.h>
#include <cstring>
namespace xiphos { namespace core {
std::string navigation_key_text(sword::VerseKey &key, const char *reference,
                                bool force_normalize, const char *locale) {
  if (force_normalize) key.setAutoNormalize(1);
  key.setText(reference);
  return locale && !std::strcmp(locale, "en") ? key.getShortText() : key.getText();
}
}}
