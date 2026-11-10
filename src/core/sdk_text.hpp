// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <swbuf.h>
#include <glib.h>
#include <cstring>
#include <stdexcept>
#include <memory>
#include <string>
namespace xiphos { namespace core { namespace detail {
// ThMLPlain can count one terminal NUL in SWBuf::size(). Exclude that terminator,
// then validate the full declared content: no hidden tail or replacement Unicode.
// The cap counts copied content, not SDK buffer capacity/temporary allocations.
inline std::string copy_sdk_text(const sword::SWBuf &buffer,std::size_t max_bytes){
  auto size=static_cast<std::size_t>(buffer.size());const auto *data=buffer.c_str();
  if(size&&data[size-1]=='\0')--size;
  if(size>max_bytes)throw std::length_error("SDK text exceeds copied capacity");
  if(std::memchr(data,'\0',size)||!g_utf8_validate(data,static_cast<gssize>(size),nullptr))
    throw std::runtime_error("SDK text is not NUL-free UTF-8");
  return {data,size};
}
inline std::string surface_key(const std::string &text, bool unmarked, bool sensitive) {
  using OwnedText = std::unique_ptr<char, decltype(&g_free)>;
  OwnedText folded(sensitive ? g_strdup(text.c_str()) : g_utf8_casefold(text.data(),text.size()), &g_free);
  if (!folded) throw std::runtime_error("Unable to case-fold surface text");
  OwnedText normalized(g_utf8_normalize(folded.get(),-1,G_NORMALIZE_NFD), &g_free);
  if (!normalized) throw std::runtime_error("Unable to normalize surface text");
  if (!unmarked) return normalized.get();
  std::string result;
  GUnicodeScript base = G_UNICODE_SCRIPT_UNKNOWN;
  for (const char *at = normalized.get(); *at; at = g_utf8_next_char(at)) {
    const auto scalar = g_utf8_get_char(at);
    if (g_unichar_ismark(scalar)) {
      if (base == G_UNICODE_SCRIPT_GREEK || base == G_UNICODE_SCRIPT_HEBREW ||
          base == G_UNICODE_SCRIPT_SYRIAC) continue;
    } else {
      base = g_unichar_get_script(scalar);
    }
    result.append(at,g_utf8_next_char(at));
  }
  return result;
}
}}}
