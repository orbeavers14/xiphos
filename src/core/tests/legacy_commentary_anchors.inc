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
// Frozen selected/missing anchor statements from CrossWire c9976a8.
std::string frozen_commentary_anchors(const std::vector<unsigned> &positions, unsigned selected) {
  sword::SWBuf swbuf; gchar *buf = nullptr;
  int curVerse = static_cast<int>(selected); bool before_curVerse = true;
  struct Key { int verse; int getVerse() const { return verse; } } value;
  auto *key = &value;
  for (auto verse : positions) {
    value.verse = static_cast<int>(verse);
		if ((curVerse != 1) && // not at top of pane
		    ((curVerse == key->getVerse()) ||
		     (before_curVerse && (key->getVerse() > curVerse)))) {
			buf = NULL;
			swbuf.appendFormatted("<tr><td>%s<hr/></td><td><hr/></td></tr>",
					      // repeated conditional check here
					      ((before_curVerse &&
						(key->getVerse() > curVerse))
					       ? (buf = g_strdup_printf(
							  "<a name=\"%d\"> </a>", curVerse))
					       : ""));
			g_free(buf);
		}

    swbuf.appendFormatted("<entry verse=\"%d\"><a name=\"%d\"> </a></entry>", key->getVerse(), key->getVerse());
    before_curVerse = key->getVerse() < curVerse;
  }
	if (before_curVerse) {
		swbuf.appendFormatted("<tr><td><a name=\"%d\"> </a><hr/></td><td><hr/></td></tr>",
				      curVerse);
	}

  return swbuf.c_str();
}
