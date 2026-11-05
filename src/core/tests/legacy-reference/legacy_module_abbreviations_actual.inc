
void main_add_abbreviation(const char *name, const char *abbreviation)
{
	module_abbreviations.add(name, abbreviation);
}


void main_clear_abbreviations(void)
{
	module_abbreviations.clear();
}


const char *main_name_to_abbrev(const char *name)
{
	return module_abbreviations.abbreviation_for(name);
}


const char *main_abbrev_to_name(const char *abbreviation)
{
	return module_abbreviations.name_for(abbreviation);
}

