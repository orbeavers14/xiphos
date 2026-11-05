void main_set_global_options(GLOBAL_OPS *ops)
{
	xiphos::core::EngineOptions values;
	values.strongs = ops->strongs != 0;
	values.morphs = ops->morphs != 0;
	values.footnotes = ops->footnotes != 0;
	values.greekaccents = ops->greekaccents != 0;
	values.lemmas = ops->lemmas != 0;
	values.scripturerefs = ops->scripturerefs != 0;
	values.hebrewpoints = ops->hebrewpoints != 0;
	values.hebrewcant = ops->hebrewcant != 0;
	values.italic_headings = ops->italic_headings != 0;
	values.words_in_red = ops->words_in_red != 0;
	values.xlit = ops->xlit != 0;
	values.enumerated = ops->enumerated != 0;
	values.glosses = ops->glosses != 0;
	values.morphseg = ops->morphseg != 0;
	values.transliteration = ops->transliteration != 0;
	values.variants_primary = ops->variants_primary != 0;
	values.variants_secondary = ops->variants_secondary != 0;
	values.variants_all = ops->variants_all != 0;
	xiphos::core::apply_engine_options(*backend->get_mgr(), values);
}
