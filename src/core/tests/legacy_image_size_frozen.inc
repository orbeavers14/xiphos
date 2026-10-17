// SPDX-License-Identifier: GPL-2.0-or-later
// Frozen arithmetic from src/gtk/utilities.c AnalyzeForImageSize, master e193cf6.
// Original Xiphos Developer Team GPL-2.0-or-later code, retained for independent
// comparison after production delegates its geometry to the shared helper.
xc_image_size legacy_bounds(int window_x,int window_y,int columns) {
			/* in a world of multi-column output, */
			/* we must constrain by column width. */
			if (columns != 1) {
				window_x /= columns;
				window_x = (int)((float)window_x * 0.85);
			}

			if ((window_x > 200) || (window_y > 200)) {
				window_x -= 23;
				window_y -= 23;
			} else {
				window_x = (window_x * 93) / 100;
				window_y = (window_y * 93) / 100;
			}
 return {window_x,window_y};
}
xc_image_size legacy_fit(int image_x,int image_y,int window_x,int window_y) {
		// knowing image size & window size, adjust to fit.
		if (image_x > window_x) {
			float proportion =
			    (float)window_x / (float)image_x;
			image_x = window_x;
			image_y = (int)((float)image_y * proportion);
		}
		if (image_y > window_y) {
			float proportion =
			    (float)window_y / (float)image_y;
			image_y = window_y;
			image_x = (int)((float)image_x * proportion);
		}
 return {image_x,image_y};
}
