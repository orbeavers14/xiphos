std::string actual_commentary_anchors(const std::vector<unsigned> &positions, unsigned selected) {
sword::SWBuf swbuf; gchar *buf = nullptr;
int curVerse = static_cast<int>(selected); bool before_curVerse = true;
struct Key { int verse; int getVerse() const { return verse; } } value; auto *key = &value;
for (auto verse : positions) { value.verse = static_cast<int>(verse);
		if ((curVerse != 1) && // not at top of pane
		    ((curVerse == key->getVerse()) ||
		     (before_curVerse && (key->getVerse() > curVerse)))) {
			buf = NULL;
			swbuf.appendFormatted("<tr><td>%s<hr/></td><td><hr/></td></tr>",
					      // repeated conditional check here
					      ((before_curVerse &&
						(key->getVerse() > curVerse))
					       ? (buf = g_strdup_printf(
							  "<a name=\"%d\"> </a>", curVerse))
					       : ""));
			g_free(buf);
		}

swbuf.appendFormatted("<entry verse=\"%d\"><a name=\"%d\"> </a></entry>", key->getVerse(), key->getVerse());
before_curVerse = key->getVerse() < curVerse;
}
	if (before_curVerse) {
		swbuf.appendFormatted("<tr><td><a name=\"%d\"> </a><hr/></td><td><hr/></td></tr>",
				      curVerse);
	}

return swbuf.c_str();
}
