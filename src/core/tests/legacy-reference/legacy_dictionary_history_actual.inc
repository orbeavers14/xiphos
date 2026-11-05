/* dic history nav */
static xiphos::core::DictionaryHistory dictionary_history;
static gboolean dict_history_navigating = FALSE;



void main_dict_history_add(const gchar *mod_name, const gchar *key)
{
    if (dict_history_navigating) return;
    if (!dictionary_history.visit({mod_name, key})) return;
    gtk_widget_set_sensitive(widgets.button_dict_back, dictionary_history.can_back());
    gtk_widget_set_sensitive(widgets.button_dict_forward, dictionary_history.can_forward());
}

void main_dict_history_back(void)
{
    if (!dictionary_history.can_back()) return;
    const auto target = dictionary_history.destination(-1);
    dictionary_history.move(-1);
    dict_history_navigating = TRUE;
    main_display_dictionary(target.module.c_str(), target.key.c_str());
    dict_history_navigating = FALSE;
    gtk_widget_set_sensitive(widgets.button_dict_back, dictionary_history.can_back());
    gtk_widget_set_sensitive(widgets.button_dict_forward, dictionary_history.can_forward());
}

void main_dict_history_forward(void)
{
    if (!dictionary_history.can_forward()) return;
    const auto target = dictionary_history.destination(1);
    dictionary_history.move(1);
    dict_history_navigating = TRUE;
    main_display_dictionary(target.module.c_str(), target.key.c_str());
    dict_history_navigating = FALSE;
    gtk_widget_set_sensitive(widgets.button_dict_back, dictionary_history.can_back());
    gtk_widget_set_sensitive(widgets.button_dict_forward, dictionary_history.can_forward());
}
