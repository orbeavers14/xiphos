// SPDX-License-Identifier: GPL-2.0-or-later
// Characterization baseline copied from Xiphos global_ops.cc at c9976a8.
// Copyright (C) 2000-2026 Xiphos Developer Team
void legacy_set_global_options(GLOBAL_OPS *ops)
{
	_set_global_option("Strong's Numbers", ops->strongs);
	_set_global_option("Morphological Tags", ops->morphs);
	_set_global_option("Footnotes", ops->footnotes);
	_set_global_option("Greek Accents", ops->greekaccents);
	_set_global_option("Lemmas", ops->lemmas);
	_set_global_option("Cross-references", ops->scripturerefs);
	_set_global_option("Hebrew Vowel Points", ops->hebrewpoints);
	_set_global_option("Hebrew Cantillation", ops->hebrewcant);

	// always turn on headings in the engine (and later cache them).
	// whether we display them or not is another matter (display.cc).
	_set_global_option("Headings", 1);
	_set_global_option("Italic Headings", ops->italic_headings);

	_set_global_option("Words of Christ in Red", ops->words_in_red);
	_set_global_option("Transliterated Forms", ops->xlit);
	_set_global_option("Enumerations", ops->enumerated);
	_set_global_option("Glosses", ops->glosses);
	_set_global_option("Morpheme Segmentation", ops->morphseg);

	_set_global_textual("Transliteration", (ops->transliteration
						    ? "Latin"
						    : "Off"));

	if (ops->variants_primary)
		_set_global_textual("Textual Variants", "Primary Reading");
	else if (ops->variants_secondary)
		_set_global_textual("Textual Variants", "Secondary Reading");
	else if (ops->variants_all)
		_set_global_textual("Textual Variants", "All Readings");
}
