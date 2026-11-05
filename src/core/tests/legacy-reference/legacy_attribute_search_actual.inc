	// Shared byte-preserving attribute grammar and whole-entry matching policy.
	if (search_type == -3) {
		const bool strongs = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(search1.rb_strongs));
		const bool morphs = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(search1.rb_morphs));
		const bool footnotes = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(search1.rb_footnotes));
		if (strongs || morphs || footnotes) {
			const auto kind = strongs ? xiphos::core::AttributeSearch::lemma
			    : morphs ? xiphos::core::AttributeSearch::morphology : xiphos::core::AttributeSearch::footnote;
			const auto prepared = xiphos::core::attribute_search_text(search_string, kind);
			if (prepared.whole_entry) search_params |= SpecialSearchFlag;
			attribute_search_string = g_strdup(prepared.expression.c_str());
		}
		XI_message(("%s", attribute_search_string));
	}
