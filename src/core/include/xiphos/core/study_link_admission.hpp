// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <cstddef>
#include <string_view>
namespace xiphos::core {
inline constexpr std::size_t maximum_study_link_uri_bytes = 32768;
inline constexpr std::size_t maximum_study_link_context_bytes = 4096;
namespace detail {
// Admission only: syntax, installed modules and tab state stay on the owner.
inline bool study_link_text(std::string_view value, std::size_t maximum) noexcept {
  return value.size() <= maximum && value.find('\0') == std::string_view::npos;
}
inline bool study_link_arguments(std::string_view module, std::string_view reference,
                                 std::string_view uri, std::string_view navigation = {}) noexcept {
  return study_link_text(module, maximum_study_link_context_bytes) &&
         study_link_text(navigation, maximum_study_link_context_bytes) &&
         study_link_text(reference, maximum_study_link_context_bytes) &&
         study_link_text(uri, maximum_study_link_uri_bytes);
}
// Scan at most maximum+1 bytes before any C-string-to-owned-string conversion.
// Null/empty requirements remain explicit at each existing ABI entry point.
inline bool study_link_c_text(const char *value, std::size_t maximum, bool required) noexcept {
  if (!value) return !required;
  if (!*value) return !required;
  for (std::size_t i = 0; i <= maximum; ++i) if (!value[i]) return true;
  return false;
}
}}
