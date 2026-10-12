/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "xiphos/core/note_presentation.h"
#include <stdio.h>
#include <stddef.h>
int main(void) {
  struct Case { const char *type; int clicked, list; XiphosNotePresentation expected; };
  const struct Case cases[]={
    {NULL,0,1,XIPHOS_NOTE_NO_PRESENTATION}, {NULL,1,1,XIPHOS_NOTE_NO_PRESENTATION},
    {"",0,1,XIPHOS_NOTE_NO_PRESENTATION}, {"unknown",1,1,XIPHOS_NOTE_NO_PRESENTATION},
    {"n",0,0,XIPHOS_NOTE_HOVER_BODY}, {"n",0,1,XIPHOS_NOTE_HOVER_BODY},
    {"n",1,0,XIPHOS_NOTE_NO_PRESENTATION}, {"n",1,1,XIPHOS_NOTE_NO_PRESENTATION},
    {"x",0,0,XIPHOS_NOTE_HOVER_REFERENCE_LINKS}, {"x",0,1,XIPHOS_NOTE_HOVER_REFERENCES},
    {"x",1,0,XIPHOS_NOTE_CLICK_REFERENCES}, {"x",1,1,XIPHOS_NOTE_CLICK_REFERENCES},
    {"nx",0,0,XIPHOS_NOTE_HOVER_BODY}, {"xn",0,1,XIPHOS_NOTE_HOVER_BODY},
    {"nx",1,0,XIPHOS_NOTE_CLICK_REFERENCES}, {"xn",1,1,XIPHOS_NOTE_CLICK_REFERENCES},
    {"X",0,1,XIPHOS_NOTE_NO_PRESENTATION}, {"αn",0,-1,XIPHOS_NOTE_HOVER_BODY},
    {"x",-1,0,XIPHOS_NOTE_CLICK_REFERENCES}, {"x",0,-1,XIPHOS_NOTE_HOVER_REFERENCES}
  };
  for(size_t i=0;i<sizeof cases/sizeof cases[0];++i) {
    if(xiphos_core_note_presentation(cases[i].type,cases[i].clicked,cases[i].list)!=cases[i].expected) {
      fprintf(stderr,"C11 note presentation case %zu failed\n",i); return 1;
    }
  }
  puts("C11 note presentation boundary/precedence checks passed"); return 0;
}
