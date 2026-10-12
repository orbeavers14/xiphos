/*
 * Xiphos Bible Study Tool
 * tab_history.c - add, remove and navigate history
 *
 * Copyright (C) 2005-2026 Xiphos Developer Team
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

// Frozen complete functions from CrossWire c9976a8; harness shims are separate.

void main_add_tab_history_item(gpointer data)
{
	gint i;
	PASSAGE_TAB_INFO *tab = (PASSAGE_TAB_INFO *)data;

	/* check to see if item is already in list
	   if so do nothing except set button sensitivity */
	for (i = 0; i < tab->history_items; ++i) {
		if (!strcmp(tab->history_list[i].verseref,
			    tab->text_commentary_key)) {
			tab->current_history_item = i;
			main_update_tab_history_menu(data);
			return;
		}
	}

	/* if we have hit max length, dispose of oldest */
	if (tab->history_items == TABHISTORYLENGTH) {
		for (i = 0; i < (TABHISTORYLENGTH - 1); ++i) {
			tab->history_list[i] = tab->history_list[i + 1];
		}
		--tab->history_items;
	}

	/* add item to history menu */
	strcpy(tab->history_list[tab->history_items].verseref,
	       tab->text_commentary_key);
	if (tab->text_mod)
		strcpy(tab->history_list[tab->history_items].textmod,
		       tab->text_mod);
	if (tab->commentary_mod)
		strcpy(tab->history_list[tab->history_items].commod,
		       tab->commentary_mod);

	tab->current_history_item = tab->history_items;
	++tab->history_items;

	main_update_tab_history_menu(data);
	tab->first_back_click = TRUE;
}

void main_fake_tab_history_item(char *reference)
{
	PASSAGE_TAB_INFO *tab = cur_passage_tab;

	/* if we have hit max length, dispose of oldest */
	if (tab->history_items == TABHISTORYLENGTH) {
		int i;
		for (i = 0; i < (TABHISTORYLENGTH - 1); ++i) {
			tab->history_list[i] = tab->history_list[i + 1];
		}
		--tab->history_items;
	}

	/* add item to history menu */
	strcpy(tab->history_list[tab->history_items].verseref, reference);
	if (tab->text_mod)
		strcpy(tab->history_list[tab->history_items].textmod,
		       tab->text_mod);
	if (tab->commentary_mod)
		strcpy(tab->history_list[tab->history_items].commod,
		       tab->commentary_mod);

	++tab->history_items;
	/* do not update what's current in history list. */
	main_update_tab_history_menu(tab);
}
