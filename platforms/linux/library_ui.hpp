// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "library_model.hpp"
#include <gtk/gtk.h>
namespace xiphos::linux_preview
{
class LibraryUI
{
      public:
	LibraryUI(GtkApplication *app, LibraryModel &model);
	~LibraryUI();
	void present();
	void tick();

      private:
	LibraryModel &model_;
	GtkWindow *window_ = nullptr;
	GtkWidget *body_ = nullptr, *status_ = nullptr, *rows_ = nullptr, *filter_ = nullptr, *progress_ = nullptr, *cancel_ = nullptr;
	GtkWidget *new_module_ = nullptr, *archive_path_ = nullptr;
	GtkWidget *root_ = nullptr, *module_ = nullptr, *replace_ = nullptr, *review_ = nullptr, *cipher_ = nullptr, *maintenance_ = nullptr, *confirm_ = nullptr;
	GtkWidget *profile_ = nullptr, *sources_ = nullptr, *id_ = nullptr, *host_ = nullptr, *catalog_ = nullptr, *packages_ = nullptr, *name_ = nullptr, *port_ = nullptr, *enabled_ = nullptr, *position_ = nullptr, *workspace_ = nullptr, *ca_ = nullptr, *ack_ = nullptr;
	GtkWidget *organization_ = nullptr, *provider_ = nullptr, *item_ = nullptr, *group_ = nullptr, *favorite_ = nullptr, *hidden_ = nullptr, *only_favorites_ = nullptr, *show_hidden_ = nullptr;
	GtkWidget *legacy_path_ = nullptr, *legacy_rows_ = nullptr, *source_remove_ = nullptr;
	GtkWidget *settings_ = nullptr, *tabs_ = nullptr, *options_ = nullptr, *package_ = nullptr, *destination_ = nullptr, *migration_review_ = nullptr, *assets_source_ = nullptr, *assets_destination_ = nullptr, *assets_review_ = nullptr, *map_from_ = nullptr, *map_to_ = nullptr, *mapping_review_ = nullptr;
	std::vector<std::pair<std::string, std::string>> mapping_;
	std::vector<size_t> visible_;
	unsigned version_ = 0;
	bool updating_ = false;
	void select_item();
	void select_source();
	void show_items();
	GtkWidget *entry(GtkWidget *box, const char *label);
	GtkWidget *button(GtkWidget *box, const char *label, int action);
	void action(int action);
};
} // namespace xiphos::linux_preview
