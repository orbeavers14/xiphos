// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/worker.hpp"
#include "xiphos/core/c_api.h"
#include "xiphos/core/engine_manager.hpp"
#include "xiphos/core/search_policy.hpp"
#include <markupfiltmgr.h>
#include <rawgenbook.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <thread>
using namespace xiphos::core;
namespace fs=std::filesystem;
static void check(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
template<class F>static void rejects(F call){try{call();}catch(const std::invalid_argument &){return;}throw std::runtime_error("Expected creation rejection");}
static auto bytes(const fs::path &root){std::map<std::string,std::string> result;for(const auto &file:fs::recursive_directory_iterator(root))if(file.is_regular_file()){std::ifstream in(file.path(),std::ios::binary);result.emplace(fs::relative(file.path(),root).generic_string(),std::string(std::istreambuf_iterator<char>(in),{}));}return result;}
static xc_status wait(xc_request *r){const auto until=std::chrono::steady_clock::now()+std::chrono::seconds(30);xc_status s;while((s=xc_request_poll(r))==XC_PENDING){check(std::chrono::steady_clock::now()<until,"Creation request deadline");std::this_thread::sleep_for(std::chrono::milliseconds(1));}return s;}
int main(int argc,char **argv)try{
 check(argc==3||(argc==4&&(std::string(argv[3])=="--require-index"||std::string(argv[3])=="--require-move")),"Named scratch, generated library and optional required index gate");const bool required=argc==4;const bool move_required=required&&std::string(argv[3])=="--require-move";fs::path root=argv[1];check(root.filename()=="book-creation-fixture","Named disposable root only");fs::remove_all(root);fs::copy(fs::path(argv[2]),root,fs::copy_options::recursive);fs::remove(root/"mods.d/modules-conf.cache");
 {sword::RawGenBook writer((root/"data/CaptureBook/entries").string().c_str());writer.setKey("/Original");writer.setEntry("Original index seed");}
 bool indexed=false;
 {EngineManager manager(root.string().c_str(),true,new sword::MarkupFilterMgr(sword::FMT_XHTML),false,false);auto *module=manager.getModule("CaptureBook");check(module,"Indexed editable fixture");auto status=module->createSearchFramework();indexed=supports_indexed_search(*module);std::cout<<"Editable index create status="<<int(status)<<" usable="<<indexed<<"\n";}
 if(required)check(indexed,"Required alternate SDK must create a usable editable-book index");
 const ModuleEditTarget target{"CaptureBook","/Journals/Entry Ω",true};
 {
  StudyWorker worker(root.string());auto tab=worker.create_tab().get(),other=worker.create_tab().get();auto selected=worker.select_book_document(other,{"CaptureBook",std::string("/Original")}).get();auto original=bytes(root/"data");
  auto prepared=worker.read_module_edit(tab,target).get();check(prepared.text.empty()&&!prepared.offset&&bytes(root/"data")==original,"Creation preparation never writes parent nodes");
  rejects([&]{(void)worker.save_module_edit(tab,target,std::string(64,'0'),"stale").get();});check(bytes(root/"data")==original,"Stale creation leaves module bytes unchanged");
  auto saved=worker.save_module_edit(tab,target,prepared.revision,"Original journal Ω").get();check(saved.offset&&saved.reference==target.reference&&saved.text=="Original journal Ω","Private tree creation verifies exact canonical path/content");
  if(indexed){auto searched=worker.search_mixed_batch_task(tab,{{"CaptureBook","journal",SearchMode::words}}).result.get();check(!searched.failures&&searched.copied_results==1&&searched.groups[0].result->engine_type!=-4,"Fresh search falls back to new content after invalidating a preexisting index");}
  auto read=worker.read_module_edit(tab,{target.module,target.reference}).get();check(read.revision==saved.revision&&read.offset==saved.offset,"Created section reopens as an ordinary existing editor target");
  check(worker.book_document_selection(other).get()->tree.node.offset==selected.tree.node.offset,"New sections preserve sibling-tab selection addresses");
  rejects([&]{(void)worker.read_module_edit(tab,target).get();});rejects([&]{(void)worker.save_module_edit(tab,target,prepared.revision,"overwrite").get();});
  auto pending=worker.read_module_edit(tab,{"CaptureBook","/Prayers/Next",true}).get();auto existing=worker.read_module_edit(tab,{target.module,target.reference}).get();(void)worker.save_module_edit(tab,{target.module,target.reference},existing.revision,"External edit").get();auto changed=bytes(root/"data");
  rejects([&]{(void)worker.save_module_edit(tab,{"CaptureBook","/Prayers/Next",true},pending.revision,"stale").get();});check(bytes(root/"data")==changed,"Intervening module edits invalidate pending creation");
  for(const auto *path:{"/","relative","/Bad//Path","/Bad/../Path","/Bad/"})rejects([&]{(void)worker.read_module_edit(tab,{"CaptureBook",path,true}).get();});
  rejects([&]{(void)worker.read_module_edit(tab,{"CaptureBook","/"+std::string(4096,'x'),true}).get();});
  auto empty=worker.read_module_edit(tab,{"CaptureBook","/Prayers/Empty",true}).get();
  check(worker.save_module_edit(tab,{"CaptureBook","/Prayers/Empty",true},empty.revision,"").get().offset>0,"Explicit empty section creation retains a real tree node");
  const auto source=worker.read_module_edit(tab,{target.module,target.reference}).get();
  (void)worker.select_book_document(other,{target.module,target.reference}).get();
  const ModuleEditTarget rename{target.module,target.reference,false,"Renamed Ω"};
  rejects([&]{(void)worker.save_module_edit(tab,rename,std::string(64,'0'),"").get();});
  const auto original_bytes=bytes(root/"data");
  rejects([&]{(void)worker.save_module_edit(tab,{target.module,target.reference,false,"Entry Ω"},source.revision,"").get();});
  check(bytes(root/"data")==original_bytes,"Rename collision leaves the book byte-for-byte intact");
  const auto renamed=worker.save_module_edit(tab,rename,source.revision,"").get();
  check(renamed.reference=="/Journals/Renamed Ω" && renamed.offset==source.offset &&
        renamed.text==source.text &&
        worker.book_document_selection(other).get()->tree.node.path==renamed.reference,
        "Leaf rename preserves source, offset and sibling selection");
  rejects([&]{(void)worker.read_module_edit(tab,{target.module,target.reference}).get();});
  const auto parent=worker.read_module_edit(tab,{target.module,"/Journals"}).get();
  rejects([&]{(void)worker.save_module_edit(tab,{target.module,"/Journals",false,"Archive"},parent.revision,"").get();});
  const auto subtree=worker.save_module_edit(tab,{target.module,"/Journals",false,"Archive",true},parent.revision,"").get();
  const auto child=worker.read_module_edit(tab,{target.module,"/Archive/Renamed Ω"}).get();
  check(subtree.reference=="/Archive" && subtree.offset==parent.offset &&
        child.offset==renamed.offset && child.text==renamed.text &&
        worker.book_document_selection(other).get()->tree.node.path==child.reference,
        "Subtree rename preserves descendant data, offsets and open selections");
  const ModuleEditTarget move{target.module,subtree.reference,false,{},false,"/Prayers"};
  if(move_required){
   const auto moved=worker.save_module_edit(tab,move,subtree.revision,"").get();
   const auto descendant=worker.read_module_edit(tab,{target.module,"/Prayers/Archive/Renamed Ω"}).get();
   check(moved.reference=="/Prayers/Archive" && moved.offset==subtree.offset &&
         descendant.offset==child.offset && descendant.text==child.text &&
         worker.book_document_selection(other).get()->tree.node.path==descendant.reference,
         "Staged move preserves descendant identity and follows open selections");
  }
#ifdef XIPHOS_SWORD_SAFE_BOOK_DELETE
  const ModuleEditTarget pending_delete{target.module,"/Prayers/Disposable",true};
  const auto pending_leaf=worker.read_module_edit(tab,pending_delete).get();
  const auto leaf=worker.save_module_edit(tab,pending_delete,pending_leaf.revision,"uniquedeletedword").get();
  (void)worker.select_book_document(other,{target.module,leaf.reference}).get();
  const ModuleEditTarget deletion{target.module,leaf.reference,false,{},false,{},true};
  rejects([&]{(void)worker.save_module_edit(tab,deletion,pending_leaf.revision,"").get();});
  const auto deleted=worker.save_module_edit(tab,deletion,leaf.revision,"").get();
  check(!deleted.offset && deleted.text.empty() && !worker.book_document_selection(other).get(),
        "Staged deletion removes the leaf and clears its open selection");
  rejects([&]{(void)worker.read_module_edit(tab,{target.module,leaf.reference}).get();});
  const ModuleEditTarget trash{target.module,"/Prayers/Trash/Child",true};
  const auto pending_child=worker.read_module_edit(tab,trash).get();
  const auto trash_child=worker.save_module_edit(tab,trash,pending_child.revision,"uniquedeletedbranch").get();
  const auto trash_root=worker.read_module_edit(tab,{target.module,"/Prayers/Trash"}).get();
  (void)worker.select_book_document(other,{target.module,trash_child.offset}).get();
  rejects([&]{(void)worker.save_module_edit(tab,{target.module,trash_root.reference,false,{},false,{},true},trash_root.revision,"").get();});
  const auto removed=worker.save_module_edit(tab,{target.module,trash_root.reference,false,{},false,{},false,true},trash_root.revision,"").get();
  check(!removed.offset&&!worker.book_document_selection(other).get(),"Subtree deletion clears descendant selection");
  const auto gone=worker.search_mixed_batch_task(tab,{{target.module,"uniquedeletedbranch",SearchMode::phrase}}).result.get();
  check(!gone.failures&&!gone.copied_results,"Sequential search omits deleted subtree prose before reindexing");
  for(const auto &entry:{trash_root,trash_child}) {
    rejects([&]{(void)worker.read_module_edit(tab,{target.module,entry.reference}).get();});
    bool detached=false;
    try{(void)worker.book_tree(target.module,entry.offset).get();}
    catch(const std::out_of_range &){detached=true;}
    check(detached,"Removed subtree no longer resolves by legacy offset");
  }
#endif
  rejects([&]{(void)worker.read_module_edit(tab,{"Personal","John3:16",true}).get();});
 }
 {EngineManager manager(root.string().c_str(),true,new sword::MarkupFilterMgr(sword::FMT_XHTML),false,false);auto *module=manager.getModule("CaptureBook");module->createSearchFramework();if(required)check(supports_indexed_search(*module),"Rollback fixture needs an actual existing index");
#ifdef XIPHOS_SWORD_SAFE_BOOK_DELETE
  check(supports_indexed_search(*module)&&module->search("uniquedeletedword",-4).getCount()==0&&
        module->search("uniquedeletedbranch",-4).getCount()==0&&module->search("seed",-4).getCount()==1,"Rebuilt index omits deleted leaf and retains original");
#endif
  if(move_required){
   auto &hits=module->search("External",-4);
   check(hits.getCount()==1 && std::string(hits.getElement(0)->getText())=="/Prayers/Archive/Renamed Ω",
         "Rebuilt index returns the moved descendant at its new path");
  }}
 {
  Reader reader(root.string());ModuleEditTarget pending{"CaptureBook","/Rollback/New",true};auto before=bytes(root/"data");auto prepared=reader.read_module_edit(pending);
  bool failed=false;try{reader.save_module_edit(pending,prepared.revision,"not committed",false,[]{throw std::runtime_error("Injected refresh failure");});}catch(const std::runtime_error &){failed=true;}check(failed,"Injected refresh must fail");check(bytes(root/"data")==before,"Failed post-install refresh rolls back created tree and content");
 }
#ifdef XIPHOS_SWORD_SAFE_BOOK_DELETE
 {
  Reader reader(root.string());
  const auto original=reader.read_module_edit({"CaptureBook","/Original"});
  const auto before=bytes(root/"data");
  std::vector<ModuleEditTarget> operations{{"CaptureBook","/Original",false,{},false,{},true}};
  operations.push_back({"CaptureBook","/Prayers",false,{},false,{},false,true});
  if(move_required) operations.push_back({"CaptureBook","/Original",false,{},false,"/Prayers"});
  for(const auto &operation:operations){
   bool refreshed=false, failed=false;
   try{
    reader.save_module_edit(operation,reader.read_module_edit({operation.module,operation.reference}).revision,"",false,[&]{
     refreshed=true;
     throw std::runtime_error("Injected refresh failure");
    });
   }catch(const std::runtime_error &){failed=true;}
   check(refreshed&&failed&&bytes(root/"data")==before&&
         reader.read_module_edit({"CaptureBook","/Original"}).offset==original.offset,
         "Failed tree edit refresh restores original data, index, and section offset");
  }
 }
#endif
 xc_worker *worker=nullptr;xc_request *request=nullptr;check(xc_worker_open(root.string().c_str(),&worker)==XC_OK&&xc_worker_create_tab(worker,&request)==XC_OK&&wait(request)==XC_OK,"C creation worker/tab");auto tab=xc_request_tab(request);xc_request_release(request);
 check(xc_worker_prepare_book_entry(worker,tab,"CaptureBook","/Prayers/Copied",&request)==XC_OK&&wait(request)==XC_OK,"C creation prepare");auto *prepared=xc_request_take_module_edit(request);xc_request_release(request);std::string revision=xc_module_edit_revision(prepared);xc_module_edit_release(prepared);
 check(xc_worker_create_book_entry(worker,tab,"CaptureBook","/Prayers/Copied",revision.c_str(),"Original prayer",&request)==XC_OK,"C explicit creation accepted");revision.clear();check(wait(request)==XC_OK,"Copied revision survives caller mutation");auto *owned=xc_request_take_module_edit(request);xc_request_release(request);xc_worker_close(worker);check(owned&&std::string(xc_module_edit_text(owned))=="Original prayer"&&xc_module_edit_offset(owned)>0,"Owned created result survives worker teardown");xc_module_edit_release(owned);
 {const auto cache=root/"data/CaptureBook/xapian";fs::create_directories(cache);{std::ofstream marker(cache/"original.txt");marker<<"Original unrecognized cache or asset";}
  Reader reader(root.string());ModuleEditTarget pending{"CaptureBook","/Foreign/New",true};auto prepared=reader.read_module_edit(pending);const auto original=bytes(root/"data");
  rejects([&]{reader.save_module_edit(pending,prepared.revision,"must not commit",false,[]{});});check(bytes(root/"data")==original,"Unrecognized cache or asset directories reject without deletion or module writes");}
 std::cout<<"Exact staged book section creation, stale/existing guards, sibling refresh, rollback and C lifetime passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
