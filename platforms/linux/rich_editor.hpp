// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "xiphos/core/c_api.h"
#include <functional>
#include <gtk/gtk.h>
#include <string>
#include <vector>
namespace xiphos::linux_preview
{
// GTK presentation of core-owned runs. Parsing, normalization and formatting
// stay in the shared rich document; positions cross that boundary as UTF-8 bytes.
class RichEditor
{
      public:
	explicit RichEditor(std::function<bool(const std::string &)> changed);
	~RichEditor();
	GtkWidget *widget() const { return view_; }
	bool load(const std::string &html);
	std::pair<size_t, size_t> selection() const;

      private:
	struct Style {
		unsigned flags = 0;
		int block = 0;
		std::string foreground, background, link;
	};
	GtkWidget *view_ = nullptr;
	bool updating_ = false;
	std::string html_;
	std::vector<GtkTextTag *> tags_;
	std::function<bool(const std::string &)> changed_;
	void publish();
};
} // namespace xiphos::linux_preview
