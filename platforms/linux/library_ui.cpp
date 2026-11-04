// SPDX-License-Identifier: GPL-2.0-or-later
#include "library_ui.hpp"
#include <algorithm>
#include <string>
namespace xiphos::linux_preview
{
namespace
{
std::string text(GtkWidget *w) { return gtk_editable_get_text(GTK_EDITABLE(w)); }
bool active(GtkWidget *w) { return gtk_check_button_get_active(GTK_CHECK_BUTTON(w)); }
GtkWidget *row(GtkWidget *parent)
{
	auto *w = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
	gtk_box_append(GTK_BOX(parent), w);
	return w;
}
GtkWidget *check(GtkWidget *parent, const char *label)
{
	auto *w = gtk_check_button_new_with_label(label);
	gtk_box_append(GTK_BOX(parent), w);
	return w;
}
} // namespace
GtkWidget *LibraryUI::entry(GtkWidget *box, const char *label)
{
	auto *w = gtk_entry_new();
	gtk_entry_set_placeholder_text(GTK_ENTRY(w), label);
	gtk_accessible_update_property(GTK_ACCESSIBLE(w), GTK_ACCESSIBLE_PROPERTY_LABEL, label, -1);
	gtk_widget_set_hexpand(w, TRUE);
	gtk_box_append(GTK_BOX(box), w);
	return w;
}
GtkWidget *LibraryUI::button(GtkWidget *box, const char *label, int action)
{
	auto *w = gtk_button_new_with_label(label);
	gtk_box_append(GTK_BOX(box), w);
	g_object_set_data(G_OBJECT(w), "action", GINT_TO_POINTER(action));
	g_signal_connect(w, "clicked", G_CALLBACK(+[](GtkButton *w, gpointer data) { static_cast<LibraryUI *>(data)->action(GPOINTER_TO_INT(g_object_get_data(G_OBJECT(w), "action"))); }), this);
	return w;
}
LibraryUI::LibraryUI(GtkApplication *app, LibraryModel &model) : model_(model)
{
	window_ = GTK_WINDOW(gtk_application_window_new(app));
	g_object_ref(window_);
	gtk_window_set_title(window_, "Module library");
	gtk_window_set_default_size(window_, 900, 740);
	gtk_window_set_hide_on_close(window_, TRUE);
	auto *outer = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
	gtk_window_set_child(window_, outer);
	auto *tabs = gtk_notebook_new();
	body_ = tabs;
	gtk_widget_set_vexpand(tabs, TRUE);
	gtk_box_append(GTK_BOX(outer), tabs);
	auto page = [&](const char *name) {auto*scroll=gtk_scrolled_window_new();auto*box=gtk_box_new(GTK_ORIENTATION_VERTICAL,5);gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll),box);gtk_notebook_append_page(GTK_NOTEBOOK(tabs),scroll,gtk_label_new(name));return box; };
	auto *local = page("Modules");
	auto *bar = row(local);
	button(bar, "Installed modules", 0);
	root_ = entry(local, "Local SWORD source root");
	bar = row(local);
	button(bar, "Inspect local catalog", 1);
	button(bar, "Review local install", 2);
	button(bar, "Install reviewed module", 3);
	replace_ = check(local, "Confirm replacement when required");
	filter_ = entry(local, "Filter modules by name, language or type");
	bar = row(local);
	only_favorites_ = check(bar, "Favorites only");
	show_hidden_ = check(bar, "Show hidden");
	for (auto *w : {filter_, only_favorites_, show_hidden_})
		g_signal_connect(w, w == filter_ ? "changed" : "toggled", G_CALLBACK(+[](GtkWidget *, gpointer p) {auto*s=static_cast<LibraryUI*>(p);if(!s->updating_)s->show_items(); }), this);
	rows_ = gtk_list_box_new();
	gtk_list_box_set_selection_mode(GTK_LIST_BOX(rows_), GTK_SELECTION_SINGLE);
	gtk_box_append(GTK_BOX(local), rows_);
	g_signal_connect(rows_, "row-selected", G_CALLBACK(+[](GtkListBox *, GtkListBoxRow *, gpointer p) {auto*s=static_cast<LibraryUI*>(p);if(!s->updating_)s->select_item(); }), this);
	module_ = entry(local, "Exact module ID");
	review_ = gtk_label_new("");
	gtk_label_set_wrap(GTK_LABEL(review_), TRUE);
	gtk_box_append(GTK_BOX(local), review_);
	auto *remote = page("HTTPS sources");
	profile_ = entry(remote, "Sources profile file");
	bar = row(remote);
	button(bar, "Open sources", 4);
	button(bar, "New sources", 5);
	button(bar, "Save sources and cache", 6);
	sources_ = gtk_list_box_new();
	gtk_box_append(GTK_BOX(remote), sources_);
	g_signal_connect(sources_, "row-selected", G_CALLBACK(+[](GtkListBox *, GtkListBoxRow *, gpointer p) {auto*s=static_cast<LibraryUI*>(p);if(!s->updating_)s->select_source(); }), this);
	id_ = entry(remote, "Source ID");
	name_ = entry(remote, "Source name");
	host_ = entry(remote, "HTTPS hostname");
	catalog_ = entry(remote, "Catalog directory");
	packages_ = entry(remote, "Package directory");
	port_ = gtk_spin_button_new_with_range(1, 65535, 1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(port_), 443);
	gtk_box_append(GTK_BOX(remote), port_);
	enabled_ = check(remote, "Enable reviewed source");
	position_ = gtk_spin_button_new_with_range(0, 255, 1);
	gtk_box_append(GTK_BOX(remote), position_);
	bar = row(remote);
	button(bar, "Add/update source", 7);
	button(bar, "Remove source", 8);
	button(bar, "Move source", 9);
	bar = row(remote);
	button(bar, "Use cached catalog", 10);
	button(bar, "Fetch HTTPS catalog", 11);
	button(bar, "Install selected remote module", 12);
	workspace_ = entry(remote, "Existing transfer workspace outside module root");
	ca_ = entry(remote, "Optional custom CA certificate bundle (empty uses system trust)");
	ack_ = check(remote, "I acknowledge remote access to the selected HTTPS server");
	legacy_path_ = entry(remote, "Legacy InstallMgr.conf path");
	button(remote, "Review legacy sources", 22);
	legacy_rows_ = gtk_drop_down_new(nullptr, nullptr);
	gtk_box_append(GTK_BOX(remote), legacy_rows_);
	button(remote, "Accept selected legacy source disabled", 23);
	source_remove_ = check(remote, "Confirm source removal");
	auto *maintenance = page("Maintenance");
	gtk_box_append(GTK_BOX(maintenance), gtk_label_new("Select an installed module in Modules, then review it here."));
	button(maintenance, "Review module maintenance", 13);
	maintenance_ = gtk_label_new("");
	gtk_label_set_wrap(GTK_LABEL(maintenance_), TRUE);
	gtk_box_append(GTK_BOX(maintenance), maintenance_);
	cipher_ = entry(maintenance, "SWORD cipher configuration key");
	gtk_entry_set_visibility(GTK_ENTRY(cipher_), FALSE);
	confirm_ = check(maintenance, "Confirm the selected maintenance action");
	bar = row(maintenance);
	button(bar, "Remove module", 14);
	button(bar, "Set cipher key", 15);
	button(bar, "Build search index", 16);
	button(bar, "Delete search index", 17);
	auto *organization = page("Organization");
	organization_ = entry(organization, "Organization profile file");
	bar = row(organization);
	button(bar, "Open organization", 18);
	button(bar, "New organization", 19);
	provider_ = entry(organization, "Provider ID");
	item_ = entry(organization, "Module/item ID");
	group_ = entry(organization, "Group");
	favorite_ = check(organization, "Favorite");
	hidden_ = check(organization, "Hidden in library list");
	button(organization, "Save item organization", 20);
	auto *migration = page("Legacy migration");
	auto *instructions = gtk_label_new("Choose legacy files explicitly. Review mappings and issues, then write a new backup package or session. Original files stay in place.");
	gtk_label_set_wrap(GTK_LABEL(instructions), TRUE);
	gtk_box_append(GTK_BOX(migration), instructions);
	settings_ = entry(migration, "Legacy settings file (optional)");
	tabs_ = entry(migration, "Legacy tabs file (optional)");
	options_ = entry(migration, "Legacy module options file (optional)");
	bar = row(migration);
	button(bar, "Review selected files", 24);
	package_ = entry(migration, "Migration backup package to read");
	button(migration, "Read backup package", 25);
	map_from_ = entry(migration, "Unavailable legacy module ID");
	map_to_ = entry(migration, "Installed replacement module ID");
	bar = row(migration);
	button(bar, "Add module mapping", 26);
	button(bar, "Clear mappings", 27);
	mapping_review_ = gtk_label_new("");
	gtk_label_set_wrap(GTK_LABEL(mapping_review_), TRUE);
	gtk_box_append(GTK_BOX(migration), mapping_review_);
	migration_review_ = gtk_label_new("");
	gtk_label_set_selectable(GTK_LABEL(migration_review_), TRUE);
	gtk_label_set_wrap(GTK_LABEL(migration_review_), TRUE);
	gtk_box_append(GTK_BOX(migration), migration_review_);
	destination_ = entry(migration, "New output file or backup folder");
	bar = row(migration);
	button(bar, "Save backup package", 28);
	button(bar, "Export native session", 29);
	button(bar, "Restore exact sources to new folder", 30);
	assets_source_ = entry(migration, "Legacy assets directory to inspect");
	button(migration, "Review assets", 31);
	assets_review_ = gtk_label_new("");
	gtk_label_set_selectable(GTK_LABEL(assets_review_), TRUE);
	gtk_label_set_wrap(GTK_LABEL(assets_review_), TRUE);
	gtk_box_append(GTK_BOX(migration), assets_review_);
	assets_destination_ = entry(migration, "New assets copy destination");
	button(migration, "Copy reviewed assets", 32);
	status_ = gtk_label_new("");
	gtk_label_set_wrap(GTK_LABEL(status_), TRUE);
	gtk_box_append(GTK_BOX(outer), status_);
	progress_ = gtk_label_new("");
	gtk_box_append(GTK_BOX(outer), progress_);
	cancel_ = button(outer, "Cancel current transfer", 21);
	model_.catalog();
}
LibraryUI::~LibraryUI()
{
	gtk_window_destroy(window_);
	g_object_unref(window_);
}
void LibraryUI::present() { gtk_window_present(window_); }
void LibraryUI::select_item()
{
	auto *row = gtk_list_box_get_selected_row(GTK_LIST_BOX(rows_));
	if (!row)
		return;
	auto i = gtk_list_box_row_get_index(row);
	if (i < 0 || size_t(i) >= visible_.size())
		return;
	const auto &item = model_.items()[visible_[i]];
	gtk_editable_set_text(GTK_EDITABLE(module_), item.id.c_str());
	gtk_editable_set_text(GTK_EDITABLE(provider_), item.provider.c_str());
	gtk_editable_set_text(GTK_EDITABLE(item_), item.id.c_str());
	gtk_editable_set_text(GTK_EDITABLE(group_), "");
	gtk_check_button_set_active(GTK_CHECK_BUTTON(favorite_), FALSE);
	gtk_check_button_set_active(GTK_CHECK_BUTTON(hidden_), FALSE);
	for (const auto &g : model_.groups())
		if (g.provider == item.provider && g.item == item.id) {
			gtk_editable_set_text(GTK_EDITABLE(group_), g.group.c_str());
			gtk_check_button_set_active(GTK_CHECK_BUTTON(favorite_), g.favorite);
			gtk_check_button_set_active(GTK_CHECK_BUTTON(hidden_), g.hidden);
		}
}
void LibraryUI::select_source()
{
	auto *row = gtk_list_box_get_selected_row(GTK_LIST_BOX(sources_));
	if (!row)
		return;
	auto i = gtk_list_box_row_get_index(row);
	if (i < 0 || size_t(i) >= model_.endpoints().size())
		return;
	const auto &e = model_.endpoints()[i];
	for (auto pair : {std::pair<GtkWidget *, const std::string &>{id_, e.id}, {name_, e.name}, {host_, e.host}, {catalog_, e.catalog}, {packages_, e.packages}})
		gtk_editable_set_text(GTK_EDITABLE(pair.first), pair.second.c_str());
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(port_), e.port);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(position_), i);
	gtk_check_button_set_active(GTK_CHECK_BUTTON(enabled_), e.enabled);
}
void LibraryUI::show_items()
{
	const bool updating = updating_;
	updating_ = true;
	while (auto *w = gtk_widget_get_first_child(rows_))
		gtk_list_box_remove(GTK_LIST_BOX(rows_), w);
	visible_.clear();
	const auto filter = text(filter_);
	for (size_t i = 0; i < model_.items().size(); ++i) {
		const auto &item = model_.items()[i];
		bool favorite = false, hidden = false;
		std::string group;
		for (const auto &g : model_.groups())
			if (g.provider == item.provider && g.item == item.id) {
				favorite = g.favorite;
				hidden = g.hidden;
				group = g.group;
			}
		if ((hidden && !active(show_hidden_)) || (active(only_favorites_) && !favorite))
			continue;
		const auto label = item.id + " · " + item.title + " · " + item.language + " · " + item.type + " · " + item.version + " · " + group + "\n" + item.license + " [access " + std::to_string(item.access) + "]";
		auto *haystack = g_utf8_casefold(label.c_str(), -1);
		auto *needle = g_utf8_casefold(filter.c_str(), -1);
		bool match = std::string(haystack).find(needle) != std::string::npos;
		g_free(haystack);
		g_free(needle);
		if (!match)
			continue;
		auto *w = gtk_label_new(label.c_str());
		gtk_label_set_wrap(GTK_LABEL(w), TRUE);
		gtk_label_set_xalign(GTK_LABEL(w), 0);
		gtk_list_box_append(GTK_LIST_BOX(rows_), w);
		visible_.push_back(i);
	}
	updating_ = updating;
}
void LibraryUI::action(int a)
{
	if (a == 21) {
		model_.cancel();
		return;
	}
	if (model_.busy())
		return;
	switch (a) {
	case 0:
		model_.catalog();
		break;
	case 1:
		model_.catalog(text(root_));
		break;
	case 2:
		model_.prepare_install(text(root_), text(module_));
		break;
	case 3:
		model_.install(active(replace_));
		break;
	case 4:
	case 5:
		model_.repositories(text(profile_), a == 5);
		break;
	case 6:
		model_.save_repositories();
		break;
	case 7:
		model_.put_repository({text(id_), text(host_), text(catalog_), text(packages_), text(name_), unsigned(gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(port_))), active(enabled_)});
		break;
	case 8:
		if (active(source_remove_))
			model_.remove_repository(text(id_));
		break;
	case 9:
		model_.move_repository(text(id_), gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(position_)));
		break;
	case 10: {
		auto *row = gtk_list_box_get_selected_row(GTK_LIST_BOX(sources_));
		if (row)
			model_.use_cached(gtk_list_box_row_get_index(row));
		break;
	}
	case 11:
		model_.fetch(text(id_), text(workspace_), active(ack_), text(ca_));
		break;
	case 12:
		model_.install_remote(text(module_), text(workspace_), active(replace_), active(ack_), text(ca_));
		break;
	case 13:
		model_.review_maintenance(text(module_));
		break;
	case 14:
	case 15:
	case 16:
	case 17:
		if (active(confirm_)) {
			model_.maintain(a - 14, a == 15 ? text(cipher_) : "");
			if (a == 15)
				gtk_editable_set_text(GTK_EDITABLE(cipher_), "");
			gtk_check_button_set_active(GTK_CHECK_BUTTON(confirm_), FALSE);
		}
		break;
	case 22:
		model_.preview_legacy_sources(text(legacy_path_));
		break;
	case 23:
		model_.accept_legacy_source(gtk_drop_down_get_selected(GTK_DROP_DOWN(legacy_rows_)), text(id_), text(packages_));
		break;
	case 24:
		model_.prepare_migration(text(settings_), text(tabs_), text(options_), mapping_);
		break;
	case 25:
		model_.read_migration(text(package_), mapping_);
		break;
	case 26:
		if (!text(map_from_).empty() && !text(map_to_).empty())
			mapping_.push_back({text(map_from_), text(map_to_)});
		break;
	case 27:
		mapping_.clear();
		break;
	case 28:
	case 29:
	case 30:
		model_.write_migration(text(destination_), a - 28);
		break;
	case 31:
		model_.inspect_assets(text(assets_source_));
		break;
	case 32:
		model_.copy_assets(text(assets_destination_));
		break;
	case 18:
	case 19:
		model_.organization(text(organization_), a == 19);
		break;
	case 20:
		model_.organize({text(provider_), text(item_), text(group_), active(favorite_), active(hidden_)});
		break;
	}
	std::string mappings;
	for (const auto &m : mapping_)
		mappings += m.first + " → " + m.second + "\n";
	gtk_label_set_text(GTK_LABEL(mapping_review_), mappings.c_str());
	tick();
}
void LibraryUI::tick()
{
	model_.tick();
	gtk_widget_set_sensitive(body_, !model_.busy());
	gtk_widget_set_sensitive(cancel_, model_.busy());
	const auto &p = model_.progress();
	gtk_label_set_text(GTK_LABEL(progress_), model_.busy() ? ("Phase " + std::to_string(p.phase) + " · " + std::to_string(p.downloaded) + " / " + std::to_string(p.download_total) + " bytes · " + std::to_string(p.entries) + " entries").c_str() : "");
	if (version_ == model_.version())
		return;
	version_ = model_.version();
	updating_ = true;
	show_items();
	while (auto *w = gtk_widget_get_first_child(sources_))
		gtk_list_box_remove(GTK_LIST_BOX(sources_), w);
	for (const auto &e : model_.endpoints()) {
		auto *w = gtk_label_new((e.name + " · " + e.host + (e.enabled ? " · enabled" : " · disabled") + (e.cached ? " · cached" : "")).c_str());
		gtk_label_set_xalign(GTK_LABEL(w), 0);
		gtk_list_box_append(GTK_LIST_BOX(sources_), w);
	}
	const auto &i = model_.install_review();
	gtk_label_set_text(GTK_LABEL(review_), (i.module.empty() ? "" : i.module + " · source " + i.source_version + " · installed " + i.installed_version + " · decision " + std::to_string(i.decision)).c_str());
	const auto &m = model_.maintenance();
	gtk_label_set_text(GTK_LABEL(maintenance_), (m.module.empty() ? "" : m.module + (m.indexed ? " · indexed" : " · no index") + (m.cipher ? " · cipher declared" : "")).c_str());
	auto *legacy_choices = gtk_string_list_new(nullptr);
	for (const auto &s : model_.legacy_sources())
		gtk_string_list_append(legacy_choices, s.c_str());
	gtk_drop_down_set_model(GTK_DROP_DOWN(legacy_rows_), G_LIST_MODEL(legacy_choices));
	g_object_unref(legacy_choices);
	gtk_label_set_text(GTK_LABEL(migration_review_), model_.migration_review().c_str());
	gtk_label_set_text(GTK_LABEL(assets_review_), model_.assets_review().c_str());
	gtk_label_set_text(GTK_LABEL(status_), model_.error().empty() ? model_.notice().c_str() : model_.error().c_str());
	updating_ = false;
}
} // namespace xiphos::linux_preview
