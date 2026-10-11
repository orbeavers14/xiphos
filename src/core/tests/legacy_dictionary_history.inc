// SPDX-License-Identifier: GPL-2.0-or-later
// Frozen from src/main/sword.cc at 4f2b975; Copyright 2000-2026 Xiphos Developer Team.
/* dic history nav */
static GList *dict_history_back    = NULL;
static GList *dict_history_forward = NULL;
static gboolean dict_history_navigating = FALSE;

typedef struct {
    gchar *mod_name;
    gchar *key;
} DictHistoryEntry;

static void dict_history_entry_free(gpointer data)
{
    DictHistoryEntry *e = (DictHistoryEntry *)data;
    g_free(e->mod_name);
    g_free(e->key);
    g_free(e);
}


void main_dict_history_add(const gchar *mod_name, const gchar *key)
{
	if (dict_history_navigating) return;  /* do not record during navigation */
	/* do not add if identical to last entry */
	if (dict_history_back) {
		DictHistoryEntry *last = (DictHistoryEntry *)g_list_last(dict_history_back)->data;
		if (!strcmp(last->key, key) && !strcmp(last->mod_name, mod_name))
			return;
	}
    
    /* vider le forward quand on consulte une nouvelle entrée */
    g_list_free_full(dict_history_forward, dict_history_entry_free);
    dict_history_forward = NULL;

    /* ajouter l'entrée courante dans back */
    DictHistoryEntry *e = g_new0(DictHistoryEntry, 1);
    e->mod_name = g_strdup(mod_name);
    e->key      = g_strdup(key);
    dict_history_back = g_list_append(dict_history_back, e);

    /* limiter l'historique à 50 entrées */
    while (g_list_length(dict_history_back) > 50) {
        dict_history_entry_free(dict_history_back->data);
        dict_history_back = g_list_delete_link(dict_history_back,
                                               dict_history_back);
    }

    /* mettre à jour l'état des boutons */
    gtk_widget_set_sensitive(widgets.button_dict_back,
        g_list_length(dict_history_back) > 1);
    gtk_widget_set_sensitive(widgets.button_dict_forward,
        dict_history_forward != NULL);
}


/******************************************************************************
 * Name
 * main_dict_history_back
 */
 
void main_dict_history_back(void)
{
    if (!dict_history_back || g_list_length(dict_history_back) < 2) return;

    /* déplacer l'entrée courante (dernière) vers forward */
    GList *last = g_list_last(dict_history_back);
    dict_history_forward = g_list_append(dict_history_forward, last->data);
    dict_history_back = g_list_delete_link(dict_history_back, last);

    /* afficher l'entrée précédente */
    DictHistoryEntry *e = (DictHistoryEntry *)g_list_last(dict_history_back)->data;
    dict_history_navigating = TRUE;
    main_display_dictionary(e->mod_name, e->key);
    dict_history_navigating = FALSE;

    /* mettre à jour boutons */
    gtk_widget_set_sensitive(widgets.button_dict_back,
        g_list_length(dict_history_back) > 1);
    gtk_widget_set_sensitive(widgets.button_dict_forward,
        dict_history_forward != NULL);
}

/******************************************************************************
 * Name
 * main_dict_history_forward
 */

void main_dict_history_forward(void)
{
    if (!dict_history_forward) return;

    GList *last = g_list_last(dict_history_forward);
    DictHistoryEntry *e = (DictHistoryEntry *)last->data;
    dict_history_forward = g_list_delete_link(dict_history_forward, last);
    dict_history_back = g_list_append(dict_history_back, e);

    dict_history_navigating = TRUE;
    main_display_dictionary(e->mod_name, e->key);
    dict_history_navigating = FALSE;

    gtk_widget_set_sensitive(widgets.button_dict_back,
        g_list_length(dict_history_back) > 1);
    gtk_widget_set_sensitive(widgets.button_dict_forward,
        dict_history_forward != NULL);
}
