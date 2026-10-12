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
#include "xiphos/core/lexical_style.hpp"
namespace xiphos { namespace core {
namespace {
const char both[] =
  " *        { line-height: 3.5em; }"
  " .word    { position: relative; top:  0.0em; left: 0; }"
  " .strongs { position: absolute; top:  0.4em; left: 0; white-space: nowrap; z-index: 2 }"
  " .morph   { position: absolute; top:  0.9em; left: 0; white-space: nowrap; z-index: 1 }"
  " *[dir=rtl] .word { padding-left: 0.8em; }"
  " *[dir=rtl] .strongs { top: 0.4em; left: auto; right: 0; }"
  " *[dir=rtl] .morph { left: auto; right: 0; }";
const char one[] =
  " *        { line-height: 2.7em; }"
  " .word    { position: relative; top:  0.0em; left: 0; }"
  " .strongs { position: absolute; top:  0.4em; left: 0; white-space: nowrap; }"
  " .morph   { position: absolute; top:  0.4em; left: 0; white-space: nowrap; }"
  " *[dir=rtl] .word { padding-left: 0.8em; }"
  " *[dir=rtl] .strongs { left: auto; right: 0; }"
  " *[dir=rtl] .morph { left: auto; right: 0; }";
const char spaced[] =
  " * { line-height: 2em ! important; }";
}
std::string lexical_block_style(bool strongs, bool lemmas, bool morphology, bool double_space) {
  if ((strongs || lemmas) && morphology) return both;
  if (strongs || lemmas || morphology) return one;
  return double_space ? spaced : "";
}
}}
