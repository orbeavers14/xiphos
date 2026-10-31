// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/verse_color.h"
#include <iostream>
#include <stdexcept>
using gchar=char;
static struct Settings{int annotate_highlight,versehighlight;gchar *highlight_fg,*highlight_bg,*currentverse_color;}settings;
static int color_choices;enum{COLOR_NONE,COLOR_TEXT,COLOR_BOTH};
static gchar *color_chosen_fg,*color_chosen_bg;
static void *e;static gchar *tag_color;
namespace frozen {
void choose(int curChapter,int thisChapter,int curVerse,int k){
#include "legacy_verse_color.inc"
}
}
namespace actual {
void choose(int curChapter,int thisChapter,int curVerse,int k){
#include "legacy_verse_color_actual.inc"
}
}
extern "C" int verse_color_c_smoke(void);
static void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
int main(){try{
 check(verse_color_c_smoke()!=0,"Strict C11 ABI links to shared C++11 policy");size_t cases=0;
 for(int annotation:{-1,0,1,2})for(int has_annotation:{0,1})for(int highlight:{-1,0,1,2})for(int has_tag:{0,1})
 for(int chapter:{1,2})for(int verse:{1,2})for(bool colors:{false,true}){
  gchar foreground[]="foreground",background[]="background",current[]="current",tag[]="tag";
  settings={annotation,highlight,colors?foreground:nullptr,colors?background:nullptr,colors?current:nullptr};e=has_annotation?&settings:nullptr;tag_color=has_tag?tag:nullptr;
  frozen::choose(chapter,1,verse,1);const int choice=color_choices;const auto *fg=color_chosen_fg,*bg=color_chosen_bg;
  actual::choose(chapter,1,verse,1);check(color_choices==choice&&color_chosen_fg==fg&&color_chosen_bg==bg,"Complete retained color block agrees with shared adapter, including null colors and nonzero flags");
  const auto policy=xh_choose_verse_color(annotation,has_annotation,chapter==1&&verse==1,has_tag,highlight);
  check((policy==XH_VERSE_COLOR_ANNOTATION)==(annotation&&has_annotation),"Personal annotation priority coexists with outer tags");
  if(has_tag&&!has_annotation)check(policy==XH_VERSE_COLOR_NONE,"Tag contrast suppresses current-verse override");++cases;
 }
 std::cout<<"Verse colors: "<<cases<<" complete frozen/current priority block comparisons and strict C11/C++11 ABI passed\n";return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
