static void
_render_display_level(SWModule &imodule, unsigned long offset,
                      int max_level, int cur_level, SWBuf &combined)
{
	TreeKeyIdx *treekey = (TreeKeyIdx *)imodule.getKey();
	treekey->setOffset(offset);
	imodule.getRawEntry(); // snap to entry

	const char *raw = imodule.getRawEntry();
	if (raw && *raw) {
		if (combined.length() > 0)
			combined += "<br/><hr/>";
		combined += imodule.renderText().c_str();
	}

	if (cur_level < max_level && treekey->hasChildren()) {
		treekey->firstChild();
		unsigned long child_offset = treekey->getOffset();
		bool has_next = true;
		while (has_next) {
			_render_display_level(imodule, child_offset,
					      max_level, cur_level + 1,
					      combined);
			// reposition after recursion
			treekey->setOffset(child_offset);
			imodule.getRawEntry();
			has_next = treekey->nextSibling() && !treekey->popError();
			if (has_next)
				child_offset = treekey->getOffset();
		}
		// restore to current node
		treekey->setOffset(offset);
		imodule.getRawEntry();
	}
}
