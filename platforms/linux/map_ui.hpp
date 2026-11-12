// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <gtk/gtk.h>
#include <filesystem>

namespace xiphos::linux_preview {
void open_map(GtkWindow *parent, bool software, const std::filesystem::path &assets,
              const std::filesystem::path &pack);
}
