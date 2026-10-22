// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
namespace sword { class SWModule; class SWBuf; }
namespace xiphos { namespace core { namespace detail {
// Internal SDK boundary. Retains exact renderText output/dataflow and restores
// the caller's entry-attribute processing flag on success or filter exceptions.
// Does not roll back keys/attributes; ordinary source renders must populate them.
// The caller serializes module access and owns its lifetime/filter configuration.
sword::SWBuf render_sdk_entry(sword::SWModule &module);
sword::SWBuf render_sdk_buffer(sword::SWModule &module, const char *text);
// Preserve counted strip-filter bytes on released SDKs whose stripText exposes
// only a NUL-terminated pointer. NULL text strips the current entry.
sword::SWBuf strip_sdk_buffer(sword::SWModule &module, const char *text);
}}}
