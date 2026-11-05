		// abbrev collisions disallowed: no dups of any .conf's [Name].
		if (abbreviation && !main_is_module((char *)abbreviation)) {
			main_add_abbreviation(modname, abbreviation);
		}
