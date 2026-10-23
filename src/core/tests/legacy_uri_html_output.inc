// Frozen from 39ead32; complete retained anchor consumer.
/*
 * Xiphos Bible Study Tool
 * utilities.c - support functions
 *
 * Copyright (C) 2000-2026 Xiphos Developer Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 */
void
HtmlOutput(char *text, GtkWidget *gtkText, MOD_FONT *mf, char *anchor)
{
	int len = strlen(text);

	XiphosHtml *html = XIPHOS_HTML(gtkText);
	XIPHOS_HTML_OPEN_STREAM(html, "text/html");

	// EVIL EVIL EVIL EVIL.
	// crazy nonsense with xulrunner 1.9.2.3, failure to jump to anchor.
	// force the issue by stuffing a javascript snippet inside <head></head>.
	// there are forms of evil so dark that they should not be contemplated.
	if (anchor || settings.special_anchor) {
		gchar *buf;

		// first, scribble out everything up to the closing </head>.
		buf = strstr(text, "</head>"); // yes, lowercase.
		assert(buf != NULL);	   // don't be so stupid as not to include <head></head>.
		int offset = buf - text;
		XIPHOS_HTML_WRITE(html, text, offset);
		len -= offset;

		// now write the javascript snippet.
		// escape the anchor for safe interpolation into a JS
		// string literal.  special_anchor can originate from
		// untrusted BibleSync navigation packets whose ref
		// field carries the '#' fragment; without escaping a
		// crafted value can break out of the string and inject
		// script into the WebKit view.  g_strescape escapes
		// backslash, double-quote, and C0 control bytes but
		// leaves '<', '>', and '/' untouched, so a value of
		// "</script><script>PAYLOAD</script>" would end the
		// inline <script> block and start a new one with
		// attacker-controlled contents.  Substitute '</' with
		// '<\/' (OWASP recommendation) to neutralise the
		// breakout.
		const gchar *raw_anchor =
		    (settings.special_anchor ? settings.special_anchor : anchor);
		gchar *esc_anchor = g_strescape(raw_anchor, NULL);
		gchar *safe_anchor = g_strdup(esc_anchor);
		gchar *q;
		for (q = safe_anchor; (q = strstr(q, "</")) != NULL; q += 2)
			q[1] = '\\';
		g_free(esc_anchor);
		buf =
		    g_strdup_printf("<script type=\"text/javascript\" language=\"javascript\">"
				    " window.onload = function () { window.location.hash = \"%s\"; }"
				    " </script>",
				    safe_anchor);
		g_free(safe_anchor);
		XIPHOS_HTML_WRITE(html, buf, strlen(buf));
		g_free(buf);
	}

	if (!anchor)
		XIPHOS_HTML_WRITE(html, text, len);

	/* use anchor if asked, but if so, special anchor takes priority. */
	XIPHOS_HTML_CLOSE(html);
	if (anchor || settings.special_anchor)
		XIPHOS_HTML_JUMP_TO_ANCHOR(html, (settings.special_anchor
						      ? settings.special_anchor
						      : anchor));

	/* the special anchor gets exactly one use. */
	settings.special_anchor = NULL;
}
