// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/heading_notes.hpp"
#include "xiphos/core/entry_attribute.hpp"
#include <swmodule.h>
#include <swoptfilter.h>
#include <utf8html.h>
#include <utilxml.h>
#include <set>
#include <stdexcept>
#include <utility>
#include <vector>
namespace xiphos { namespace core {
namespace {
// Copy/allocation precedes mutation; restoration swaps without allocating.
class ScopedEntryAttributes {
  sword::SWModule &module_;
  sword::AttributeTypeList saved_;
  bool processing_;
public:
  explicit ScopedEntryAttributes(sword::SWModule &module)
      : module_(module), saved_(module.getEntryAttributes()), processing_(module.isProcessEntryAttributes()) {}
  void refreshed() {
    auto baseline = module_.getEntryAttributes(); saved_.swap(baseline);
  }
  ~ScopedEntryAttributes() {
    module_.getEntryAttributes().swap(saved_);
    // SDK arbitrary-buffer rendering temporarily disables attribute processing;
    // an exception in a filter can bypass its normal restoration.
    module_.setProcessEntryAttributes(processing_);
  }
  ScopedEntryAttributes(const ScopedEntryAttributes &) = delete;
  ScopedEntryAttributes &operator=(const ScopedEntryAttributes &) = delete;
};
std::string checked(const sword::SWBuf &text) {
  std::string result(text.c_str(), text.size());
  if (result.find('\0') != std::string::npos) throw std::invalid_argument("Heading note source contains NUL");
  return result;
}
// Some modules run Footnotes before Headings: their saved heading already has
// SDK IDs and the note bodies live in the source attribute map. Do not filter
// those notes again or replace their bodies with the emptied marker contents.
template<class Visit> void visit_tags(const std::string &raw, Visit visit) {
  for (std::size_t at = 0; (at = raw.find('<', at)) != std::string::npos;) {
    auto end = at + 1; char quote = 0;
    for (; end < raw.size(); ++end) {
      const auto c = raw[end];
      if (quote) { if (c == quote) quote = 0; }
      else if (c == '\'' || c == '"') quote = c;
      else if (c == '>') break;
    }
    if (end == raw.size()) throw std::invalid_argument("Unterminated heading source tag");
    sword::XMLTag tag(raw.substr(at, end + 1 - at).c_str());
    visit(at, end + 1, tag); at = end + 1;
  }
}
std::set<std::string> processed_note_ids(const std::string &raw) {
  std::set<std::string> ids; bool unfiltered = false;
  visit_tags(raw, [&](std::size_t, std::size_t, sword::XMLTag &tag) {
    if (std::string(tag.getName()) == "note" && !tag.isEndTag()) {
      const auto *id = tag.getAttribute("swordFootnote");
      if (id && *id) ids.insert(id); else unfiltered = true;
    }
  });
  if (!ids.empty() && unfiltered) throw std::invalid_argument("Mixed filtered and unfiltered heading notes");
  return ids;
}
std::string scope_note_ids(const std::string &raw, std::size_t fragment, const std::map<std::string, StudyNote> &notes) {
  std::string result; std::size_t copied = 0;
  visit_tags(raw, [&](std::size_t at, std::size_t end, sword::XMLTag &tag) {
    if (std::string(tag.getName()) != "note" || tag.isEndTag()) return;
    const auto *id = tag.getAttribute("swordFootnote");
    if (!id || !notes.count(id)) throw std::invalid_argument("Heading marker has no copied source note");
    const auto scoped = heading_note_identifier(fragment, id);
    tag.setAttribute("swordFootnote", scoped.c_str());
    result.append(raw, copied, at - copied); result += tag.toString(); copied = end;
  });
  result.append(raw, copied, raw.size() - copied);
  return result;
}
}
static HeadingNoteSource capture_notes(sword::SWModule &module,
                                      std::size_t fragment, bool render_bodies, bool scoped) {
  if (!module.isProcessEntryAttributes()) throw std::invalid_argument("Heading-note capture requires entry attribute processing");
  ScopedEntryAttributes scope(module);
  module.renderText(); scope.refreshed();
  if (module.getKey()->getError()) throw std::invalid_argument("SWORD could not resolve the heading-note passage");
  auto &attributes = module.getEntryAttributes();
  const auto heading = attributes.find("Heading");
  if (heading == attributes.end()) throw std::out_of_range("Heading fragment is unavailable");
  const auto preverse = heading->second.find("Preverse");
  if (preverse == heading->second.end()) throw std::out_of_range("Heading fragment is unavailable");
  const auto found = preverse->second.find(std::to_string(fragment).c_str());
  if (found == preverse->second.end() || found->second.size() == 0) throw std::out_of_range("Heading fragment is unavailable");
  HeadingNoteSource result;
  result.reported_note_label = entry_attribute(module, "Footnote", "1", "n", false);
  const auto raw = checked(found->second);
  const auto existing_ids = processed_note_ids(raw);
  const std::string reference = module.getKeyText(), name = module.getName();
  // OSIS starts at 1; ThML otherwise continues the BODY's Footnote/count. Neither
  // identifier belongs to the body namespace after filtering a heading buffer.
  if (existing_ids.empty()) attributes.erase("Footnote");
  sword::SWBuf filtered(raw.c_str());
  // Headings already extracted this buffer. Reapplying it strips ThML's
  // fromEntryAttributes marker, causing its XHTML renderer to hide the heading.
  for (auto *filter : module.getOptionFilters()) {
    const auto *name = filter->getOptionName();
    if (name && (std::string(name) == "Headings" || (!existing_ids.empty() && std::string(name) == "Footnotes"))) continue;
    filter->processText(filtered, module.getKey(), &module);
  }
  auto filtered_text = checked(filtered);
  std::vector<std::pair<std::string, std::string>> bodies;
  const auto group = attributes.find("Footnote");
  if (group != attributes.end()) for (const auto &item : group->second) {
    if (!existing_ids.empty() && !existing_ids.count(item.first.c_str())) continue;
    const auto body = item.second.find("body");
    if (body == item.second.end()) continue; // SDK bookkeeping such as ThML count.
    StudyNote note;
    note.module = name; note.reference = reference; note.identifier = checked(item.first);
    auto value = [&](const char *key) {
      const auto field = item.second.find(key);
      return field == item.second.end() ? std::string() : checked(field->second);
    };
    note.type = value("type"); note.label = value("n"); note.reference_list = value("refList");
    bodies.emplace_back(note.identifier, checked(body->second));
    result.notes.emplace(note.identifier, std::move(note));
  }
  if (!existing_ids.empty() && result.notes.size() != existing_ids.size())
    throw std::invalid_argument("Filtered heading note has no source attributes");
  if (scoped) {
    filtered_text = scope_note_ids(filtered_text, fragment, result.notes);
    std::map<std::string, StudyNote> renamed;
    for (auto &item : result.notes) {
      const auto id = heading_note_identifier(fragment, item.first);
      item.second.identifier = id; renamed.emplace(id, std::move(item.second));
    }
    result.notes.swap(renamed);
    for (auto &body : bodies) body.first = heading_note_identifier(fragment, body.first);
  }
  sword::SWBuf encoded(filtered_text.c_str()); sword::UTF8HTML conversion; conversion.processText(encoded);
  result.html = checked(module.renderText(encoded.c_str()));
  if (render_bodies) for (const auto &body : bodies) {
    sword::SWBuf text(body.second.c_str()); conversion.processText(text);
    result.notes.at(body.first).html = checked(module.renderText(text.c_str()));
  }
  return result;
}
HeadingNoteSource capture_preverse_heading_notes(sword::SWModule &module,
                                                 std::size_t fragment, bool render_bodies) {
  return capture_notes(module, fragment, render_bodies, false);
}
HeadingNoteSource capture_scoped_preverse_heading_notes(sword::SWModule &module,
                                                        std::size_t fragment, bool render_bodies) {
  return capture_notes(module, fragment, render_bodies, true);
}

}}
