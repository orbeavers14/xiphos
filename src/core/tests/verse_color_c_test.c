/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "xiphos/core/verse_color.h"
int verse_color_c_smoke(void) {
 return xh_choose_verse_color(1,1,1,1,1)==XH_VERSE_COLOR_ANNOTATION &&
        xh_choose_verse_color(0,1,1,0,1)==XH_VERSE_COLOR_CURRENT_HIGHLIGHT &&
        xh_choose_verse_color(0,0,1,0,0)==XH_VERSE_COLOR_CURRENT_TEXT &&
        xh_choose_verse_color(0,0,1,1,1)==XH_VERSE_COLOR_NONE;
}
