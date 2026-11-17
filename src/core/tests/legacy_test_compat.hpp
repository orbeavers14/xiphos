// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#ifdef _MSC_VER
#include <cstring>
#include <glib.h>

inline const char *strcasestr(const char *haystack, const char *needle) {
  const auto length = std::strlen(needle);
  do {
    if (g_ascii_strncasecmp(haystack, needle, length) == 0) return haystack;
  } while (*haystack++);
  return nullptr;
}
#endif
