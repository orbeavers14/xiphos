
void main_add_tab_history_item(gpointer data)
{
	gint i;
	PASSAGE_TAB_INFO *tab = (PASSAGE_TAB_INFO *)data;

	/* check to see if item is already in list
	   if so do nothing except set button sensitivity */
	const char *references[TABHISTORYLENGTH];
	for (i = 0; i < tab->history_items; ++i)
		references[i] = tab->history_list[i].verseref;
	size_t found = xh_find_reference(references, tab->history_items,
					tab->text_commentary_key);
	if (found < (size_t)tab->history_items) {
		tab->current_history_item = found;
		main_update_tab_history_menu(data);
		return;
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
