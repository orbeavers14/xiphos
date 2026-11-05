int BackEnd::clear_search_list(void)
{
	search_scope_list.clear();
	return search_scope_list.getCount();
}

int BackEnd::set_range(const char *module_name, const char *list)
{
	SWModule *mod = get_SWModule(module_name);
	if (!mod)
		return 0;

	VerseKey *key = (VerseKey *)(SWKey *)(*mod);

	// setText can release this key's own getText buffer before copying it.
	const std::string context = mod->getKeyText();
	key->setText(context.c_str());
	search_range = key->parseVerseList(list, "", true);
	return search_range.getCount();
}

void BackEnd::set_scope2range(void)
{
	current_scope = &search_range;
}

int BackEnd::set_scope2last_search(void)
{
	current_scope = &search_scope_list; //-- move searchlist into current_scope
	return 1;
}

