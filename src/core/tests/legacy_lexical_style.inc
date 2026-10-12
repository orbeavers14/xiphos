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

// Frozen CSS declarations and selection contract, CrossWire c9976a8.
#define CSS_BLOCK_BOTH                                                                            \
	" *        { line-height: 3.5em; }"                                                       \
	" .word    { position: relative; top:  0.0em; left: 0; }"                                 \
	" .strongs { position: absolute; top:  0.4em; left: 0; white-space: nowrap; z-index: 2 }" \
	" .morph   { position: absolute; top:  0.9em; left: 0; white-space: nowrap; z-index: 1 }" \
	" *[dir=rtl] .word { padding-left: 0.8em; }"                               \
	" *[dir=rtl] .strongs { top: 0.4em; left: auto; right: 0; }"                             \
	" *[dir=rtl] .morph { left: auto; right: 0; }"
#define CSS_BLOCK_ONE                                                                  \
	" *        { line-height: 2.7em; }"                                            \
	" .word    { position: relative; top:  0.0em; left: 0; }"                      \
	" .strongs { position: absolute; top:  0.4em; left: 0; white-space: nowrap; }" \
	" .morph   { position: absolute; top:  0.4em; left: 0; white-space: nowrap; }" \
	" *[dir=rtl] .word { padding-left: 0.8em; }"                               \
	" *[dir=rtl] .strongs { left: auto; right: 0; }"                               \
	" *[dir=rtl] .morph { left: auto; right: 0; }"

#define DOUBLE_SPACE " * { line-height: 2em ! important; }"
const char *frozen_lexical_style(int strongs, int lemmas, int morphs, int doublespace) {
  const bool strongs_and_morph = ((strongs || lemmas) && morphs);
  const bool strongs_or_morph = ((strongs || lemmas) || morphs);
  return strongs_and_morph ? CSS_BLOCK_BOTH : (strongs_or_morph ? CSS_BLOCK_ONE : (doublespace ? DOUBLE_SPACE : ""));
}
