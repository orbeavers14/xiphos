// SPDX-License-Identifier: GPL-2.0-or-later
#include "study_ui.hpp"
#include <memory>
#include <set>
namespace xiphos::linux_preview
{
namespace
{
using Variant = std::unique_ptr<GVariant, decltype(&g_variant_unref)>;
Variant parse(const std::string &state)
{
	if (state.empty())
		return {nullptr, g_variant_unref};
	if (state.compare(0, 8, "gtk4-v1:") != 0)
		return {nullptr, g_variant_unref};
	GError *error = nullptr;
	auto *v = g_variant_parse(G_VARIANT_TYPE_VARDICT, state.c_str() + 8, nullptr, nullptr, &error);
	g_clear_error(&error);
	return {v, g_variant_unref};
}
std::string text(GtkWidget *w) { return gtk_editable_get_text(GTK_EDITABLE(w)); }
} // namespace
bool StudyUI::validate_state(const std::string &state)
{
	if (state.empty())
		return true;
	if (state.size() > 32768)
		return false;
	auto v = parse(state);
	if (!v)
		return false;
	const std::set<std::string> strings{"modules", "query", "bounds", "first", "last", "bookmarks", "annotations", "parallelProfile", "referenceDraft", "boundsProvider"};
	const std::set<std::string> booleans{"mixed", "case", "annotationsOverride", "strongs", "morphology", "footnotes", "chapter", "exportHTML", "exportAll", "exportText", "exportDetails", "exportImages"};
	const std::set<std::string> numbers{"mode", "scope", "displayFlags", "comparisonLayout"};
	std::set<std::string> seen;
	GVariantIter it;
	g_variant_iter_init(&it, v.get());
	const gchar *key = nullptr;
	GVariant *value = nullptr;
	while (g_variant_iter_next(&it, "{&sv}", &key, &value)) {
		if (!seen.insert(key).second) {
			g_variant_unref(value);
			return false;
		}
		bool valid = strings.count(key) ? g_variant_is_of_type(value, G_VARIANT_TYPE_STRING) : booleans.count(key) ? g_variant_is_of_type(value, G_VARIANT_TYPE_BOOLEAN)
															   : numbers.count(key) && g_variant_is_of_type(value, G_VARIANT_TYPE_UINT32);
		if (std::string(key) == "readingStyles") {
			valid = g_variant_is_of_type(value, G_VARIANT_TYPE("a{s(uuss)}")) && g_variant_n_children(value) <= 64;
			if (valid) {
				GVariantIter styles;
				g_variant_iter_init(&styles, value);
				const char *module, *foreground, *background;
				guint font, size;
				while (valid && g_variant_iter_next(&styles, "{&s(uu&s&s)}", &module, &font, &size, &foreground, &background))
					valid = font <= 2 && size >= 12 && size <= 36 &&
						valid_reading_color(foreground) && valid_reading_color(background);
			}
		}
		if (std::string(key) == "hiddenTranslations") {
			valid = g_variant_is_of_type(value, G_VARIANT_TYPE("a(asab)")) && g_variant_n_children(value) <= 16;
			for (size_t i = 0; valid && i < g_variant_n_children(value); ++i) {
				Variant entry(g_variant_get_child_value(value, i), g_variant_unref);
				Variant names(g_variant_get_child_value(entry.get(), 0), g_variant_unref);
				Variant flags(g_variant_get_child_value(entry.get(), 1), g_variant_unref);
				valid = g_variant_n_children(names.get()) == g_variant_n_children(flags.get()) &&
					g_variant_n_children(names.get()) <= 32;
			}
		}
		if (valid && std::string(key) == "comparisonLayout")
			valid = g_variant_get_uint32(value) <= 1;
		if (valid && std::string(key) == "mode")
			valid = g_variant_get_uint32(value) <= XC_SEARCH_SURFACE_UNMARKED;
		if (valid && std::string(key) == "scope")
			valid = g_variant_get_uint32(value) <= 3;
		if (valid && std::string(key) == "displayFlags")
			valid = (g_variant_get_uint32(value) & ~(XC_CHAPTER_VERSE_NUMBERS | XC_CHAPTER_HIGHLIGHT_SELECTED)) == 0;
		g_variant_unref(value);
		if (!valid)
			return false;
	}
	return true;
}
std::string StudyUI::capture_state(const std::string &reference_draft) const
{
	GVariantBuilder b;
	g_variant_builder_init(&b, G_VARIANT_TYPE_VARDICT);
	auto string = [&](const char *key, const std::string &s) { g_variant_builder_add(&b, "{sv}", key, g_variant_new_string(s.c_str())); };
	auto boolean = [&](const char *key, bool v) { g_variant_builder_add(&b, "{sv}", key, g_variant_new_boolean(v)); };
	auto number = [&](const char *key, unsigned v) { g_variant_builder_add(&b, "{sv}", key, g_variant_new_uint32(v)); };
	if (parallel_layout_)
		number("comparisonLayout", gtk_drop_down_get_selected(GTK_DROP_DOWN(parallel_layout_)));
	GVariantBuilder visibility;
	g_variant_builder_init(&visibility, G_VARIANT_TYPE("a(asab)"));
	size_t saved = 0, name_bytes = 0;
	for (const auto &[order, hidden] : parallel_hidden_) {
		if (saved >= 16 || order.size() > 32 || hidden.size() != order.size())
			continue;
		size_t group_bytes = 0;
		for (const auto &module : order)
			group_bytes += module.size();
		if (name_bytes + group_bytes > 4096)
			continue;
		GVariantBuilder names, flags;
		g_variant_builder_init(&names, G_VARIANT_TYPE("as"));
		g_variant_builder_init(&flags, G_VARIANT_TYPE("ab"));
		for (size_t i = 0; i < order.size(); ++i) {
			g_variant_builder_add(&names, "s", order[i].c_str());
			g_variant_builder_add(&flags, "b", bool(hidden[i]));
		}
		g_variant_builder_add(&visibility, "(asab)", &names, &flags);
		++saved;
		name_bytes += group_bytes;
	}
	g_variant_builder_add(&b, "{sv}", "hiddenTranslations", g_variant_builder_end(&visibility));
	GVariantBuilder styles;
	g_variant_builder_init(&styles, G_VARIANT_TYPE("a{s(uuss)}"));
	size_t style_count = 0, style_name_bytes = 0;
	for (const auto &[module, style] : reading_styles_) {
		if (style_count >= 64 || style_name_bytes + module.size() > 4096)
			continue;
		g_variant_builder_add(&styles, "{s(uuss)}", module.c_str(), style.font, style.size,
			style.foreground.c_str(), style.background.c_str());
		++style_count;
		style_name_bytes += module.size();
	}
	g_variant_builder_add(&b, "{sv}", "readingStyles", g_variant_builder_end(&styles));
	string("referenceDraft", reference_draft);
	const auto provider = gtk_drop_down_get_selected(GTK_DROP_DOWN(range_module_));
	if (provider < search_ids_.size())
		string("boundsProvider", search_ids_[provider]);
	string("modules", text(search_modules_));
	string("query", text(search_text_));
	string("bounds", text(range_));
	string("first", text(first_key_));
	string("last", text(last_key_));
	boolean("mixed", gtk_check_button_get_active(GTK_CHECK_BUTTON(mixed_)));
	boolean("case", gtk_check_button_get_active(GTK_CHECK_BUTTON(case_)));
	boolean("annotationsOverride", gtk_check_button_get_active(GTK_CHECK_BUTTON(annotations_)));
	const char *annotations[] = {"strongs", "morphology", "footnotes"};
	for (int i = 0; i < 3; ++i)
		boolean(annotations[i], gtk_check_button_get_active(GTK_CHECK_BUTTON(annotation_[i])));
	number("mode", gtk_drop_down_get_selected(GTK_DROP_DOWN(search_mode_)));
	number("scope", gtk_drop_down_get_selected(GTK_DROP_DOWN(search_scope_)));
	if (model_) {
		boolean("chapter", model_->show_chapter());
		number("displayFlags", model_->display_flags());
		string("bookmarks", model_->bookmarks().revision ? model_->bookmarks().path : "");
		string("annotations", model_->annotations().revision ? model_->annotations().path : "");
		string("parallelProfile", model_->parallel_profile().revision ? model_->parallel_profile().path : "");
	}
	const char *exportNames[] = {"exportHTML", "exportAll", "exportText", "exportDetails", "exportImages"};
	GtkWidget *exportWidgets[] = {export_html_, export_all_, export_scripture_, export_context_, export_images_};
	for (int i = 0; i < 5; ++i)
		boolean(exportNames[i], gtk_check_button_get_active(GTK_CHECK_BUTTON(exportWidgets[i])));
	auto *v = g_variant_ref_sink(g_variant_builder_end(&b));
	auto *printed = g_variant_print(v, TRUE);
	std::string out = "gtk4-v1:" + std::string(printed);
	g_free(printed);
	g_variant_unref(v);
	return out;
}
std::optional<std::string> StudyUI::reference_draft(const std::string &state)
{
	if (!validate_state(state) || state.empty())
		return std::nullopt;
	auto v = parse(state);
	const gchar *text = nullptr;
	if (g_variant_lookup(v.get(), "referenceDraft", "&s", &text))
		return std::string(text);
	return std::nullopt;
}
bool StudyUI::restore_state(const std::string &state)
{
	if (!validate_state(state))
		return false;
	if (state.empty())
		return true;
	if (state.size() > 32768)
		return false;
	auto v = parse(state);
	const bool updating = updating_;
	updating_ = true;
	auto set = [&](const char *key, GtkWidget *w) {const gchar *s=nullptr;if(g_variant_lookup(v.get(),key,"&s",&s))gtk_editable_set_text(GTK_EDITABLE(w),s); };
	auto toggle = [&](const char *key, GtkWidget *w) {gboolean active=false;if(g_variant_lookup(v.get(),key,"b",&active))gtk_check_button_set_active(GTK_CHECK_BUTTON(w),active); };
	set("modules", search_modules_);
	set("query", search_text_);
	set("bounds", range_);
	set("first", first_key_);
	set("last", last_key_);
	const gchar *provider = nullptr;
	if (g_variant_lookup(v.get(), "boundsProvider", "&s", &provider))
		for (size_t i = 0; i < search_ids_.size(); ++i)
			if (search_ids_[i] == provider)
				gtk_drop_down_set_selected(GTK_DROP_DOWN(range_module_), i);
	toggle("mixed", mixed_);
	toggle("case", case_);
	toggle("annotationsOverride", annotations_);
	const char *names[] = {"strongs", "morphology", "footnotes"};
	for (int i = 0; i < 3; ++i)
		toggle(names[i], annotation_[i]);
	guint layout = 0;
	if (parallel_layout_ && g_variant_lookup(v.get(), "comparisonLayout", "u", &layout))
		gtk_drop_down_set_selected(GTK_DROP_DOWN(parallel_layout_), layout);
	Variant visibility(g_variant_lookup_value(v.get(), "hiddenTranslations", G_VARIANT_TYPE("a(asab)")), g_variant_unref);
	if (visibility) {
		parallel_hidden_.clear();
		for (size_t i = 0; i < g_variant_n_children(visibility.get()); ++i) {
			Variant entry(g_variant_get_child_value(visibility.get(), i), g_variant_unref);
			Variant names(g_variant_get_child_value(entry.get(), 0), g_variant_unref);
			Variant flags(g_variant_get_child_value(entry.get(), 1), g_variant_unref);
			std::vector<std::string> order;
			std::vector<bool> hidden;
			for (size_t c = 0; c < g_variant_n_children(names.get()); ++c) {
				Variant name(g_variant_get_child_value(names.get(), c), g_variant_unref);
				Variant flag(g_variant_get_child_value(flags.get(), c), g_variant_unref);
				order.emplace_back(g_variant_get_string(name.get(), nullptr));
				hidden.push_back(g_variant_get_boolean(flag.get()));
			}
			parallel_hidden_.emplace(std::move(order), std::move(hidden));
		}
		parallel_order_.clear();
		parallel_needs_render_ = true;
	}
	Variant styles(g_variant_lookup_value(v.get(), "readingStyles", G_VARIANT_TYPE("a{s(uuss)}")), g_variant_unref);
	if (styles) {
		reading_styles_.clear();
		GVariantIter items;
		g_variant_iter_init(&items, styles.get());
		const char *module, *foreground, *background;
		guint font, size;
		while (g_variant_iter_next(&items, "{&s(uu&s&s)}", &module, &font, &size, &foreground, &background))
			reading_styles_.emplace(module, ReadingStyle{font, size, foreground, background});
		++style_revision_;
		parallel_needs_render_ = true;
		if (style_module_)
			load_reading_style();
	}
	guint mode = XC_SEARCH_PHRASE, scope = 0, flags = XC_CHAPTER_VERSE_NUMBERS | XC_CHAPTER_HIGHLIGHT_SELECTED;
	gboolean chapter = false;
	g_variant_lookup(v.get(), "mode", "u", &mode);
	g_variant_lookup(v.get(), "scope", "u", &scope);
	g_variant_lookup(v.get(), "displayFlags", "u", &flags);
	g_variant_lookup(v.get(), "chapter", "b", &chapter);
	gtk_drop_down_set_selected(GTK_DROP_DOWN(search_mode_), mode);
	gtk_drop_down_set_selected(GTK_DROP_DOWN(search_scope_), scope);
	const char *exports[] = {"exportHTML", "exportAll", "exportText", "exportDetails", "exportImages"};
	GtkWidget *widgets[] = {export_html_, export_all_, export_scripture_, export_context_, export_images_};
	for (int i = 0; i < 5; ++i)
		toggle(exports[i], widgets[i]);
	const char *paths[] = {"bookmarks", "annotations", "parallelProfile"};
	for (int i = 0; i < 3; ++i) {
		const gchar *p = nullptr;
		if (g_variant_lookup(v.get(), paths[i], "&s", &p) && *p)
			restore_sources_.push_back({i, p});
	}
	if (model_ && model_->ready() && !model_->busy())
		model_->set_display(chapter, flags);
	updating_ = updating;
	return true;
}
} // namespace xiphos::linux_preview
