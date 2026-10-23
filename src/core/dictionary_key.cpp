// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/dictionary.hpp"
#include <glib.h>
#include <memory>
#include <cstring>
#include <stdexcept>
#if !GLIB_CHECK_VERSION(2, 32, 0)
#error GLib 2.32 or newer is required
#endif
namespace xiphos { namespace core {
DictionaryDisplayRoute dictionary_display_route(const char *current_dictionary,
    const char *target_module, const char *feature, const char *key) noexcept {
  if (!current_dictionary || !target_module || !feature ||
      std::strcmp(current_dictionary, target_module) == 0 || std::strcmp(feature, "DailyDevotion") != 0)
    return DictionaryDisplayRoute::dictionary;
  if (key && key[0] >= '0' && key[0] <= '9' && key[1] >= '0' && key[1] <= '9' &&
      key[2] == '.' && key[3] >= '0' && key[3] <= '9' && key[4] >= '0' && key[4] <= '9' && key[5] == '\0')
    return DictionaryDisplayRoute::devotional_key;
  return DictionaryDisplayRoute::devotional_today;
}
std::string dictionary_click_key(const std::string &key) {
  if (key.size() > static_cast<std::size_t>(G_MAXSSIZE) || key.find('\0') != std::string::npos ||
      !g_utf8_validate(key.data(), static_cast<gssize>(key.size()), nullptr))
    throw std::invalid_argument("Dictionary click key must be valid UTF-8 without NUL");
  const std::unique_ptr<gchar, decltype(&g_free)> upper(g_utf8_strup(key.c_str(), -1), &g_free);
  return upper.get();
}
}}
