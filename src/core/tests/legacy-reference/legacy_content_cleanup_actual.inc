GString *
CleanupContent(GString *text,
	       GLOBAL_OPS *ops,
	       const char *name,
	       bool reset = true)
{
    const auto cleaned = xiphos::core::cleanup_html_content(
        text->str, static_cast<xiphos::core::ContentPreference>(ops->image_content),
        static_cast<xiphos::core::ContentPreference>(ops->respect_font_faces));
    memcpy(text->str, cleaned.html.c_str(), cleaned.html.size());
    auto report = [&](xiphos::core::ContentCleanupKind kind) {
        for (const auto &issue : cleaned.diagnostics) {
            if (issue.kind != kind) continue;
            if (kind == xiphos::core::ContentCleanupKind::image)
                XI_message(("ClearImages: no img end: %s\n", issue.suffix.c_str()));
            else
                XI_message(("ClearFontFaces: no closing double quote: %s\n", issue.suffix.c_str()));
        }
    };
    report(xiphos::core::ContentCleanupKind::image);
    ops->image_content = static_cast<int>(cleaned.image_content);
    if (cleaned.discovered_images)
        main_save_module_options(name, "Image Content", 1);
    report(xiphos::core::ContentCleanupKind::font_face);
    ops->respect_font_faces = static_cast<int>(cleaned.respect_font_faces);
    if (cleaned.discovered_font_faces)
        main_save_module_options(name, "Respect Font Faces", 1);

    // Keep the backend's exact first-note label probe and call ordering. A
    // reported module label suppresses automatic numbering for the whole entry.
    gchar *reported = backend->get_entry_attribute("Footnote", "1", "n", false);
    const bool labelled = reported != NULL;
    g_free(reported);
    const auto numbered = xiphos::core::number_note_markers(
        text->str, ops->xrefnotenumbers != 0, labelled,
        xiphos::core::NoteNumberingState(footnote, xref));
    g_string_assign(text, numbered.html.c_str());
    footnote = numbered.counters.footnotes;
    xref = numbered.counters.cross_references;

	return text;
}

