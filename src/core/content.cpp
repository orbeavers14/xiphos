// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/content.hpp"
#include <stdexcept>
#include "xiphos/core/link.hpp"
#include "xiphos/core/reference_list.hpp"
namespace xiphos::core {
namespace {
std::string attribute(const std::string &value) {
  std::string result;
  for (const auto c : value) {
    switch (c) {
    case '&': result += "&amp;"; break;
    case '<': result += "&lt;"; break;
    case '>': result += "&gt;"; break;
    case '"': result += "&quot;"; break;
    case '\'': result += "&#39;"; break;
    default: result += c;
    }
  }
  return result;
}
}
std::string display_reference_list(const ReferenceList &references) {
  std::string html;
  for (std::size_t i = 0; i < references.entries.size(); ++i) {
    const auto &entry = references.entries[i];
    html += "<section id=\"reference-" + std::to_string(i + 1) + "\" class=\"xc-reference\" aria-label=\"" + attribute(entry.reference) + "\">";
    html += "<header><a href=\"" + attribute(passage_uri(entry.reference)) + "\">" + attribute(entry.reference) + "</a></header>";
    html += entry.html + "</section>\n";
  }
  return html;
}
std::string display_reference_links(const ParsedReferenceList &references) {
  std::string html;
  for (const auto &reference : references.references)
    html += "<a href=\"" + attribute(passage_uri(reference)) + "\">" +
        attribute(reference) + ",</a><br/>";
  html += "<a href=\"" + attribute(passage_uri(references.context)) + "\">Back to " +
      attribute(references.context) + "</a><br/>";
  return html;
}
std::string display_chapter(const Chapter &chapter, const DisplayOptions &options) {
  std::string html;
  for (std::size_t i = 0; i < chapter.verses.size(); ++i) {
    const auto selected = i + 1 == chapter.selected_verse;
    html += "<section id=\"verse-" + std::to_string(i + 1) + "\" class=\"xc-verse";
    if (selected && options.highlight_selected) html += " xc-current";
    html += "\" aria-label=\"" + attribute(chapter.verses[i].reference) + "\"";
    if (selected) html += " aria-current=\"location\"";
    html += ">";
    if (options.verse_numbers) html += "<sup><a href=\"" + attribute(passage_uri(chapter.verses[i].reference)) + "\">" + std::to_string(i + 1) + "</a></sup> ";
    html += chapter.verses[i].html + "</section>\n";
  }
  return html;
}
std::string display_commentary_chapter(const Chapter &chapter, const std::vector<unsigned> &positions,
                                       const DisplayOptions &options) {
  if (positions.size() != chapter.verses.size()) throw std::invalid_argument("Commentary positions do not match entries");
  std::string html = options.verse_numbers ? "<table class=\"xc-commentary\"><tbody>" : "<div class=\"xc-commentary\">";
  bool before = true;
  const auto selected = chapter.selected_verse;
  auto missing_anchor = [&] {
    return "<a id=\"" + attribute(chapter.selected_anchor) + "\"></a>";
  };
  for (size_t i = 0; i < positions.size(); ++i) {
    const auto verse = positions[i];
    if (selected != 1 && (selected == verse || (before && verse > selected))) {
      const auto anchor = before && verse > selected ? missing_anchor() : "";
      html += options.verse_numbers ? "<tr><td>" + anchor + "<hr/></td><td><hr/></td></tr>" : anchor + "<hr/>";
    }
    const auto &entry = chapter.verses[i];
    const auto anchor = "commentary-verse-" + std::to_string(verse);
    const auto link = "<a href=\"" + attribute(passage_uri(entry.reference)) + "\">" + std::to_string(verse) + "</a>";
    const auto current = verse == selected;
    const auto attributes = " id=\"" + anchor + "\" class=\"xc-commentary-section" +
        (current && options.highlight_selected ? " xc-current" : "") + "\" aria-label=\"" + attribute(entry.reference) +
        "\"" + (current ? " aria-current=\"location\"" : "");
    if (options.verse_numbers) html += "<tr" + attributes + "><td valign=\"top\">" + link + "</td><td>" + entry.html + "</td></tr>";
    else html += "<section" + attributes + ">" + entry.html + "</section>";
    before = verse < selected;
  }
  if (before) {
    html += options.verse_numbers ? "<tr><td>" + missing_anchor() + "<hr/></td><td><hr/></td></tr>" : missing_anchor() + "<hr/>";
  }
  html += options.verse_numbers ? "</tbody></table>" : "</div>";
  return html;
}

}
