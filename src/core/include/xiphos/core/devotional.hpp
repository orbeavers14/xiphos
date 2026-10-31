// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <string>
namespace xiphos { namespace core {
struct DevotionalDate {
  std::string key;
  int month; // One-based, always in 1..12 for safe month-name lookup.
  int day;
  bool used_today;
};
// The host supplies local today (month 1..12, day 1..31); no clock/UI/SDK access.
// Retain the legacy five-byte key and atoi day semantics for safe months,
// including noncalendar days and separators. Other inputs use local today.
// Reject embedded NUL and invalid host dates. Owns all returned key bytes.
DevotionalDate devotional_date(const std::string &entered, int today_month, int today_day);
}}
