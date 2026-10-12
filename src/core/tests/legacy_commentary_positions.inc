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

// Frozen iterator statements from GTKEntryDisp::displayByChapter, CrossWire
// c9976a8. Rendering/cache/presentation body is deliberately not characterized.
std::vector<std::string> frozen_commentary_positions(sword::SWModule &imodule) {
  using sword::VerseKey; using sword::SWKey;
  imodule.setSkipConsecutiveLinks(true);
  VerseKey *key = (VerseKey *)(SWKey *) imodule;
  int curChapter = key->getChapter();
  int curBook = key->getBook();
  std::vector<std::string> result;
  for (key->setVerse(1);
       (key->getBook() == curBook) && (key->getChapter() == curChapter) && !imodule.popError();
       imodule++) {
    result.push_back(key->getText());
  }
  return result;
}
