// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/dictionary.hpp"
#include <glib.h>
#include <memory>
#include <stdexcept>
#if !GLIB_CHECK_VERSION(2, 32, 0)
#error GLib 2.32 or newer is required
#endif
namespace xiphos { namespace core {
std::string dictionary_click_key(const std::string &key) {
  if (key.size() > static_cast<std::size_t>(G_MAXSSIZE) || key.find('\0') != std::string::npos ||
      !g_utf8_validate(key.data(), static_cast<gssize>(key.size()), nullptr))
    throw std::invalid_argument("Dictionary click key must be valid UTF-8 without NUL");
  const std::unique_ptr<gchar, decltype(&g_free)> upper(g_utf8_strup(key.c_str(), -1), &g_free);
  return upper.get();
}
}}
