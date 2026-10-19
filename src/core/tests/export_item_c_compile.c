/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Compile actual retained C adapters without desktop dependencies. Runtime byte
 * and SDK-call comparisons use export_item_test; this object is compile-only. */
#include <glib.h>
#include <string.h>
#include "xiphos/core/export_item.h"
enum { TEXT_TYPE=0, COMMENTARY_TYPE=1 };
extern struct ExportSettings { const char *MainWindowModule; } settings;
extern gboolean main_is_Bible_key(const gchar *,const gchar *);
extern gint main_get_mod_type(const gchar *);
extern gchar *main_get_rendered_text(const gchar *,const gchar *);
extern gchar *main_get_striptext(const gchar *,gchar *);
#include "legacy_export_items_actual.inc"
