// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2000-2026 Xiphos Developer Team
// Adapted from src/backend/sword_main.cc; existing fork behavior retained.
// Frozen existing fork adapter before shared extraction, f4ff0ca.
GList *BackEnd::parse_verse_list(const char *module_name, const char *list, char *current_key)
{
	SWModule *mod = get_SWModule(module_name);
	if (!mod)
		return NULL;

	VerseKey *key = (VerseKey *)(SWKey *)(*mod);
	GList *retlist = NULL;
	ListKey vs;

	if (!list)
		return retlist;
	XI_message(("current_key=%s", current_key));
	key->setText(current_key);
	vs = key->parseVerseList(list, *key, TRUE);

	int count = vs.getCount();
	if (!count)
		return retlist;
	/* Use indexed access rather than popError()-driven iteration:
	 * popError() on the ListKey stops the whole walk as soon as ANY
	 * single element fails to resolve, silently dropping every
	 * element after it, even when they are perfectly valid. Indexed
	 * access lets us simply skip a bad element and keep going.
	 *
	 * But each INDIVIDUAL element also carries its own error state:
	 * when a fragment of the input list doesn't parse as a valid
	 * verse reference at all (e.g. a dictionary/Strong's-number key
	 * or some other non-scriptural bookmark text accidentally fed
	 * in here), Sword does not simply omit it -- it silently falls
	 * back to some arbitrary boundary position (observed: the very
	 * last verse of the Bible) and reports that as a "resolved"
	 * element without necessarily failing the whole ListKey. Left
	 * unchecked, that spurious element then looks like a genuine
	 * match for whatever verse it landed on. Skip any element whose
	 * own popError() is set, since it does not represent a real
	 * reference from the input. */
	for (int i = 0; i < count; i++) {
		SWKey *elem = vs.getElement(i);
		if (!elem || elem->popError())
			continue;

		/* If this element is a hyphenated range (e.g. "Jer 3:6-11"),
		 * elem->getText() only gives the START of the range -- Sword
		 * does not auto-expand it into individual verses here. Empirically,
		 * for a VerseKey list element, getUpperBound() (and getLowerBound(),
		 * which mirrors it) return the END of the range when there is one,
		 * and are otherwise equal to the element itself for a plain single
		 * verse. So: if the upper bound differs from the element's own
		 * value, walk every verse from the element (start) through the
		 * upper bound (end) and add each individually -- restoring the
		 * full-range expansion behaviour this used to have before this
		 * function was switched to indexed access. */
		VerseKey *velem = dynamic_cast<VerseKey *>(elem);
		if (velem) {
			VerseKey &bound = velem->getUpperBound();
			if (bound.compare(*velem) != 0) {
				VerseKey walker(*velem);
				int guard = 0;
				while ((walker.compare(bound) <= 0) && !walker.popError() && (guard++ < 2000)) {
					retlist = g_list_append(retlist, strdup((char *)walker.getText()));
					walker.increment(1);
				}
				continue;
			}
		}

		retlist = g_list_append(retlist, strdup((char *)elem->getText()));
	}
	return retlist;
}

