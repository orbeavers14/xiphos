// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/annotations.hpp"
#include <glib.h>
#include <libxml/parser.h>
#include <cstring>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <limits>
#include <stdexcept>
#ifdef _WIN32
#define strcasecmp _stricmp
#else
#include <strings.h>
#endif
using namespace xiphos::core;
#define _(text) (text)
static std::vector<std::string> warnings;
static void gui_generic_warning(const char *text){warnings.push_back(text);}
static const char *main_get_osisref_from_key(const char *,const char *key){return key;}
struct marked_element{gchar *module,*book;int chapter_verse;GString *annotation;};
using MC=std::map<int,marked_element *>;
namespace frozen {
static xmlDocPtr xml_settings_doc;static xmlNodePtr section_ptr;static MC marked_cache;
static gchar *marked_cache_modname=nullptr,*marked_cache_book=nullptr;
#include "legacy_annotation_xml.inc"
#include "legacy_annotation_cache.inc"
}
namespace actual {
static xmlDocPtr xml_settings_doc;static xmlNodePtr section_ptr;static MC marked_cache;
static gchar *marked_cache_modname=nullptr,*marked_cache_book=nullptr;
#include "legacy_annotation_xml_actual.inc"
#include "legacy_annotation_cache_actual.inc"
}
static void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
template<class F>static void rejects(F f){try{f();}catch(const std::invalid_argument &){return;}throw std::runtime_error("Expected annotation rejection");}
struct Cached{std::map<int,std::string> notes;std::vector<std::string> warnings;bool operator==(const Cached &other)const{return notes==other.notes&&warnings==other.warnings;}};
static Cached run(const std::string &source,const std::string &module,const std::string &book,bool current){
 auto &doc=current?actual::xml_settings_doc:frozen::xml_settings_doc;auto &cache=current?actual::marked_cache:frozen::marked_cache;
 doc=xmlReadMemory(source.data(),static_cast<int>(source.size()),nullptr,nullptr,XML_PARSE_NONET);check(doc,"Retained annotation fixture parses");warnings.clear();std::string key=book+".1.1";
 if(current)actual::markedCacheFill(module.c_str(),key.data());else frozen::markedCacheFill(module.c_str(),key.data());
 Cached result;result.warnings=warnings;for(const auto &entry:cache){result.notes[entry.first]=entry.second->annotation->str;check((current?actual::markedCacheCheck(entry.first):frozen::markedCacheCheck(entry.first))==entry.second,"Complete retained cache lookup");}
 xmlFreeDoc(doc);doc=nullptr;return result;
}
static std::map<int,std::string> copied(const AnnotationCache &cache){std::map<int,std::string> result;for(const auto &entry:cache.entries)result[entry.packed]=entry.html;return result;}
int main(int argc,char **argv){try{
 check(argc==2,"Disposable annotations root required");const auto root=std::filesystem::u8path(argv[1]);std::filesystem::create_directories(root);std::size_t comparisons=0;
 const std::string first="<markedverse label='Text Gen.1.1' list='first'/>",generic="<markedverse label=' Gen.1.1' list='generic'/>",other="<markedverse label='Other Gen.1.1' list='other'/>",escaped="<markedverse label='text GEN.2.3' list='&amp; &lt; &gt; &quot; &apos; &#10;文字'/>";
 for(const auto &body:std::vector<std::string>{first,first+generic,generic+first,first+other,first+escaped,first+"<markedverse label='Text Gen.1.1001' list='collision'/><markedverse label='Text Gen.2.1' list='last'/>","<markedverse label='Text Gen. 2tail.3x' list='prefix'/>","<markedverse label='Text Gen.a.b'/>","<markedverse label='Text Gen.1.1' list=''/>",first+"<markedverse label='bad' list='malformed'/>"+escaped,first+"<unknown/>"+escaped,"\n"+first,first+"\n"+escaped,"<unknown label='Text Gen.1.1' list='accepted by cursor'/>"})
 for(const auto *module:{"Text","TEXT","Other"})for(const auto *book:{"Gen","gen","John"}){
  const std::string source="<Xiphos><osisrefmarkedverses>"+body+"</osisrefmarkedverses><osisrefmarkedverses><markedverse label='Text Gen.3.1' list='shadow'/></osisrefmarkedverses><unknown/></Xiphos>";
  const auto expected=run(source,module,book,false);check(run(source,module,book,true)==expected,"Complete frozen/current annotation cache/cursor traces agree");const auto profile=parse_annotations(source);const auto cache=legacy_annotation_cache(profile,module,book);
  check(copied(cache)==expected.notes&&profile.shadowed_sections==1,"Owned profile/cache matches complete retained duplicate/generic/module/book/cursor semantics");check(cache.malformed_record.has_value()==!expected.warnings.empty(),"Malformed annotation label returns owned prefix diagnostic");++comparisons;
 }
 const auto plain=decode_annotation_label("Text Gen. 2tail.3x");check(plain&&plain->module=="Text"&&plain->book=="Gen"&&plain->chapter==2&&plain->verse==3&&plain->packed==2003,"Bounded atoi prefix and raw module/book decoding");
 for(const auto *label:{"bad","Text Gen.1","Text.Gen.1.1","Text Gen.2147483648.1","Text Gen.2147483647.1","Text Gen.-2147483648.0","Text Gen.2147484.-353"})check(!decode_annotation_label(label),"Malformed/unsafe annotation labels do not execute undefined integer behavior");
 check(annotation_note_html("&<>\n\"'文字")=="&amp;&lt;&gt;<br />&quot;&apos;文字","Native plain note HTML exactly matches retained six replacements");
 const auto barrier=parse_annotations("<Xiphos><osisrefmarkedverses>\n<markedverse label='Text Gen.1.1' list='hidden'/></osisrefmarkedverses></Xiphos>");const auto hidden=legacy_annotation_cache(barrier,"Text","Gen");check(barrier.legacy_stop_child==0&&barrier.records.size()==1&&!barrier.records[0].legacy_visible&&hidden.incomplete&&hidden.entries.empty(),"Whitespace cursor barrier is visible as metadata rather than discarding raw notes");
 const auto trailing=legacy_annotation_cache(parse_annotations("<Xiphos><osisrefmarkedverses><markedverse label='Text Gen.1.1'/>\n</osisrefmarkedverses></Xiphos>"),"Text","Gen");check(!trailing.incomplete&&trailing.entries.size()==1&&trailing.entries[0].html.empty(),"Trailing whitespace without omitted records does not imply lost annotations");
 check(parse_annotations("<Xiphos/>").records.empty(),"Missing annotation section is empty, without writes/defaults");
 for(const auto &bad:std::vector<std::string>{"","<SwordBookmarks/>","<Xiphos>","<!DOCTYPE Xiphos [<!ENTITY x 'hidden'>]><Xiphos/>",std::string("<Xiphos/>\0tail",14)})rejects([&]{parse_annotations(bad);});
 rejects([&]{decode_annotation_label(std::string(4097,'x'));});rejects([&]{annotation_note_html(std::string(maximum_annotation_note_bytes+1,'x'));});
 std::string many="<Xiphos><osisrefmarkedverses>";for(size_t i=0;i<maximum_annotation_records;++i)many+="<markedverse/>";check(parse_annotations(many+"</osisrefmarkedverses></Xiphos>").records.size()==maximum_annotation_records,"Exact annotation record bound");rejects([&]{parse_annotations(many+"<markedverse/></osisrefmarkedverses></Xiphos>");});
 check(decode_annotation_label("Text Gen.2147483.647")->packed==std::numeric_limits<int>::max()&&decode_annotation_label("Text Gen.-2147483.-648")->packed==std::numeric_limits<int>::min(),"Exact safe packed integer boundaries");
 AnnotationProfile expanded;for(int i=1;i<=3;++i){AnnotationRecord record;record.element="markedverse";record.label="Text Gen.1."+std::to_string(i);record.note=std::string(maximum_annotation_note_bytes,'\n');record.legacy_visible=true;expanded.records.push_back(std::move(record));}
 rejects([&]{legacy_annotation_cache(expanded,"Text","Gen");});for(auto &record:expanded.records)record.label="Text Gen.1.1";
 const auto replaced=legacy_annotation_cache(expanded,"Text","Gen");check(replaced.entries.size()==1&&replaced.entries[0].record==2&&replaced.entries[0].html.size()==6*maximum_annotation_note_bytes&&replaced.snapshot_bytes<maximum_annotation_snapshot_bytes,"Duplicate cache replacement accounts retained HTML rather than discarded copies");
 auto invalid_prefix=expanded;invalid_prefix.records[0].legacy_visible=false;rejects([&]{legacy_annotation_cache(invalid_prefix,"Text","Gen");});
 const auto file=root/"annotations.xml";const auto source="<Xiphos><!--keep--><osisrefmarkedverses>"+first+escaped+"</osisrefmarkedverses><other/></Xiphos>";{std::ofstream out(file);out<<source;}const auto permissions=std::filesystem::status(file).permissions();const auto loaded=read_annotations(file.u8string());std::ifstream in(file);const std::string after(std::istreambuf_iterator<char>(in),{});check(loaded.records.size()==2&&after==source&&std::filesystem::status(file).permissions()==permissions,"Explicit annotation read preserves source bytes/permissions");
 std::cout<<"Personal annotations: "<<comparisons<<" complete frozen/current cache/cursor comparisons, raw metadata, prefix diagnostics, escaping and safe bounds passed\n";return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
