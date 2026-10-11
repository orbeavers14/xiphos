// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <string>
#include <variant>
namespace xiphos::core {
struct PassageLink { std::string module, reference; };
struct StudyNoteLink { std::string module, reference, identifier, type; };
struct ReferenceListLink { std::string module, expression; };
using ContextualStudyCommandLink = std::variant<PassageLink, StudyNoteLink, ReferenceListLink>;
// showRef commands retain lists/ranges for rendering with explicit source context.
ContextualStudyCommandLink contextual_study_link(const std::string &url, const std::string &current_module);
using StudyCommandLink = std::variant<PassageLink, StudyNoteLink>;
StudyCommandLink study_link(const std::string &url, const std::string &current_module);
StudyNoteLink study_note_link(const std::string &url, const std::string &current_module);
// Scoped single-reference subset of main/url.cc sword_uri + hex_decode.
// Module abbreviations, ranges and fragments remain pending.
PassageLink passage_link(const std::string &url, const std::string &current_module);
std::string passage_uri(const std::string &reference);
}
