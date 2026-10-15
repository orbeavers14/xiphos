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
#include <swbuf.h>
#include <glib.h>
#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstring>
#include <stdexcept>
#include <vector>
namespace xiphos { namespace core {
namespace {
using sword::SWBuf;
using std::max;
#define XI_warning(args) (supported = false)
#define ALIGN_WORD "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"

void
block_dump(SWBuf &rendered,
	   const char **word,
	   const char **strongs,
	   const char **morph)
{
	int wlen, min_length, slen, mlen;
	char *s, *s0, *t;

	// unannotated words need no help.
	if (*word && (*strongs == NULL) && (*morph == NULL)) {
		rendered += "<span class=\"word\">";
		rendered += *word;
		rendered += "</span>";
		g_free((char *)*word);
		*word = NULL;
		rendered += " ";
		return;
	}

	if (*word == NULL) {
		// we need to cobble up something to take the place of
		// a word, in order that the strongs/morph not overlay.
		*word = g_strdup(ALIGN_WORD);
	}

	rendered += "<span class=\"word\">";
	if (*strongs) {
		s = g_strrstr(*strongs, "</a>");
		*s = '\0';
		t = (char *)strrchr(*strongs, '>') + 1;
		// correct weird NASB lexicon references.
		if ((s0 = (char *)strchr(*strongs, '!'))) {
			do {
				*s0 = *(s0 + 1);
				++s0;
			} while (*s0 != '"');
			*s0 = ' ';
		}
		*s = '<';
		slen = s - t;
		s = (char *)strstr(*strongs, "&lt;");
		*s = *(s + 1) = *(s + 2) = *(s + 3) = ' ';
		s = strstr(s, "&gt;");
		*s = *(s + 1) = *(s + 2) = *(s + 3) = ' ';

		// gross hack needed to handle new class="..." in sword -r2512.
		if ((s = (char *)strstr(*strongs, " class=\"strongs\">"))) {
			memcpy(s, ">                ", 17);
			if ((s = (char *)strstr(s, " class=\"strongs\">")))
				memcpy(s, ">                ", 17);
		}
	} else
		slen = 0;

	if (*morph) {
		s = s0 = (char *)g_strrstr(*morph, "\">") + 2;
		t = strchr(s, '<');
		for (/* */; s < t; ++s)
			if (isupper(*s))
				*s = tolower(*s);
		for (s = strchr(s0, ' '); s && (s < t); s = strchr(s, ' '))
			*s = '-';
		s = g_strrstr(*morph, "</a>");
		*s = '\0';
		t = (char *)strrchr(*morph, '>') + 1;
		*s = '<';
		mlen = s - t;
		s = (char *)strchr(*morph, '(');
		*s = ' ';
		s = strrchr(s, ')');
		*s = ' ';

		// gross hack needed to handle new class="..." in sword -r2512.
		if ((s = (char *)strstr(*morph, " class=\"morph\">"))) {
			memcpy(s, ">              ", 15);
			if ((s = (char *)strstr(s, " class=\"morph\">")))
				memcpy(s, ">              ", 15);
		}
	} else
		mlen = 0;

	min_length = 2 + max(slen, mlen);

	rendered += *word;

	for (wlen = strlen(*word); wlen <= min_length; ++wlen)
		rendered += "&nbsp;";

	g_free((char *)*word);
	*word = NULL;

	rendered += "<span class=\"strongs\"><sup>";
	rendered += (*strongs ? *strongs : "&nbsp;");
	rendered += "</sup></span>";
	if (*strongs)
		g_free((char *)*strongs);
	*strongs = NULL;

/* truncate long Hebrew morph codes: remove pronominal suffix segment */
	if (*morph) {
		char *anchor_start = (char *)g_strrstr(*morph, "\">") + 2;
		char *anchor_end = (char *)strstr(anchor_start, "</a>");
		if (anchor_start && anchor_end) {
			/* count slashes */
			char *first_slash = strchr(anchor_start, '/');
			if (first_slash && first_slash < anchor_end) {
				char *second_slash = strchr(first_slash + 1, '/');
				if (second_slash && second_slash < anchor_end) {
					/* 2 segments: hr/ncmsc/sp2mp → truncate at second slash */
					memmove(second_slash, anchor_end,
						strlen(anchor_end) + 1);
				} else {
					/* 1 segment: check if suffix is pronominal (sp/sd) */
					char *seg = first_slash + 1;
					if ((seg[0] == 's' && (seg[1] == 'p' || seg[1] == 'd'))
					    || (seg[0] == 'S' && (seg[1] == 'p' || seg[1] == 'd'))) {
						memmove(first_slash, anchor_end,
							strlen(anchor_end) + 1);
					}
				}
			}
		}
	}

	rendered += "<span class=\"morph\"><sup>";
	rendered += (*morph ? *morph : "&nbsp;");
	rendered += "</sup></span>";
	if (*morph)
		g_free((char *)*morph);
	*morph = NULL;

	rendered += "</span> <span class=\"word\">&nbsp;</span>";
}

//
// re-process a block of text so as to envelope its strong's and morph
// references in <span> blocks which will be interpreted by the CSS
// directives to put each ref immediately below the word it annotates.
// this keeps the text linearly readable while providing annotation.
//
// secondary interface.
// text destination is provided, ready to go.
// this means we are able to recurse when needed.

#define EMPTY_WORD ""

void
block_render_secondary(const char *text,
		       SWBuf &rendered, bool &supported)
{
	const char *word = NULL,
		   *strongs = NULL,
		   *morph = NULL;
	int bracket;
	const char *s;
	char *t, *u;

	for (s = text; *s; ++s) {
		switch (*s) {
		case ' ':
		case '\t':
			break;

		case '<':
			// <token> causes a lot of grief, because we must
			// keep it bound with its </token> terminator,
			// esp. because anchors contain <small></small>. *sigh*
			// i believe we see only anchors + font switches here.
			if ((((*(s + 1) == 'a') || (*(s + 1) == 'A')) && (*(s + 2) == ' ')) ||
			    (((*(s + 1) == 'b') || (*(s + 1) == 'B') ||
			      (*(s + 1) == 'i') || (*(s + 1) == 'I') ||
			      (*(s + 1) == 'u') || (*(s + 1) == 'U')) &&
			     (*(s + 2) == '>'))) {
				if (word)
					block_dump(rendered, &word, &strongs, &morph);

				char end[5] = "</X>";
				end[2] = *(s + 1);
			again:
				if ((t = strstr((char *)s, end)) == NULL) {
					XI_warning(("No %s in %s\n", end, s));
					break;
				}

				// yet another nightmare:
				// if the markup results in e.g. doubled <i>
				// (bogus "<hi><hi>word</hi></hi>"),
				// then we will mis-assess termination.
				// so we search for the same markup embedded within.
				// if we find an internal set, we just wipe it out.
				char embedded[4] = "<X>";
				embedded[1] = *(s + 1);
				if ((u = g_strstr_len(s + 3, t - (s + 3), embedded))) {
					*u = *(u + 1) = *(u + 2) =
					    *t = *(t + 1) = *(t + 2) = *(t + 3) = ' ';
					goto again; // yuck, yes, i know...
				}

				// nasb eph 5:31: whole verse is an italicized
				// quotation of gen 2:24...containing strongs.
				// in event of font switch, output bare switch
				// controls but recurse on content within.
				if ((*(s + 1) == 'a') || (*(s + 1) == 'A')) {
					// <a href> anchor -- uninteresting.
					t += 4;
					word = g_strndup(s, t - s);
					s = t - 1;
				} else {
					// font switch nightmare.
					word = g_strndup(s, 3);
					rendered += word;
					g_free((char *)word);
					word = g_strndup(s + 3, t - (s + 3));
					block_render_secondary(word, rendered, supported);
					g_free((char *)word);
					word = NULL;
					rendered += end;
					s = t + 3;
				}
				break;
			} else if (!strncmp(s + 1, "small>", 6)) {
				// strongs and morph are bounded by "<small>".
				if ((t = strstr((char *)s, "</small>")) == NULL) {
					XI_warning(("No </small> in %s\n", s));
					break;
				}
				t += 8;
				// this is very dicey -- phenomenological/
				// observable about Sword filters' provision.
				// strongs: "<em>&lt;...&gt;</em>"
				// morph:   "<em>(...)</em>"
				// if Sword ever changes this, we're dead.
				const char *u = s + 11;
				while ((*u != '(') && (*u != '&'))
					++u; // it has to be one or the other.
				if (*u == '(') {
					if (morph) {
						block_dump(rendered, &word, &strongs, &morph);
						word = g_strdup(EMPTY_WORD);
					}
					morph = g_strndup(s, t - s);
				} else {
					if (strongs) {
						block_dump(rendered, &word, &strongs, &morph);
						word = g_strdup(EMPTY_WORD);
					}
					strongs = g_strndup(s, t - s);
				}
				s = t - 1;
				break;
			}
		// ...fall through to ordinary text...
		// (includes other "<>"-bounded markup.)

		default:
			if (word)
				block_dump(rendered, &word, &strongs, &morph);

			// here's an unfortunate problem.  consider:
			// L<font size="-1">ORD</font> followed by strongs.
			// the SPC breaks it into 2 "words", very badly.
			// we need to track <> use to get it as *1* word,
			// before we capture the strongs, or just the latter
			// half of it ("size=...") goes inside the <span>.
			bracket = 0;
			word = s;
			do {
				while ((*s == ' ') || (*s == '\t'))
					s++;
				for (/* */;
				     *s && (*s != ' ') && (*s != '\t');
				     ++s) {
					if (*s == '<') {
						if (!strncmp(s + 1, "small>", 6) ||
						    ((*(s + 1) == 'a') &&
						     (*(s + 2) == ' '))) {
							// "break 2;"
							goto outword;
						}
						bracket++;
					} else if (*s == '>')
						bracket--;
					assert(bracket >= 0);
				}
			} while (bracket != 0);
		outword:
			word = g_strndup(word, s - word);
			s--;
		}
	}
	if (word || strongs || morph)
		block_dump(rendered, &word, &strongs, &morph);
}


#undef ALIGN_WORD
#undef EMPTY_WORD
#undef XI_warning
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
  std::vector<char> mutable_text(html.begin(), html.end()); mutable_text.resize(mutable_text.size() + 8, '\0');
  SWBuf rendered; bool supported = true;
  block_render_secondary(mutable_text.data(), rendered, supported);
  result.html = supported ? std::string(rendered.c_str()) : std::move(html);
  result.applied = supported;
  return result;
}
}}
