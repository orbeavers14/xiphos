// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
namespace sword { class SWModule; }
namespace xiphos { namespace core {
// Legacy OSIS/ThML/GBF/TEI XHTML label switch. Requires the serialized engine
// owner. Mutates all matching module render filters; not a scoped setting.
// SDK setters have no getter: native use requires an owned restoration policy.
void set_render_note_numbers(sword::SWModule &module, bool enabled);
}}
