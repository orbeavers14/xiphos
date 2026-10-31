/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef XIPHOS_CORE_VERSE_COLOR_H
#define XIPHOS_CORE_VERSE_COLOR_H
#ifdef __cplusplus
extern "C" {
#endif
typedef enum xh_verse_color {
 XH_VERSE_COLOR_NONE=0,
 XH_VERSE_COLOR_ANNOTATION=1, /* Inner fg=highlight_bg, bg=highlight_fg. */
 XH_VERSE_COLOR_CURRENT_HIGHLIGHT=2, /* Inner fg=highlight_fg, bg=highlight_bg. */
 XH_VERSE_COLOR_CURRENT_TEXT=3 /* Inner fg=currentverse_color only. */
} xh_verse_color;
/* Exact retained priority. Annotation highlighting wins inside an optional tag
 * wrapper. Tags suppress current-verse colors, including background highlight.
 * All nonzero input flags are true. No allocation, persistence or presentation. */
xh_verse_color xh_choose_verse_color(int annotation_highlight,int has_annotation,
 int current_verse,int has_tag_color,int verse_highlight);
#ifdef __cplusplus
}
#endif
#endif
