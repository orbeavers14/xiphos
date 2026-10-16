// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <string>
namespace sword { class SWModule; }
namespace xiphos { namespace core {
struct StudyNote {
  std::string module, reference, identifier, type, label, html, reference_list;
};
// Regenerates/copies the note at an already positioned SDK key, without textual
// re-resolution. Shared by typed tree sources and existing passage wrappers.
// Caller owns key validation/serialization. Temporarily enables source-attribute
// capture and restores its flag, including on exceptions. Metadata-only html is empty.
StudyNote capture_study_note_at_key(sword::SWModule &module, const std::string &identifier, bool render_body);
// Regenerates entry attributes at the requested passage and copies note data
// before rendering its body. Matches legacy UTF8HTML/renderText processing.
// Caller owns the module and serializes access, including its key mutation.
StudyNote render_study_note(sword::SWModule &module, const std::string &reference,
                            const std::string &identifier);
// Same exact-source attribute generation and copied metadata, without a second
// note-body render. html is empty. Caller lifetime/key-serialization rules match.
StudyNote read_study_note_metadata(sword::SWModule &module, const std::string &reference,
                                   const std::string &identifier);
}}
