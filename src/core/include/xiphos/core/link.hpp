// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <string>
#include <cstdint>
#include "book_resource.hpp"
#include <variant>
namespace xiphos::core {
enum class LexicalLinkKind { strongs, morphology };
// Parsed source context, not a selected dictionary. Selection is a separate policy.
struct LexicalLink { LexicalLinkKind kind; std::string module, type, value, passage; };
LexicalLink lexical_link(const std::string &url, const std::string &current_module);
struct PassageLink { std::string module, reference; };
struct StudyNoteLink { std::string module, reference, identifier, type; };
struct ReferenceListLink { std::string module, expression; };
using ContextualStudyCommandLink = std::variant<PassageLink, StudyNoteLink, ReferenceListLink, LexicalLink>;
// showRef commands retain lists/ranges for rendering with explicit source context.
ContextualStudyCommandLink contextual_study_link(const std::string &url, const std::string &current_module);
// Main-pane sword/bible links default to the main Bible; note/lexical commands
// retain their source pane module. Explicit URI module names are preserved.
ContextualStudyCommandLink contextual_study_link(const std::string &url, const std::string &source_module,
                                                 const std::string &navigation_module);
// Main-hover note parsing adds the characterized short/missing passage fallback
// (<5 decoded bytes) to committed source context. Other parsers remain strict.
ContextualStudyCommandLink contextual_hover_link(const std::string &url, const std::string &current_module,
                                                 const std::string &current_reference);
using StudyCommandLink = std::variant<PassageLink, StudyNoteLink>;
StudyCommandLink study_link(const std::string &url, const std::string &current_module);
StudyNoteLink study_note_link(const std::string &url, const std::string &current_module);
// Exact source-bound book command. Context comes from an owned SDK tree node;
// slash paths are never fed to the verse parser or short-passage fallback.
StudyNoteLink book_note_link(const std::string &url, const std::string &module, const std::string &path);
struct BookDocument;
struct BookDocumentNoteTarget { std::uint64_t offset; StudyNoteLink command; };
// Parse once, then require an exact rendered section in the owned document.
// Skipped/foreign/ambiguous paths are rejected; no verse or relative fallback.
BookDocumentNoteTarget book_document_note_link(const std::string &url, const BookDocument &document);
// Typed legacy image command only; no filesystem reads or viewer launch.
BookImageLink book_image_link(const std::string &url);
// HTML-decoded img src URL, not a showImage query parameter. Percent escapes
// decode once; literal '+' stays '+'. Query/fragment are URL components, not
// filename bytes. Only local relative/file/absolute paths are supported.
BookImageLink book_inline_image_link(const std::string &source);
enum class BookLinkKind { note, image };
BookLinkKind book_link_kind(const std::string &url);
// Scoped single-reference subset of main/url.cc sword_uri + hex_decode.
// Module abbreviations, ranges and fragments remain pending.
PassageLink passage_link(const std::string &url, const std::string &current_module);
std::string passage_uri(const std::string &reference);
}
