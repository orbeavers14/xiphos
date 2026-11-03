// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/worker.hpp"
#include "xiphos/core/c_api.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <thread>
#include <glib.h>
using namespace xiphos::core;
namespace fs=std::filesystem;
void check(bool ok,const char *s){if(!ok)throw std::runtime_error(s);}
template<class F>void rejects(F f){try{f();}catch(const std::exception &){return;}throw std::runtime_error("Expected migration rejection");}
std::string bytes(const fs::path &p){std::ifstream in(p,std::ios::binary);return {std::istreambuf_iterator<char>(in),{}};}
void write(const fs::path &p,const std::string &s){std::ofstream out(p,std::ios::binary);out<<s;}
std::string settings(){return R"(<?xml version="1.0"?><Xiphos future="keep"><!-- Original Ω settings -->
<modules><bible>Text</bible><comm>Notes</comm><dict>Dictionary</dict><book>CaptureBook</book><parallels>Text,TextAlt</parallels><parallel_set_names>Original</parallel_set_names><parallel_set_Original>TextAlt,Text</parallel_set_Original></modules>
<keys><verse>John 3:16</verse><dictionary>alpha</dictionary><book>/Original</book><offset>4</offset><future>unknown key</future></keys>
<lexicons><greek>Dictionary</greek><hebrew>Dictionary</hebrew><future>retained</future></lexicons>
<misc><showtexts>0</showtexts><showpreview>1</showpreview><showcomms>1</showcomms><showdicts>0</showdicts><xrefsinverselist>0</xrefsinverselist></misc>
<parallel><Strong_s_Numbers>1</Strong_s_Numbers><Headings>0</Headings><Footnotes>0</Footnotes></parallel>
<locale><special>original_TEST.UTF-8</special></locale><layout><width>777</width><hight>555</hight></layout>
<search_mod_lists><item label="Original">Text,TextAlt</item></search_mod_lists>
<future secret="retained attribute"><opaque>Exact original prose</opaque></future><lexicons><greek>shadowed</greek></lexicons></Xiphos>)";}
std::string tabs(){return R"(<Xiphos_Tabs Version="original" unknown="keep"><!-- Original tabs --><tabs><tab text_mod="Text" commentary_mod="Notes" dictlex_mod="Dictionary" book_mod="CaptureBook" text_commentary_key="John 3:16" dictlex_key="alpha" book_offset="/Original" comm_showing="yes" showparallel="yes" future="keep"><opaque/></tab></tabs><unrelated/><tabs><tab text_mod="TextAlt" text_commentary_key="John 3:17" showtexts="yes" showpreview="no" showcomms="no" showdicts="yes"/></tabs></Xiphos_Tabs>)";}
xc_status wait(xc_request *r){auto end=std::chrono::steady_clock::now()+std::chrono::seconds(20);xc_status status;while((status=xc_request_poll(r))==XC_PENDING){check(std::chrono::steady_clock::now()<end,"C migration deadline");std::this_thread::sleep_for(std::chrono::milliseconds(1));}return status;}
int main(int argc,char **argv){try{
 check(argc>=3,"Fixture arguments");auto root=fs::u8path(argv[2]);check(root.filename()=="legacy-migration-fixture","Named fixture only");
 if(argc==4){StudyWorker worker((root/"library").u8string());auto migration=worker.read_legacy_migration((root/"migration.bundle").u8string()).get();check(migration.session&&migration.session->tabs.size()==2,"Fresh-process migration package resolves again");auto restored=worker.restore_native_session(*migration.session).get();check(restored.tabs.size()==2,"Fresh process restores all converted tabs");auto selection=worker.reading_selection(restored.tabs[0].tab).get();check(selection&&selection->module=="Text","Converted selection survives restart");auto current=read_native_session((root/"converted.xml").u8string());check(current.tabs[0].state.module_engine.at("Text").footnotes==false,"Converted module settings survive native file restart");return 0;}
 fs::remove_all(root);fs::create_directories(root);fs::copy(fs::u8path(argv[1]),root/"library",fs::copy_options::recursive);
 const auto config=root/"settings.xml",tabfile=root/"tabs.xml",modops=root/"modops.conf";
 write(config,settings());write(tabfile,tabs());const std::string ini="; Original exact module configuration\n[Text]\nFootnotes=Off\nStrong's Numbers=On\nHeadings=Off\nDoublespace=On\nFutureOption=Keep this\n[Notes]\nCommentary by Chapter=Off\n[NotInstalled]\nFootnotes=On\n";write(modops,ini);
 const LegacyMigrationInput input{config.u8string(),tabfile.u8string(),modops.u8string()};
 {StudyWorker worker((root/"library").u8string());auto live=worker.create_tab().get();(void)worker.visit(live,"Text","John3:18").get();const auto before=worker.reading_selection(live).get();
 auto m=worker.prepare_legacy_migration(input).get();for(const auto &i:m.issues)std::cerr<<i.tab<<' '<<i.module<<' '<<i.reason<<'\n';
 check(m.session&&m.issues.empty()&&m.session->tabs.size()==2,"Complete two-tab conversion");const auto &first=m.session->tabs[0];
 check(!first.visibility.texts&&first.visibility.preview&&first.visibility.commentaries&&!first.visibility.dictionaries&&first.commentary_showing,"Legacy pane-default inheritance and exact yes flag");
 check(m.session->tabs[1].visibility.texts&&!m.session->tabs[1].visibility.preview&&m.session->active_key=="legacy-1","Explicit tab visibility/order and retained first selection");
 check(first.state.history.size()==1&&first.state.dictionary_history.size()==1&&first.state.book&&first.state.book->path=="/Original","Initial histories, dictionary and resolved book identity");
 check(first.state.parallel&&first.state.parallel->options.strongs&&!first.state.parallel->options.footnotes&&!first.state.parallel->headings&&first.state.parallel->options.greekaccents,"Legacy parallel key mapping and defaults");
 check(first.state.module_engine.at("Text").strongs&&!first.state.module_engine.at("Text").footnotes&&first.state.module_engine.count("NotInstalled")&&first.state.presentation.at("Text").double_space,"Engine, presentation and dormant module settings retained");
 check(first.state.lexicons->greek=="Dictionary"&&!first.state.cross_reference_preview_list,"Lexicon and preview preferences mapped");
 check(worker.reading_selection(live).get()->reference==before->reference,"Preview never replaces live tabs");
 auto archive=worker.save_legacy_migration((root/"migration.bundle").u8string(),m).get();check(archive.sources[0].bytes==settings()&&archive.sources[1].bytes==tabs()&&archive.sources[2].bytes==ini&&archive.sources[0].path.empty(),"Package preserves exact original source bytes without machine paths");
 auto saved=worker.export_legacy_migration_session((root/"converted.xml").u8string(),m).get();check(saved.tabs.size()==2,"New native session exported");
 worker.restore_legacy_migration_sources((root/"restored-originals").u8string(),archive).get();check(bytes(root/"restored-originals/settings.xml")==settings()&&bytes(root/"restored-originals/tabs.xml")==tabs()&&bytes(root/"restored-originals/modops.conf")==ini,"Original settings including comments/unknown fields restore byte-for-byte");
 rejects([&]{(void)worker.save_legacy_migration(config.u8string(),m).get();});rejects([&]{(void)worker.save_legacy_migration((root/"migration.bundle").u8string(),m).get();});rejects([&]{(void)worker.export_legacy_migration_session((root/"converted.xml").u8string(),m).get();});rejects([&]{worker.restore_legacy_migration_sources((root/"restored-originals").u8string(),m).get();});
 auto damaged=bytes(root/"migration.bundle");damaged.back()^=1;rejects([&]{parse_legacy_migration(damaged);});rejects([&]{parse_legacy_migration("XiphosMigration:2\n");});
 auto changed=settings();changed+="\n";write(config,changed);rejects([&]{(void)worker.save_legacy_migration((root/"stale.bundle").u8string(),m).get();});check(!fs::exists(root/"stale.bundle"),"Stale source refuses output");write(config,settings());
 auto missing=tabs();missing.replace(missing.find("text_mod=\"Text\""),15,"text_mod=\"Absent\"");write(tabfile,missing);auto blocked=worker.prepare_legacy_migration(input).get();check(!blocked.session&&!blocked.issues.empty(),"Missing module blocks entire session, no partial publish");
 (void)worker.save_legacy_migration((root/"blocked.bundle").u8string(),blocked).get();rejects([&]{(void)worker.export_legacy_migration_session((root/"blocked.xml").u8string(),blocked).get();});auto remapped=worker.prepare_legacy_migration(input,{{"Absent","Text"}}).get();check(remapped.session.has_value(),"Explicit module mapping resolves unavailable identities");write(tabfile,tabs());
 write(root/"empty.xml","<Xiphos_Tabs/>");auto empty=worker.prepare_legacy_migration({config.u8string(),(root/"empty.xml").u8string(),""}).get();check(empty.session&&empty.session->tabs.empty(),"Explicit empty tabs stay empty");auto fallback=worker.prepare_legacy_migration({config.u8string(),"",""}).get();check(fallback.session&&fallback.session->tabs.size()==1,"Omitted tabs use saved global selection");
 write(root/"dtd.xml","<!DOCTYPE Xiphos [<!ENTITY secret SYSTEM 'file:///not-read'>]><Xiphos/>");rejects([&]{(void)worker.prepare_legacy_migration({(root/"dtd.xml").u8string(),"",""}).get();});
 gsize converted_size=0;GError *error=nullptr;auto original=settings();gchar *converted=g_convert(original.data(),original.size(),"UTF-16LE","UTF-8",nullptr,&converted_size,&error);check(converted&&!error,"Create original UTF-16 fixture");std::string utf16("\xff\xfe",2);utf16.append(converted,converted_size);g_free(converted);write(root/"utf16.xml",utf16);
 auto encoded=worker.prepare_legacy_migration({(root/"utf16.xml").u8string(),tabfile.u8string(),modops.u8string()}).get();check(encoded.session&&encoded.sources[0].bytes==utf16,"Declared/auto-detected XML encoding preserved while values decode to UTF-8");auto binary=serialize_legacy_migration(encoded);check(parse_legacy_migration(binary).sources[0].bytes==utf16,"Package length framing preserves embedded UTF-16 NUL units");
 auto overflow=settings();auto position=overflow.find("<showtexts>0");overflow.replace(position,12,"<showtexts>2147483648");write(root/"overflow.xml",overflow);rejects([&]{(void)worker.prepare_legacy_migration({(root/"overflow.xml").u8string(),tabfile.u8string(),""}).get();});
 auto legacy_ini=ini+"; Original Latin-1 comment "+std::string(1,static_cast<char>(0xe9))+"\n";write(root/"latin1.conf",legacy_ini);auto comments=worker.prepare_legacy_migration({config.u8string(),tabfile.u8string(),(root/"latin1.conf").u8string()}).get();check(comments.session&&comments.sources[2].bytes==legacy_ini,"Non-UTF-8 INI comments retained exactly without changing module identities");
 // Retained xml_find_prop searches later sections only when the key is absent,
 // not when an earlier matching element has an empty value.
 const std::string sparse="<Xiphos><modules><bible>Text</bible></modules><keys><verse>John 3:16</verse></keys><misc><showtexts/><showpreview> -0tail</showpreview></misc><misc><showtexts>0</showtexts><showdicts>0</showdicts></misc><lexicons><greek/></lexicons><lexicons><greek>Dictionary</greek><hebrew>Dictionary</hebrew></lexicons><future><opaque>Original Ω</opaque></future></Xiphos>";
 write(root/"sparse.xml",sparse);auto sparseResult=worker.prepare_legacy_migration({(root/"sparse.xml").u8string(),"",""}).get();
 check(sparseResult.session&&sparseResult.session->tabs.size()==1,"Sparse duplicate sections remain migratable");const auto &sparseTab=sparseResult.session->tabs[0];
 check(sparseTab.visibility.texts&&!sparseTab.visibility.preview&&!sparseTab.visibility.dictionaries,"First empty value uses legacy fallback; absent key searches later sections; signed numeric prefix remains compatible");
 check(sparseTab.state.lexicons->greek=="StrongsGreek"&&sparseTab.state.lexicons->hebrew=="Dictionary","Sparse lexicons match retained first-key lookup including empty values");
 std::size_t mappedGreek=0,backedGreek=0;for(const auto &row:sparseResult.settings)if(row.section=="lexicons"&&row.key=="greek"){if(row.mapped)++mappedGreek;else ++backedGreek;}check(mappedGreek==1&&backedGreek==1,"Review marks only effective duplicate key while preserving shadowed metadata");
 auto sparsePackage=save_legacy_migration((root/"sparse.bundle").u8string(),sparseResult);check(sparsePackage.sources[0].bytes==sparse&&sparsePackage.session->tabs[0].visibility.texts,"Package round trip preserves sparse conversion and exact source");
 for(const auto *encoding:{"UTF-16BE","ISO-8859-1"}){
  auto sourceText=std::string("<?xml version='1.0' encoding='")+encoding+"'?>"+sparse;auto accent=sourceText.find("Original Ω");sourceText.replace(accent,std::string("Original Ω").size(),"Original café");
  converted_size=0;error=nullptr;converted=g_convert(sourceText.data(),sourceText.size(),encoding,"UTF-8",nullptr,&converted_size,&error);check(converted&&!error,"Create original declared-encoding fixture");std::string exact(converted,converted_size);g_free(converted);
  const auto selected=root/(std::string(encoding)+".xml");write(selected,exact);auto oldEncoding=worker.prepare_legacy_migration({selected.u8string(),"",""}).get();check(oldEncoding.session&&oldEncoding.sources[0].bytes==exact,"Declared big-endian/legacy single-byte XML decodes without replacing original bytes");
  bool unicode=false;for(const auto &row:oldEncoding.settings)if(row.section=="future"&&row.key=="opaque"&&row.value=="Original café")unicode=true;check(unicode,"Review decodes declared original text to UTF-8");
  auto packageCopy=parse_legacy_migration(serialize_legacy_migration(oldEncoding));const auto restoredFolder=root/(std::string(encoding)+"-restored");restore_legacy_migration_sources(restoredFolder.u8string(),packageCopy);check(bytes(restoredFolder/"settings.xml")==exact&&bytes(selected)==exact,"Package restoration keeps exact encoded bytes and leaves source unchanged");
 }
 auto large=settings();large.insert(large.find("</Xiphos>"),"<future><large>"+std::string(40000,'x')+"</large></future>");write(root/"large-unknown.xml",large);auto retained=worker.prepare_legacy_migration({(root/"large-unknown.xml").u8string(),tabfile.u8string(),modops.u8string()}).get();check(retained.session&&retained.sources[0].bytes==large,"Large unknown settings remain exact backup data, not session-field failures");
 } // Destroy the C++ FIFO owner before opening the C ABI worker.
 // C ownership and admission copying remain valid after worker/request teardown.
 xc_worker *cw=nullptr;check(xc_worker_open((root/"library").u8string().c_str(),&cw)==XC_OK,"C migration worker");std::string selected=config.u8string();xc_legacy_migration_input ci{selected.c_str(),nullptr,nullptr};std::string tabpath=tabfile.u8string();ci.tabs=tabpath.c_str();xc_request *r=nullptr;check(xc_worker_prepare_legacy_migration(cw,&ci,nullptr,0,&r)==XC_OK,"C migration admitted");selected="changed after admission";tabpath="changed";check(wait(r)==XC_OK,"C migration completes");auto owned=xc_request_take_legacy_migration(r);check(owned&&!xc_request_take_legacy_migration(r),"Take-once owned migration");xc_request_release(r);xc_worker_close(cw);check(xc_legacy_migration_ready(owned)&&xc_legacy_migration_tab_count(owned)==2,"Owned migration survives teardown");xc_legacy_migration_setting row{};check(xc_legacy_migration_setting_get(owned,0,&row)==XC_OK&&std::string(row.section)=="modules","Borrowed setting view from owned handle");xc_legacy_migration_release(owned);
 check(bytes(config)==settings()&&bytes(tabfile)==tabs()&&bytes(modops)==ini,"All original inputs unchanged");std::cout<<"Legacy settings/session migration, exact backup/restore and owned C checks passed\n";return 0;
 }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
