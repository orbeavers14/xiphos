// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/verse_color.h"
extern "C" xh_verse_color xh_choose_verse_color(int annotation_highlight,int has_annotation,
    int current_verse,int has_tag_color,int verse_highlight) {
  if(annotation_highlight&&has_annotation)return XH_VERSE_COLOR_ANNOTATION;
  if(current_verse&&!has_tag_color)
    return verse_highlight?XH_VERSE_COLOR_CURRENT_HIGHLIGHT:XH_VERSE_COLOR_CURRENT_TEXT;
  return XH_VERSE_COLOR_NONE;
}
