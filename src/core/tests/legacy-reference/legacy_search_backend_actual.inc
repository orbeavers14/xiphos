int BackEnd::do_module_search(char *module_name,
			      const char *search_string,
			      int search_type,
			      int search_params,
			      int is_dialog)
{
	char progressunits = 70;
	results.clear();
	//search_scope_list.clear()
	search_mod = NULL;

	search_mod = main_mgr->Modules[module_name];
	if (!search_mod)
		return -1;
	if ((current_scope == &search_scope_list) &&
	    (search_scope_list.getCount() == 0))
		return 0;

	results = search_mod->search(search_string,
				     search_type,
				     search_params,
				     current_scope, 0,
				     (is_dialog
					  ? main_dialog_search_percent_update
					  : main_sidebar_search_percent_update),
				     (void *)&progressunits);
	search_scope_list = results;
	if (search_type == -4)
		results.sort();
	return results.getCount();
}
