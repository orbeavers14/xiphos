/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <libxml/parser.h>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define _(value) (value)
static xmlDocPtr xml_settings_doc;
static xmlDocPtr frozen_xml_settings_doc;
#include "legacy_xml_profile.inc"
#include "legacy_xml_profile_actual.inc"
static unsigned checks;
static void compare(const char *section, const char *key, const char *expected) {
  char *actual = xml_get_value(section, key);
  char *frozen = frozen_xml_get_value(section, key);
  assert((actual == NULL) == (frozen == NULL));
  assert((actual == NULL) == (expected == NULL));
  if (actual) { assert(strcmp(actual, frozen) == 0); assert(strcmp(actual, expected) == 0); }
  xmlFree(actual); xmlFree(frozen); ++checks;
}
static void same_document(void) {
  xmlChar *actual, *frozen; int a_size, f_size;
  xmlDocDumpMemory(xml_settings_doc, &actual, &a_size);
  xmlDocDumpMemory(frozen_xml_settings_doc, &frozen, &f_size);
  assert(a_size == f_size); assert(memcmp(actual, frozen, (size_t)a_size) == 0);
  xmlFree(actual); xmlFree(frozen); ++checks;
}
static void load(const char *source) {
  xml_settings_doc = xmlReadMemory(source, (int)strlen(source), "actual.xml", NULL, XML_PARSE_NONET);
  frozen_xml_settings_doc = xmlReadMemory(source, (int)strlen(source), "frozen.xml", NULL, XML_PARSE_NONET);
  assert(xml_settings_doc && frozen_xml_settings_doc);
}
static void unload(void) { xmlFreeDoc(xml_settings_doc); xmlFreeDoc(frozen_xml_settings_doc); }
int main(void) {
  const char *keys[] = {"greek", "hebrew", "greek_morph_nt", "greek_morph_ot", "hebrew_morph"};
  const char *contents[] = {"", "UnavailableModule", "CAFÉ", "Greek &amp; Hebrew", "<![CDATA[<Lexicon>]]>", " ", "<part>child</part>", "&#x130;"};
  const char *decoded[] = {NULL, "UnavailableModule", "CAFÉ", "Greek & Hebrew", "<Lexicon>", " ", NULL, "İ"};
  char input[4096];
  for (size_t k=0; k<5; ++k) for (size_t v=0; v<8; ++v) {
    snprintf(input, sizeof input, "<Xiphos><!--keep--><other attr='preserve'>unrelated</other><lexicons><%s>%s</%s><%s>second</%s><defaultdictionary>Saved</defaultdictionary></lexicons><lexicons><%s>third</%s></lexicons></Xiphos>", keys[k],contents[v],keys[k],keys[k],keys[k],keys[k],keys[k]);
    load(input); compare("lexicons", keys[k], decoded[v]);
    xml_set_value("Xiphos", "lexicons", keys[k], "Saved &amp; &lt;escaped&gt;");
    frozen_xml_set_value("Xiphos", "lexicons", keys[k], "Saved &amp; &lt;escaped&gt;");
    compare("lexicons",keys[k],"Saved & <escaped>"); same_document();
    compare("lexicons","defaultdictionary","Saved"); compare("other","missing",NULL);
    xml_add_new_item_to_section("lexicons", (char *)keys[k], "appended");
    frozen_xml_add_new_item_to_section("lexicons", (char *)keys[k], "appended");
    compare("lexicons",keys[k],"Saved & <escaped>"); same_document(); unload();
  }
  load("<Xiphos><lexicons/><other>untouched</other></Xiphos>");
  xml_set_value("Xiphos","lexicons","greek","ignored");
  frozen_xml_set_value("Xiphos","lexicons","greek","ignored");
  compare("lexicons","greek",NULL); same_document();
  xml_add_new_item_to_section("lexicons","greek","created");
  frozen_xml_add_new_item_to_section("lexicons","greek","created");
  compare("lexicons","greek","created"); same_document(); unload();
  load("<Xiphos><other>untouched</other></Xiphos>");
  xml_add_new_item_to_section("lexicons","greek","ignored");
  frozen_xml_add_new_item_to_section("lexicons","greek","ignored");
  compare("lexicons","greek",NULL); same_document();
  xml_add_new_section_to_settings_doc("lexicons");
  frozen_xml_add_new_section_to_settings_doc("lexicons");
  xml_add_new_item_to_section("lexicons","greek","created");
  frozen_xml_add_new_item_to_section("lexicons","greek","created");
  compare("lexicons","greek","created"); same_document(); unload();
  load("<Xiphos><lexicons/><lexicons><greek>later-section</greek></lexicons></Xiphos>");
  compare("lexicons","greek","later-section");
  xml_add_new_item_to_section("lexicons","greek","first-section");
  frozen_xml_add_new_item_to_section("lexicons","greek","first-section");
  compare("lexicons","greek","first-section"); same_document(); unload();
  load("<Xiphos><lexicons><greek>original</greek></lexicons></Xiphos>");
  xml_set_value("Xiphos","lexicons","greek","Raw & broken");
  frozen_xml_set_value("Xiphos","lexicons","greek","Raw & broken");
  compare("lexicons","greek",NULL); same_document(); unload();
  const char *invalid[]={"<Xiphos>","", "<Xiphos><greek>&missing;</greek></Xiphos>"};
  for(size_t i=0;i<3;++i) {
    xmlDocPtr bad=xmlReadMemory(invalid[i],(int)strlen(invalid[i]),"invalid.xml",NULL,XML_PARSE_NONET|XML_PARSE_NOERROR|XML_PARSE_NOWARNING);
    assert(!bad); ++checks;
  }
  printf("XML profile: %u frozen/actual comparisons and explicit assertions passed\n",checks);
  return 0;
}
