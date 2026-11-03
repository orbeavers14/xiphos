// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/worker.hpp"
#include "xiphos/core/c_api.h"
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
 RecoverySnapshot notes;RecoveryDraft note;note.id="verse-note";note.target=RecoveryTarget::annotation;note.path=(root/"annotations.xml").u8string();note.module={"TextAlt","Tob1:1"};note.annotation_label=" Tob.1.1";note.text="Original unsaved note Ω";notes.drafts.push_back(note);
 auto note_bytes=serialize_recovery_snapshot(notes);check(note_bytes.rfind("XiphosRecovery:2\n",0)==0&&encoded.rfind("XiphosRecovery:1\n",0)==0,"Annotation snapshots opt into a new version while ordinary snapshots retain version1");
 const auto decoded=parse_recovery_snapshot(note_bytes);check(decoded.drafts[0].annotation_label==note.annotation_label&&decoded.drafts[0].module.reference==note.module.reference&&decoded.drafts[0].text==note.text&&serialize_recovery_snapshot(decoded)==note_bytes,"Exact annotation label/control/source round trip");
 auto invalid_note=notes;invalid_note.drafts[0].annotation_label="bad";rejects([&]{serialize_recovery_snapshot(invalid_note);});invalid_note=notes;invalid_note.drafts[0].text=std::string(1024*1024+1,'x');rejects([&]{serialize_recovery_snapshot(invalid_note);});
 invalid_note=notes;invalid_note.drafts[0].module.reference=std::string(4097,'x');rejects([&]{serialize_recovery_snapshot(invalid_note);});
 auto mislabeled=s;mislabeled.drafts[0].annotation_label=note.annotation_label;rejects([&]{serialize_recovery_snapshot(mislabeled);});
 const auto stored_notes=save_recovery_snapshot((root/"annotation-recovery").u8string(),std::nullopt,notes);check(!fs::exists(fs::u8path(note.path))&&stored_notes.drafts[0].text==note.text,"Annotation recovery storage never writes its source notes file");
 xc_recovery_snapshot *owned=nullptr;check(xc_recovery_snapshot_create(nullptr,nullptr,0,&owned)==XC_OK,"C recovery builder");xc_recovery_draft input{"copied-note","Note",3,note.path.c_str(),"TextAlt","Tob1:1",nullptr,"","Copied Ω note",0,0};std::string copied_label=" Tob.1.1";
 check(xc_recovery_snapshot_append_annotation(owned,&input,copied_label.c_str())==XC_OK,"C explicit annotation draft append");copied_label.clear();check(std::string(xc_recovery_snapshot_annotation_label(owned,0))==note.annotation_label,"C draft label copies before return");
 check(xc_recovery_snapshot_append_annotation(owned,&input," Tob.1.2")==XC_INVALID_ARGUMENT&&xc_recovery_snapshot_count(owned)==1,"Duplicate append failure leaves snapshot unchanged");check(!xc_recovery_snapshot_annotation_label(owned,1)&&!xc_recovery_snapshot_annotation_label(nullptr,0),"Safe annotation label getters");xc_recovery_snapshot_release(owned);
 auto first=save_recovery_snapshot(file,std::nullopt,s);rejects([&]{save_recovery_snapshot(file,std::nullopt,s);});auto next=s;next.drafts[0].text="Later unsaved prose";next.drafts[0].selection_end=next.drafts[0].text.size();auto saved=save_recovery_snapshot(file,first.revision,next);check(parse_recovery_snapshot(bytes(file+".previous")).drafts[0].text==draft.text,"Previous valid snapshot retained");auto backup=bytes(file+".previous");rejects([&]{save_recovery_snapshot(file,first.revision,s);});check(bytes(file+".previous")==backup,"Stale save does not alter backup");
 write(file,"Corrupt primary");auto recovered=read_recovery_snapshot(file);check(recovered.recovered_from_previous&&recovered.drafts[0].text==draft.text,"Corrupt primary offers last good draft");rejects([&]{save_recovery_snapshot(file,recovered.revision,recovered);});check(bytes(file)=="Corrupt primary"&&bytes(file+".previous")==backup,"Recovery never overwrites damaged primary");auto exported=save_recovery_snapshot((root/"recovered-copy").u8string(),std::nullopt,recovered);check(!exported.recovered_from_previous&&exported.drafts[0].text==draft.text,"Explicit recovery Save As preserves draft");fs::remove(file);check(read_recovery_snapshot(file).recovered_from_previous,"Missing primary fallback");
 write(file,serialize_recovery_snapshot(next));write(file+".previous","Unrelated future backup");rejects([&]{save_recovery_snapshot(file,saved.revision,s);});check(bytes(file)==serialize_recovery_snapshot(next)&&bytes(file+".previous")=="Unrelated future backup","Unknown backup preserved");fs::remove(file+".previous");
 {StudyWorker worker((root/"library").u8string());auto loaded=worker.read_recovery(file).get();auto done=worker.save_recovery(file,loaded.revision,s).get();check(done.drafts[0].saved_text==draft.saved_text&&worker.modules().get().empty(),"FIFO recovery storage does not mutate source modules");}
 check(!fs::exists(fs::u8path(draft.path)),"Recovered file source never written");std::cout<<"Recovery checks passed\n";return 0;
 }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
