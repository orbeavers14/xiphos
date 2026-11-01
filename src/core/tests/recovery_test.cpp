// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/worker.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace xiphos::core;
namespace fs=std::filesystem;
void check(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
template<class F>void rejects(F f){try{f();}catch(const std::exception &){return;}throw std::runtime_error("Expected rejection");}
std::string bytes(const std::string &file){std::ifstream in(fs::u8path(file),std::ios::binary);return {std::istreambuf_iterator<char>(in),{}};}
void write(const std::string &file,const std::string &s){std::ofstream out(fs::u8path(file),std::ios::binary);out<<s;}
int main(int argc,char **argv){try{
 check(argc==2||argc==3,"Fixture path required");auto root=fs::u8path(argv[1]);check(root.filename()=="recovery-fixture","Named fixture only");auto file=(root/"recovery").u8string();
 if(argc==3){auto loaded=read_recovery_snapshot(file);check(!loaded.recovered_from_previous&&loaded.drafts.size()==1&&loaded.drafts[0].text=="Changed Ω draft","Fresh process restores exact draft source");check(loaded.session.tabs.empty(),"Session round trip");fs::remove_all(root);return 0;}
 fs::remove_all(root);fs::create_directories(root/"library/mods.d");RecoverySnapshot s;RecoveryDraft draft;draft.id="original";draft.title="Original draft";draft.target=RecoveryTarget::file;draft.path=(root/"document").u8string();draft.origin_revision=std::string(64,'a');draft.saved_text="Original saved prose";draft.text="Changed Ω draft";draft.selection_end=draft.text.size();s.drafts.push_back(draft);
 auto encoded=serialize_recovery_snapshot(s);check(serialize_recovery_snapshot(parse_recovery_snapshot(encoded))==encoded,"Exact source/session round trip");for(std::size_t n=0;n<encoded.size();++n)rejects([&]{parse_recovery_snapshot(encoded.substr(0,n));});auto altered=encoded;altered.back()^=1;rejects([&]{parse_recovery_snapshot(altered);});
 auto bad=s;bad.drafts.push_back(draft);rejects([&]{serialize_recovery_snapshot(bad);});bad=s;bad.drafts[0].selection_start=9;bad.drafts[0].selection_end=9;rejects([&]{serialize_recovery_snapshot(bad);});bad=s;bad.drafts[0].target=RecoveryTarget::module;rejects([&]{serialize_recovery_snapshot(bad);});
 check(recovery_draft_source_matches(draft,draft.origin_revision)&&!recovery_draft_source_matches(draft,std::string(64,'b'))&&!recovery_draft_source_matches(draft,std::nullopt),"Changed/deleted source requires conflict resolution");
 auto first=save_recovery_snapshot(file,std::nullopt,s);rejects([&]{save_recovery_snapshot(file,std::nullopt,s);});auto next=s;next.drafts[0].text="Later unsaved prose";next.drafts[0].selection_end=next.drafts[0].text.size();auto saved=save_recovery_snapshot(file,first.revision,next);check(parse_recovery_snapshot(bytes(file+".previous")).drafts[0].text==draft.text,"Previous valid snapshot retained");auto backup=bytes(file+".previous");rejects([&]{save_recovery_snapshot(file,first.revision,s);});check(bytes(file+".previous")==backup,"Stale save does not alter backup");
 write(file,"Corrupt primary");auto recovered=read_recovery_snapshot(file);check(recovered.recovered_from_previous&&recovered.drafts[0].text==draft.text,"Corrupt primary offers last good draft");rejects([&]{save_recovery_snapshot(file,recovered.revision,recovered);});check(bytes(file)=="Corrupt primary"&&bytes(file+".previous")==backup,"Recovery never overwrites damaged primary");auto exported=save_recovery_snapshot((root/"recovered-copy").u8string(),std::nullopt,recovered);check(!exported.recovered_from_previous&&exported.drafts[0].text==draft.text,"Explicit recovery Save As preserves draft");fs::remove(file);check(read_recovery_snapshot(file).recovered_from_previous,"Missing primary fallback");
 write(file,serialize_recovery_snapshot(next));write(file+".previous","Unrelated future backup");rejects([&]{save_recovery_snapshot(file,saved.revision,s);});check(bytes(file)==serialize_recovery_snapshot(next)&&bytes(file+".previous")=="Unrelated future backup","Unknown backup preserved");fs::remove(file+".previous");
 {StudyWorker worker((root/"library").u8string());auto loaded=worker.read_recovery(file).get();auto done=worker.save_recovery(file,loaded.revision,s).get();check(done.drafts[0].saved_text==draft.saved_text&&worker.modules().get().empty(),"FIFO recovery storage does not mutate source modules");}
 check(!fs::exists(fs::u8path(draft.path)),"Recovered file source never written");std::cout<<"Recovery checks passed\n";return 0;
 }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
