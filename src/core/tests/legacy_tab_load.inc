// Frozen from 01a75e4; complete gui_load_tabs, UI/file policy host stubs.
/*
 * Xiphos Bible Study Tool
 * tabbed_browser.c - functions to facilitate tabbed browsing of different passages at once
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

void gui_load_tabs(const gchar *filename)
{
	xmlDocPtr xml_doc;
	xmlNodePtr tmp_node, childnode;
	gboolean error = FALSE;
	gboolean back_compat_need_save = FALSE;
	settings.showparatab = FALSE;
	PASSAGE_TAB_INFO *pt = NULL, *pt_first = NULL;

	stop_refresh = TRUE;

	if (filename == NULL) {
		error = TRUE;
	} else {
		gchar *file;

		if (g_access(filename, F_OK) == 0) {
			/* we're done, just copy for local use that can be free'd */
			file = g_strdup(filename);
		} else {
			gchar *tabs_dir =
			    g_strdup_printf("%s/tabs/",
					    settings.gSwordDir);
			if (g_access(tabs_dir, F_OK) == -1) {
				XI_message(("Creating new tabs directory\n"));
				gui_save_tabs(filename);
			}
			file = g_strdup_printf("%s%s", tabs_dir, filename);
			g_free(tabs_dir);
		}

		/* we need this for first time non tabbed browsing */
		if (!settings.browsing && g_access(file, F_OK) == -1) {
			_save_off_tab(filename);
		}
		xml_doc = xmlParseFile(file);
		g_free(file);
		if (xml_doc == NULL) {
			gui_generic_warning_modal("Tabs document not parsed successfully.");
			error = TRUE;
		} else {
			tmp_node = xmlDocGetRootElement(xml_doc);
			if (tmp_node == NULL) {
				gui_generic_warning_modal("Tabs document is empty.");
				xmlFreeDoc(xml_doc);
				error = TRUE;
			} else if (xmlStrcmp(tmp_node->name,
					     (const xmlChar *)"Xiphos_Tabs")) {
				gui_generic_warning_modal("Tabs document has wrong type, root node != Xiphos_Tabs");
				xmlFreeDoc(xml_doc);
				error = TRUE;
			}
		}

		if (error == FALSE) {
			for (childnode = tmp_node->children;
			     childnode != NULL;
			     childnode = childnode->next) {
				if (!xmlStrcmp(childnode->name,
					       (const xmlChar *)"tabs")) {
					tmp_node = childnode;
					for (tmp_node = tmp_node->children;
					     tmp_node != NULL;
					     tmp_node = tmp_node->next) {
						if (!xmlStrcmp(tmp_node->name,
							       (const xmlChar *)"tab")) {
							gchar *val;

							pt = g_new0(PASSAGE_TAB_INFO,
								    1);
							if (pt_first ==
							    NULL)
								pt_first =
								    pt;

							/* load per-tab module information. */
							val = (gchar *)
							    xmlGetProp(tmp_node,
								       (const xmlChar *)"text_mod");
							pt->text_mod =
							    g_strdup(val);
							xmlFree(val);
							val = (gchar *)
							    xmlGetProp(tmp_node,
								       (const xmlChar *)"commentary_mod");
							pt->commentary_mod =
							    g_strdup(val);
							xmlFree(val);
							val = (gchar *)
							    xmlGetProp(tmp_node,
								       (const xmlChar *)"dictlex_mod");
							pt->dictlex_mod =
							    g_strdup(val);
							xmlFree(val);
							val = (gchar *)
							    xmlGetProp(tmp_node,
								       (const xmlChar *)"book_mod");
							pt->book_mod =
							    g_strdup(val);
							xmlFree(val);
							val = (gchar *)
							    xmlGetProp(tmp_node,
								       (const xmlChar *)"text_commentary_key");
							pt->text_commentary_key = g_strdup(val);
							xmlFree(val);
							val = (gchar *)
							    xmlGetProp(tmp_node,
								       (const xmlChar *)"dictlex_key");
							pt->dictlex_key =
							    g_strdup(val);
							xmlFree(val);
							val = (gchar *)
							    xmlGetProp(tmp_node,
								       (const xmlChar *)"book_offset");
							pt->book_offset =
							    g_strdup(val);
							xmlFree(val);
							val = (gchar *)
							    xmlGetProp(tmp_node,
								       (const xmlChar *)"comm_showing");
							pt->comm_showing =
							    yes_no2true_false(val);
							xmlFree(val);
							val = (gchar *)
							    xmlGetProp(tmp_node,
								       (const xmlChar *)"showparallel");
							pt->showparallel =
							    yes_no2true_false(val);
							xmlFree(val);
							if (pt->showparallel) {
								settings.showparatab = TRUE;
								pt->paratab =
								    gui_create_parallel_tab();
								gtk_box_pack_start(GTK_BOX(widgets.page),
										   pt->paratab,
										   TRUE,
										   TRUE,
										   0);
								gtk_widget_hide(pt->paratab);
								gui_parallel_tab_sync((gchar *)
										      settings.currentverse);
								settings.showparatab = TRUE;
								sync_on =
								    TRUE;
							} else
								pt->paratab = NULL;

							/*
							 * load per-tab "show" state.
							 * includes backward compatibility:
							 * if there is no per-tab state,
							 * take tab state from global state.
							 */
							if ((val =
								 (gchar *)
							     xmlGetProp(tmp_node,
									(const xmlChar *)"showtexts"))) {
								pt->showtexts = yes_no2true_false(val);
								xmlFree(val);
								val =
								    (gchar *)
								    xmlGetProp(tmp_node,
									       (const xmlChar *)"showpreview");
								pt->showpreview = yes_no2true_false(val);
								xmlFree(val);
								val =
								    (gchar *)
								    xmlGetProp(tmp_node,
									       (const xmlChar *)"showcomms");
								pt->showcomms = yes_no2true_false(val);
								xmlFree(val);
								val =
								    (gchar *)
								    xmlGetProp(tmp_node,
									       (const xmlChar *)"showdicts");
								pt->showdicts = yes_no2true_false(val);
								xmlFree(val);

							} else {
								pt->showtexts = settings.showtexts;
								pt->showpreview = settings.showpreview;
								pt->showcomms = settings.showcomms;
								pt->showdicts = settings.showdicts;
								back_compat_need_save = TRUE;
							}

							pt->history_items =
							    0;
							pt->current_history_item = 0;
							pt->first_back_click = TRUE;
							main_add_tab_history_item((PASSAGE_TAB_INFO *)pt);

							gui_reassign_strdup(&settings.currentverse,
									    pt->text_commentary_key);

							passage_list =
							    g_list_append(passage_list,
									  (PASSAGE_TAB_INFO *)pt);
							notebook_main_add_page(pt);
						}
					}
				}
			}
		}
		xmlFreeDoc(xml_doc);

		/* backward compatibility completion. */
		if (back_compat_need_save)
			gui_save_tabs(filename);

		if (error == TRUE || pt == NULL) {
			pt = g_new0(PASSAGE_TAB_INFO, 1);
			pt->text_mod = g_strdup(settings.MainWindowModule);
			pt->commentary_mod =
			    g_strdup(settings.CommWindowModule);
			pt->dictlex_mod =
			    g_strdup(settings.DictWindowModule);
			pt->book_mod = g_strdup(settings.book_mod); //NULL;
			pt->text_commentary_key =
			    g_strdup(settings.currentverse);
			pt->dictlex_key = g_strdup(settings.dictkey);
			pt->book_offset = NULL; //settings.book_offset = atol(xml_get_value( "keys", "offset"));

			pt->paratab = NULL;

			pt->showtexts = settings.showtexts;
			pt->showpreview = settings.showpreview;
			pt->showcomms = settings.showcomms;
			pt->showdicts = settings.showdicts;

			pt->history_items = 0;
			pt->current_history_item = 0;
			pt->first_back_click = TRUE;
			passage_list =
			    g_list_append(passage_list,
					  (PASSAGE_TAB_INFO *)pt);
			notebook_main_add_page(pt);
		} else {
			// first passage is current/displayed.
			pt = pt_first;

			pt->paratab = NULL;
			// This is a hack to keep gs from loading the settings
			// from the last session into the last tab loaded here.
			gui_reassign_strdup(&settings.MainWindowModule,
					    pt->text_mod);
			gui_reassign_strdup(&settings.CommWindowModule,
					    pt->commentary_mod);
			gui_reassign_strdup(&settings.DictWindowModule,
					    pt->dictlex_mod);
			gui_reassign_strdup(&settings.book_mod,
					    pt->book_mod);
			gui_reassign_strdup(&settings.currentverse,
					    pt->text_commentary_key);
			gui_reassign_strdup(&settings.dictkey,
					    pt->dictlex_key);
			settings.book_offset = atol(pt->book_offset);
		}
	}

	stop_refresh = FALSE;
	set_current_tab(pt);
	gui_recompute_view_menu_choices();
}

