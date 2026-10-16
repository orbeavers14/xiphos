// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/heading_note_presentation.hpp"
#include "xiphos/core/link.hpp"
#include <algorithm>
#include <optional>
#include <stdexcept>
#include <vector>
namespace xiphos::core {
namespace {
// Compare exact SDK presentation outside note identifiers. Candidate IDs must
// resolve to this copied heading source, with the same module/reference/type.
// Empty legacy IDs get a parser-only placeholder; none is ever published.
struct NoteTemplate { std::string html; std::vector<std::string> ids; };
std::optional<NoteTemplate> heading_note_template(const std::string &html,
    const HeadingNoteSource &source, std::size_t fragment, bool scoped) {
  NoteTemplate result; std::size_t scan = 0, copied = 0; bool found = false;
  while ((scan = html.find("href=\"", scan)) != std::string::npos) {
    const auto start = scan + 6, end = html.find('"', start);
    if (end == std::string::npos) return std::nullopt;
    const auto uri = html.substr(start, end - start); scan = end + 1;
    if (uri.find("action=showNote") == std::string::npos) continue;
    const auto marker = uri.find("&value=");
    if (marker == std::string::npos) return std::nullopt;
    const auto value = marker + 7, tail = uri.find('&', value);
    if (tail == std::string::npos) return std::nullopt;
    auto parsed_uri = uri;
    if (tail == value) parsed_uri.insert(value, "xiphos-parser-placeholder");
    StudyNoteLink target;
    try { target = study_note_link(parsed_uri, ""); }
    catch (const std::invalid_argument &) { return std::nullopt; }
    if (scoped) {
      HeadingNoteTarget identity;
      try { if (!parse_heading_note_identifier(target.identifier, identity) || identity.fragment != fragment) return std::nullopt; }
      catch (const std::invalid_argument &) { return std::nullopt; }
      result.ids.push_back(identity.local_id);
      const auto note = source.notes.find(target.identifier);
      if (note == source.notes.end() || note->second.identifier != target.identifier || note->second.module != target.module || note->second.reference != target.reference ||
          ((note->second.type == "crossReference") != (target.type == "x"))) return std::nullopt;
    } else if (tail != value && !std::all_of(uri.begin() + value, uri.begin() + tail, [](char c) { return c >= '0' && c <= '9'; })) {
      return std::nullopt; // Preserve literal-counter quirks for nonstandard IDs.
    }
    if (!scoped) result.ids.push_back(tail == value ? std::string() : target.identifier);
    result.html.append(html, copied, start + value - copied); result.html += "xiphos-note-identity";
    copied = start + tail; found = true;
  }
  if (!found) return std::nullopt;
  result.html.append(html, copied, html.size() - copied); return result;
}
}
bool compatible_heading_note_presentation(const HeadingFragment &raw, const HeadingNoteSource &candidate, std::size_t fragment) {
  if (raw.reported_note_label != candidate.reported_note_label) return false;
  const auto original = heading_note_template(raw.html, candidate, fragment, false);
  const auto scoped = heading_note_template(candidate.html, candidate, fragment, true);
  if (!original || !scoped || original->html != scoped->html || original->ids.size() != scoped->ids.size()) return false;
  for (std::size_t i = 0; i < original->ids.size(); ++i)
    if (!original->ids[i].empty() && original->ids[i] != scoped->ids[i]) return false;
  return true;
}
}
