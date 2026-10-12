// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <string>
namespace sword { class SWModule; }
namespace xiphos { namespace core {
struct StudyNote {
  std::string module, reference, identifier, type, label, html, reference_list;
};
// Regenerates entry attributes at the requested passage and copies note data
// before rendering its body. Matches legacy UTF8HTML/renderText processing.
// Caller owns the module and serializes access, including its key mutation.
StudyNote render_study_note(sword::SWModule &module, const std::string &reference,
                            const std::string &identifier);
}}
