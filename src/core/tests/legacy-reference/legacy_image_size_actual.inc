#define IMGSRC_STRING "<img src=\""
#define IMGSRC_LENGTH 10 // strlen(IMGSRC_STRING)
#ifndef HAVE_STRCASESTR
const char *strcasestr(const char *haystack, const char *needle);
#endif

const char *AnalyzeForImageSize(const char *origtext, int columns, GdkWindow *window)
{
	static GString *resized;
	static gint resized_init = FALSE;

	const char *trail; // "trail" trails behind ...
	char *path;	// ... the current "path".
	char *end;	 // "end" is the path's end.
	char buf[64];      // two full signed-int dimensions plus attribute text.
	gint image_x, image_y, window_x = -999, window_y = -999;
	int image_retval;
	gboolean warned_once = FALSE;

	if (!resized_init) {
		resized = g_string_new("");
		resized_init = TRUE;
	}
	// performance tweak:
	// image content is by no means common. therefore, spend an extra
	// search call to determine whether any of the rest is needed,
	// most especially to stop copying large blocks of text w/no images.
	if ((path = (char *)strcasestr(origtext, IMGSRC_STRING)) == NULL)
		return origtext;

	for (resized = g_string_assign(resized, ""), trail = origtext
						     /* and path was initialized just above */;
	     path; path = (char *)strcasestr(path, IMGSRC_STRING)) {

		if (window_y == -999) {
/* we have images, but we don't know bounds yet */

#ifdef USE_GTK_3
			window_x = gdk_window_get_width(window);
			window_y = gdk_window_get_height(window);
#else
			gdk_drawable_get_size(window, &window_x,
					      &window_y);
#endif

            xc_image_size bounds;
            if (xc_image_viewport_size(window_x, window_y, columns, &bounds) != 0)
                return origtext; // No usable viewport; retain unresized content.
            window_x = bounds.width;
            window_y = bounds.height;
		}

		/* add the working segment, with annotation added */
		/* to keep us from matching IMGSRC_STRING again. */
		path += IMGSRC_LENGTH;
		resized =
		    g_string_append_len(resized, trail, path - trail - 5);
		resized = g_string_append(resized, "resized=\"yes\" ");
		resized = g_string_append_len(resized, path - 5, 5);

		// some modules play fast-n-loose with proper file spec.
		if (strncmp(path, "file://", 7) == 0) {
#ifdef WIN32
			/* due to need for local soup server */
			path += 5;
			resized =
			    g_string_append(resized,
					    "http://127.0.0.1:7878/");
#else
			path += 7;
			resized = g_string_append(resized, "file://");
#endif
		} else if (strncmp(path, "file:", 5) == 0) {
			path += 5;
#ifdef WIN32
			resized =
			    g_string_append(resized,
					    "http://127.0.0.1:7878/");
#else
			resized = g_string_append(resized, "file:");
#endif
		}
		// else we have an odd case of a src="..." which does not
		// begin with "file:"...which may become the WIN32 norm because
		// webkit is broken as of 2012 jan 12.
		// we will just take "path" as it stands.

		// getting this far means we have a valid img src and file.
		// find closing '"' to determine pathname end.
		if ((end = strchr(path, '"')) == 0)
			continue;

		*end = '\0';
#ifdef WIN32
		resized =
		    g_string_append(resized,
				    g_strdelimit(path, "\\", '/'));
#else
		resized = g_string_append(resized, path);
#endif
		image_retval = ImageDimensions(path, &image_x, &image_y);
		*end = '"';

		resized = g_string_append_c(resized, '"');
		path = end + 1;
		trail = path;

		if (image_retval != 0) {
			if (!warned_once) {
				gui_generic_warning(_("An image file's size could not be determined.\n"
						      "Xiphos cannot resize images to fit window."));
				warned_once = TRUE;
			}
			continue;
		}
        xc_image_size fitted;
        if (xc_fit_image_size(image_x, image_y, window_x, window_y, &fitted) != 0)
            continue; // Invalid decoder geometry leaves this image unresized.
        image_x = fitted.width;
        image_y = fitted.height;
		g_snprintf(buf, sizeof(buf), " width=\"%d\" height=\"%d\"", image_x,
			image_y);
		resized = g_string_append(resized, buf);
	}

	resized = g_string_append(resized, trail); // remainder of text appended.
	return resized->str;
}

