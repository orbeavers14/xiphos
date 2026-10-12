// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/content.hpp"
#include "xiphos/core/link.hpp"
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
std::string display_reference_links(const ReferenceList &references) {
  std::string html;
  for (const auto &entry : references.entries)
    html += "<a href=\"" + attribute(passage_uri(entry.reference)) + "\">" +
        attribute(entry.reference) + ",</a><br/>";
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
}
