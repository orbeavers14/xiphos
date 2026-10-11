// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <string>
#include <variant>
namespace xiphos::core {
struct PassageLink { std::string module, reference; };
struct StudyNoteLink { std::string module, reference, identifier, type; };
using StudyCommandLink = std::variant<PassageLink, StudyNoteLink>;
StudyCommandLink study_link(const std::string &url, const std::string &current_module);
StudyNoteLink study_note_link(const std::string &url, const std::string &current_module);
// Scoped single-reference subset of main/url.cc sword_uri + hex_decode.
// Module abbreviations, ranges and fragments remain pending.
PassageLink passage_link(const std::string &url, const std::string &current_module);
std::string passage_uri(const std::string &reference);
}
