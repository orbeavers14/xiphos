if (!strcmp(action, "showImage")) {
			if (HAS_URL_PARAM(svalue)) {
				const gchar *img_path = (!strncmp(svalue, "file:", 5)
							 ? svalue + 5
							 : svalue);
				gboolean is_image = (g_str_has_suffix(img_path, ".png") ||
						     g_str_has_suffix(img_path, ".jpg") ||
						     g_str_has_suffix(img_path, ".jpeg") ||
						     g_str_has_suffix(img_path, ".gif") ||
						     g_str_has_suffix(img_path, ".bmp") ||
						     g_str_has_suffix(img_path, ".svg") ||
						     g_str_has_suffix(img_path, ".tiff") ||
						     g_str_has_suffix(img_path, ".webp"));
				if (is_image) {
					show_separate_image(img_path, clicked);
					retval = 1;
				}
			}
		}
