		// several verse colorization choices.
		// Shared priority; tag span remains outside annotation/current colors.
		color_choices = COLOR_NONE;
		color_chosen_fg = NULL;
		color_chosen_bg = NULL;
		switch (xh_choose_verse_color(settings.annotate_highlight, e != NULL,
					     (curChapter == thisChapter) && (curVerse == k),
					     tag_color != NULL, settings.versehighlight)) {
		case XH_VERSE_COLOR_ANNOTATION:
			color_choices = COLOR_BOTH;
			color_chosen_fg = settings.highlight_bg;
			color_chosen_bg = settings.highlight_fg;
			break;
		case XH_VERSE_COLOR_CURRENT_HIGHLIGHT:
			color_choices = COLOR_BOTH;
			color_chosen_fg = settings.highlight_fg;
			color_chosen_bg = settings.highlight_bg;
			break;
		case XH_VERSE_COLOR_CURRENT_TEXT:
			color_choices = COLOR_TEXT;
			color_chosen_fg = settings.currentverse_color;
			break;
		case XH_VERSE_COLOR_NONE:
			break;
		}
