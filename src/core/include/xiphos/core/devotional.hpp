// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <string>
#include <cstddef>
namespace xiphos { namespace core {
struct DevotionalDate {
  std::string key;
  int month; // One-based, always in 1..12 for safe month-name lookup.
  int day;
  bool used_today;
};
// Explicit read request limits; applied before worker admission/C copies.
constexpr std::size_t maximum_devotional_module_bytes = 4096;
constexpr std::size_t maximum_devotional_date_bytes = 64;
bool devotional_read_arguments(const std::string &module, const std::string &entered,
    int today_month, int today_day) noexcept;
// Selected date is separate from the SDK's nearest matching dictionary key.
struct DevotionalEntry {
  std::string module;
  DevotionalDate date;
  std::string matched_key, html;
};
// The host supplies local today (month 1..12, day 1..31); no clock/UI/SDK access.
// Retain the legacy five-byte key and atoi day semantics for safe months,
// including noncalendar days and separators. Other inputs use local today.
// Reject embedded NUL and invalid host dates. Owns all returned key bytes.
DevotionalDate devotional_date(const std::string &entered, int today_month, int today_day);
}}
