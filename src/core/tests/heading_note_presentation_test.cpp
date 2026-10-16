// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/heading_note_presentation.hpp"
#include <iostream>
#include <stdexcept>
using namespace xiphos::core;
void check(bool ok, const char *message) { if (!ok) throw std::runtime_error(message); }
std::string anchor(const std::string &id, const std::string &type = "n") {
  return "<a class=\"noteMarker study\" href=\"passagestudy.jsp?action=showNote&type=" + type + "&value=" + id +
      "&module=Notes&passage=John+4%3A1\"><small><sup>*na</sup></small></a>";
}
int main() {
  try {
    HeadingFragment raw; raw.html = "<h3>Original " + anchor("") + "</h3>"; raw.reported_note_label = "body";
    HeadingNoteSource source; source.reported_note_label = raw.reported_note_label;
    const auto id = heading_note_identifier(0, "1");
    source.html = "<h3>Original " + anchor(id) + "</h3>";
    StudyNote note; note.identifier = id; note.module = "Notes"; note.reference = "John 4:1"; note.type = "study";
    source.notes.emplace(id, note);
    check(compatible_heading_note_presentation(raw, source, 0), "empty legacy URI IDs can be replaced by exact scoped source targets");
    auto numbered = raw; numbered.html = "<h3>Original " + anchor("1") + "</h3>";
    check(compatible_heading_note_presentation(numbered, source, 0), "known original SDK identity matches the scoped local ID");
    for (const auto *other : {"2", "01", "*n", "999"}) {
      auto changed = raw; changed.html = "<h3>Original " + anchor(other) + "</h3>";
      check(!compatible_heading_note_presentation(changed, source, 0), "known/malformed original IDs cannot be retargeted by matching visible labels");
    }
    check(!compatible_heading_note_presentation(raw, source, 1), "candidate cannot move to a different heading fragment");
    for (int field = 0; field < 8; ++field) {
      auto changed = source;
      if (field == 0) changed.html.replace(changed.html.find("Original"), 8, "Changed");
      if (field == 1) changed.reported_note_label = "different";
      if (field == 2) changed.notes.at(id).module = "Other";
      if (field == 3) changed.notes.at(id).reference = "John 4:2";
      if (field == 4) changed.notes.at(id).type = "crossReference";
      if (field == 5) changed.notes.clear();
      if (field == 6) changed.html += " extra";
      if (field == 7) changed.notes.at(id).identifier = "different";
      check(!compatible_heading_note_presentation(raw, changed, 0), "changed text, suppression, module, reference, type, ownership or structure rejects");
    }
    for (const auto &html : {std::string("<h3>Original</h3>"), "<h3>Original " + anchor("xiphos-heading-01-31") + "</h3>",
         std::string("<a href=\"unfinished"), "<h3>Original " + anchor("") + "</h3>"}) {
      auto changed = source; changed.html = html;
      check(!compatible_heading_note_presentation(raw, changed, 0), "missing, noncanonical or malformed candidate links remain unsupported");
    }
    auto duplicate = source; duplicate.html.insert(duplicate.html.find("&module="), "&value=other");
    check(!compatible_heading_note_presentation(raw, duplicate, 0), "duplicate query parameters cannot be normalized into valid links");
    auto cross_raw = raw; cross_raw.html = "<h3>Original " + anchor("", "x") + "</h3>";
    auto cross = source; cross.html = "<h3>Original " + anchor(id, "x") + "</h3>"; cross.notes.at(id).type = "crossReference";
    check(compatible_heading_note_presentation(cross_raw, cross, 0), "cross-reference type is validated against copied metadata");
    const auto second = heading_note_identifier(0, "2");
    auto multiple = source; multiple.notes.emplace(second, note); multiple.notes.at(second).identifier = second;
    multiple.html = source.html + anchor(second);
    auto multiple_raw = raw; multiple_raw.html += anchor("");
    check(compatible_heading_note_presentation(multiple_raw, multiple, 0), "multiple original markers preserve exact structure and source targets");
    multiple_raw.html = numbered.html + anchor("2");
    check(compatible_heading_note_presentation(multiple_raw, multiple, 0), "multiple already-filtered markers preserve their own SDK IDs");
    multiple_raw.html = numbered.html + anchor("1");
    check(!compatible_heading_note_presentation(multiple_raw, multiple, 0), "repeated known ID cannot silently change identity");
    std::cout << "Heading presentation gate preserves exact HTML, local identity, labels and target metadata\n";
  } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
