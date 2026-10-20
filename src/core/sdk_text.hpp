// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <swbuf.h>
#include <glib.h>
#include <cstring>
#include <stdexcept>
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
}}}
