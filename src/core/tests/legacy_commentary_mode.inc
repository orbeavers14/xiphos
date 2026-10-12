/*
 * Xiphos Bible Study Tool
 * mod_global_ops.c - setup for SWORD global options and
 *                    a few of our own in the gui
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

// Frozen commentary mode assignment from main_new_globals, CrossWire c9976a8.
bool frozen_commentary_mode(FrozenBackend *backend, const char *mod_name, sword::SWConfig &module_options) {
  struct { bool commentary_by_chapter; } value; auto *ops = &value;
	ops->commentary_by_chapter =
	    ((backend->module_type(mod_name) == COMMENTARY_TYPE) &&
	     (module_options[mod_name]["Commentary by Chapter"] != "Off"));
return ops->commentary_by_chapter;
}
