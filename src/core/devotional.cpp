// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/devotional.hpp"
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
namespace xiphos { namespace core {
DevotionalDate devotional_date(const std::string &entered, int today_month, int today_day) {
  if (today_month < 1 || today_month > 12 || today_day < 1 || today_day > 31)
    throw std::invalid_argument("Invalid local date for devotional selection");
  if (entered.find('\0') != std::string::npos)
    throw std::invalid_argument("Devotional date must not contain NUL");
  if (entered.size() == 5) {
    const int month = std::atoi(entered.c_str());
    if (month >= 1 && month <= 12)
      return {entered, month, std::atoi(entered.c_str() + 3), false};
  }
  char key[6];
  std::snprintf(key, sizeof key, "%02d.%02d", today_month, today_day);
  return {key, today_month, today_day, true};
}
}}
