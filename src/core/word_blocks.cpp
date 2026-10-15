// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Xiphos Bible Study Tool
 * display.cc -
 *
 * Copyright (C) 2000-2026 Xiphos Developer Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 */
#include "xiphos/core/word_blocks.hpp"
#include "xiphos/core/lexical_style.hpp"
#include <swbuf.h>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <stdexcept>
#include <utility>
#include <vector>
namespace xiphos { namespace core {
namespace {
using sword::SWBuf;
const char *const alignment_word = "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;";

// Keep legacy spacing and displayed-code rules, but edit owned strings using
// checked offsets instead of mutating buffers through unchecked pointer searches.
bool format_annotation(std::string &html, bool strong, std::size_t &width) {
  const auto close = html.rfind("</a>");
  if (close == std::string::npos || close == 0) return false;
  const auto open = html.rfind('>', close - 1);
  if (open == std::string::npos || open >= close) return false;
  width = close - open - 1; // Legacy layout deliberately measures UTF-8 bytes.

  if (strong) {
    const auto bang = html.find('!');
    if (bang != std::string::npos && bang < close) {
      const auto quote = html.find('"', bang);
      if (quote == std::string::npos || quote >= close) return false;
      // NASB correction removes ! while retaining byte length and href spelling.
      std::rotate(html.begin() + bang, html.begin() + bang + 1, html.begin() + quote + 1);
      html[quote] = ' ';
    }
    const auto left = html.find("&lt;"), right = html.find("&gt;");
    if (left == std::string::npos || right == std::string::npos) return false;
    html.replace(left, 4, 4, ' ');
    html.replace(right, 4, 4, ' ');
  } else {
    const auto label = html.rfind("\">");
    if (label == std::string::npos) return false;
    const auto end = html.find('<', label + 2);
    if (end == std::string::npos) return false;
    for (auto i = label + 2; i < end; ++i) {
      const auto byte = static_cast<unsigned char>(html[i]);
      if (std::isupper(byte)) html[i] = static_cast<char>(std::tolower(byte));
      if (html[i] == ' ') html[i] = '-';
    }
    const auto left = html.find('('), right = html.rfind(')');
    if (left == std::string::npos || right == std::string::npos) return false;
    html[left] = html[right] = ' ';
  }

  const std::string marker = strong ? " class=\"strongs\">" : " class=\"morph\">";
  std::size_t from = 0;
  for (unsigned n = 0; n < 2; ++n) {
    const auto found = html.find(marker, from);
    if (found == std::string::npos) break;
    html.replace(found, marker.size(), ">" + std::string(marker.size() - 1, ' '));
    from = found;
  }

  if (!strong) {
    const auto label = html.rfind("\">");
    if (label == std::string::npos) return false;
    const auto end = html.find("</a>", label + 2);
    if (end == std::string::npos) return false;
    const auto slash = html.find('/', label + 2);
    if (slash != std::string::npos && slash < end) {
      const auto second = html.find('/', slash + 1);
      if (second != std::string::npos && second < end) html.erase(second, end - second);
      else if (html.compare(slash + 1, 2, "sp") == 0 || html.compare(slash + 1, 2, "sd") == 0 ||
               html.compare(slash + 1, 2, "Sp") == 0 || html.compare(slash + 1, 2, "Sd") == 0)
        html.erase(slash, end - slash);
    }
  }
  return true;
}

struct WordParts {
  std::string word, strongs, morph;
  bool has_word = false; // Empty placeholder words differ from absent words.
  bool any() const { return has_word || !strongs.empty() || !morph.empty(); }
};
void block_dump(SWBuf &rendered, WordParts &parts, bool &supported) {
  WordParts owned = std::move(parts);
  parts = WordParts{};
  const std::string word_html = owned.has_word ? owned.word : alignment_word;
  if (owned.has_word && owned.strongs.empty() && owned.morph.empty()) {
    rendered += "<span class=\"word\">";
    rendered += word_html.c_str();
    rendered += "</span> ";
    return;
  }
  std::size_t strong_width = 0, morph_width = 0;
  if ((!owned.strongs.empty() && !format_annotation(owned.strongs, true, strong_width)) ||
      (!owned.morph.empty() && !format_annotation(owned.morph, false, morph_width))) {
    supported = false;
    return;
  }
  rendered += "<span class=\"word\">";
  rendered += word_html.c_str();
  for (auto width = word_html.size(); width <= 2 + std::max(strong_width, morph_width); ++width)
    rendered += "&nbsp;";
  rendered += "<span class=\"strongs\"><sup>";
  rendered += owned.strongs.empty() ? "&nbsp;" : owned.strongs.c_str();
  rendered += "</sup></span><span class=\"morph\"><sup>";
  rendered += owned.morph.empty() ? "&nbsp;" : owned.morph.c_str();
  rendered += "</sup></span></span> <span class=\"word\">&nbsp;</span>";
}

// All scanner state and recursive working text are owned values. The bounded
// grammar guard runs first; no caller buffer or global output is mutated.
void block_render_secondary(std::string text, SWBuf &rendered, bool &supported) {
  text.append(8, '\0'); // Retain bounded lookahead room for the legacy grammar.
  WordParts parts;
  for (char *s = &text[0]; *s; ++s) {
    switch (*s) {
    case ' ':
    case '\t':
      break;
    case '<':
      if (((s[1] == 'a' || s[1] == 'A') && s[2] == ' ') ||
          ((s[1] == 'b' || s[1] == 'B' || s[1] == 'i' || s[1] == 'I' ||
            s[1] == 'u' || s[1] == 'U') && s[2] == '>')) {
        if (parts.has_word) block_dump(rendered, parts, supported);
        char end[] = "</X>";
        end[2] = s[1];
        char *close;
        for (;;) {
          close = std::strstr(s, end);
          if (!close) { supported = false; return; }
          char embedded[] = "<X>";
          embedded[1] = s[1];
          char *nested = std::search(s + 3, close, embedded, embedded + 3);
          if (nested == close) break;
          // Preserve legacy duplicate-style flattening on this private copy.
          std::fill_n(nested, 3, ' ');
          std::fill_n(close, 4, ' ');
        }
        if (s[1] == 'a' || s[1] == 'A') {
          close += 4;
          parts.word.assign(s, close - s);
          parts.has_word = true;
          s = close - 1;
        } else {
          rendered += std::string(s, 3).c_str();
          block_render_secondary(std::string(s + 3, close - (s + 3)), rendered, supported);
          rendered += end;
          s = close + 3;
        }
        break;
      } else if (!std::strncmp(s + 1, "small>", 6)) {
        char *close = std::strstr(s, "</small>");
        if (!close) { supported = false; return; }
        close += 8;
        const char *marker = s + 11;
        while (marker < close && *marker != '(' && *marker != '&') ++marker;
        if (marker == close) { supported = false; return; }
        std::string &annotation = *marker == '(' ? parts.morph : parts.strongs;
        if (!annotation.empty()) {
          block_dump(rendered, parts, supported);
          parts.has_word = true;
        }
        // block_dump resets the values, but not the identity of their members.
        annotation.assign(s, close - s);
        s = close - 1;
        break;
      }
      // Other tags stay attached to the ordinary word as in the legacy parser.
      // Fall through.
    default:
      if (parts.has_word) block_dump(rendered, parts, supported);
      {
        const char *start = s;
        int bracket = 0;
        bool annotation = false;
        do {
          while (*s == ' ' || *s == '\t') ++s;
          for (; *s && *s != ' ' && *s != '\t'; ++s) {
            if (*s == '<') {
              if (!std::strncmp(s + 1, "small>", 6) || (s[1] == 'a' && s[2] == ' ')) {
                annotation = true;
                break;
              }
              ++bracket;
            } else if (*s == '>') --bracket;
            if (bracket < 0) { supported = false; return; }
          }
        } while (!annotation && bracket != 0);
        parts.word.assign(start, s - start);
        parts.has_word = true;
        --s;
      }
    }
  }
  if (parts.any()) block_dump(rendered, parts, supported);
}

bool starts(const std::string &text, std::size_t at, const std::string &prefix) {
  return at <= text.size() && text.compare(at, prefix.size(), prefix) == 0;
}
bool small_supported(const std::string &text) {
  if (!starts(text, 0, "<small><em")) return false;
  const auto em_end = text.find('>', 7);
  if (em_end == std::string::npos) return false;
  const auto marker = em_end + 1;
  const bool strong = starts(text, marker, "&lt;");
  if (!strong && !starts(text, marker, "(")) return false;
  // The inherited scanner starts at byte 11 and must reach this marker before
  // any marker-like data in the em attributes.
  if (marker < 11 || text.substr(11, marker - 11).find_first_of("(&") != std::string::npos) return false;
  const auto prefix_size = marker + (strong ? 4 : 1);
  const std::string suffix = strong ? "&gt;</em></small>" : ")</em></small>";
  if (text.size() < prefix_size + suffix.size() || !starts(text, text.size() - suffix.size(), suffix)) return false;
  const auto content = text.substr(prefix_size, text.size() - prefix_size - suffix.size());
  std::size_t at = 0; unsigned anchors = 0;
  while (at < content.size()) {
    if (content[at] == ' ' || content[at] == '\t' || content[at] == ',') { ++at; continue; }
    // The inherited annotation formatter searches for lowercase </a> and
    // dereferences the match. Uppercase anchors are fine as ordinary content,
    // but annotations must fall back until that formatter supports them.
    if (!starts(content, at, "<a ")) return false;
    const auto open = content.find('>', at), close = content.find("</a>", open);
    if (open == std::string::npos || close == std::string::npos || open == 0 || content[open - 1] != '"') return false;
    const auto tag = content.substr(at, open - at);
    if (tag.find('\'') != std::string::npos) return false;
    for (auto i = open + 1; i < close; ++i) {
      const auto c = static_cast<unsigned char>(content[i]);
      if ((!strong && c >= 128) || c == '<' || c == '>' || c == '"' || c == '&' || c == '!') return false;
    }
    if (++anchors > 8 || (!strong && anchors > 1)) return false;
    at = close + 4;
  }
  return anchors != 0;
}
bool supported_markup(const std::string &html) {
  if (html.size() > 1024 * 1024) return false;
  std::vector<std::string> stack;
  std::size_t opaque_anchor_depth = 0;
  for (std::size_t at = 0; at < html.size();) {
    if (html[at] == '>') return false;
    if (html[at] != '<') { ++at; continue; }
    // Ordinary lowercase anchors are copied intact by the scanner. A SWORD
    // footnote's nested small/sup marker is content, not a lexical annotation.
    if (starts(html, at, "<small>") && !opaque_anchor_depth) {
      const auto end = html.find("</small>", at);
      if (end == std::string::npos || !small_supported(html.substr(at, end + 8 - at))) return false;
    }
    auto name_start = at + 1;
    const bool closing = name_start < html.size() && html[name_start] == '/';
    if (closing) ++name_start;
    auto name_end = name_start;
    while (name_end < html.size()) {
      const auto c = html[name_end];
      if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_')) break;
      ++name_end;
    }
    if (name_end == name_start) return false;
    const auto name = html.substr(name_start, name_end - name_start);
    auto end = name_end; char quote = 0;
    for (; end < html.size(); ++end) {
      const auto c = html[end];
      if (c == '<' || (quote && c == '>')) return false;
      if (quote) { if (c == quote) quote = 0; }
      else if (c == '\'' || c == '"') quote = c;
      else if (c == '>') break;
    }
    if (end == html.size() || quote) return false;
    auto lower = name;
    for (auto &c : lower) if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
    const bool empty = html[end - 1] == '/' || lower == "img" || lower == "br" || lower == "hr" || lower == "meta" || lower == "link" || lower == "input";
    if (closing) {
      if (stack.empty() || stack.back() != name || end != name_end) return false;
      if (opaque_anchor_depth == stack.size()) opaque_anchor_depth = 0;
      stack.pop_back();
    } else if (!empty) {
      if (stack.size() >= 64 || (lower == "a" &&
          (std::find(stack.begin(), stack.end(), "a") != stack.end() ||
           std::find(stack.begin(), stack.end(), "A") != stack.end()))) return false;
      if (name == "a" && starts(html, at, "<a ")) opaque_anchor_depth = stack.size() + 1;
      stack.push_back(name);
    }
    at = end + 1;
  }
  return stack.empty();
}
}
WordBlockResult render_word_blocks(std::string html) {
  if (html.find('\0') != std::string::npos) throw std::invalid_argument("Word block HTML contains NUL");
  WordBlockResult result;
  if (!supported_markup(html)) { result.html = std::move(html); return result; }
  SWBuf rendered; bool supported = true;
  block_render_secondary(html, rendered, supported);
  result.html = supported ? std::string(rendered.c_str()) : std::move(html);
  result.applied = supported;
  return result;
}
LexicalLayout layout_lexical_content(std::string html, bool strongs, bool lemmas,
                                     bool morphology, bool double_space) {
  if (html.find('\0') != std::string::npos) throw std::invalid_argument("Lexical layout HTML contains NUL");
  LexicalLayout result;
  result.requested = strongs || lemmas || morphology;
  if (result.requested) {
    auto rendered = render_word_blocks(std::move(html));
    result.html = std::move(rendered.html);
    result.applied = rendered.applied;
  } else result.html = std::move(html);
  result.css = result.applied ? lexical_block_style(strongs, lemmas, morphology, double_space)
                             : lexical_block_style(false, false, false, double_space);
  return result;
}
}}
