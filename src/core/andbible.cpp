// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/andbible.hpp"
#include "xiphos/core/editable_document.hpp"
#include "bounded_file.hpp"
#include <sqlite3.h>
#include <versekey.h>
#include <glib.h>
#include <libxml/tree.h>
#include <filesystem>
#include <fstream>
#include <memory>
#include <map>
#include <set>
#include <chrono>
#include <atomic>
#include <climits>
#include <cstdio>
namespace xiphos::core {
namespace {
namespace fs=std::filesystem;
void invalid(){throw std::invalid_argument("Unsupported or invalid AndBible backup schema, text or bounds");}
struct Snapshot {
 fs::path folder,file;
 Snapshot(){static std::atomic<unsigned long> serial{0};for(unsigned n=0;n<64;++n){auto p=fs::temp_directory_path()/("xiphos-andbible-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+"-"+std::to_string(serial++));if(fs::create_directory(p)){folder=p;try{fs::permissions(p,fs::perms::owner_all,fs::perm_options::replace);}catch(...){fs::remove(p);throw;}file=folder/"backup.sqlite";return;}}throw std::runtime_error("Cannot stage AndBible snapshot");}
 ~Snapshot(){std::error_code e;for(const auto *suffix:{"","-wal","-shm","-journal"})fs::remove(fs::u8path(file.u8string()+suffix),e);fs::remove(folder,e);}
};
struct Database {sqlite3 *db=nullptr;~Database(){if(db)sqlite3_close(db);}};
struct Query {
 sqlite3_stmt *s=nullptr;
 Query(sqlite3 *db,const char *sql){if(sqlite3_prepare_v2(db,sql,-1,&s,nullptr)!=SQLITE_OK){sqlite3_finalize(s);s=nullptr;invalid();}}
 ~Query(){sqlite3_finalize(s);}
 bool next(){auto status=sqlite3_step(s);if(status==SQLITE_ROW)return true;if(status==SQLITE_DONE)return false;invalid();return false;}
};
std::string text(sqlite3_stmt *s,int column,std::size_t max=maximum_bookmark_text_bytes){if(sqlite3_column_type(s,column)==SQLITE_NULL)return {};if(sqlite3_column_type(s,column)!=SQLITE_TEXT)invalid();auto count=sqlite3_column_bytes(s,column);if(count<0||static_cast<std::size_t>(count)>max)invalid();const auto *p=sqlite3_column_text(s,column);if(!p&&count)throw std::bad_alloc();std::string value(p?reinterpret_cast<const char *>(p):"",count);if(!editable_document_text(value))invalid();return value;}
std::string uuid(sqlite3_stmt *s,int column){if(sqlite3_column_type(s,column)!=SQLITE_BLOB||sqlite3_column_bytes(s,column)!=16)return {};const auto *p=static_cast<const unsigned char *>(sqlite3_column_blob(s,column));if(!p)return {};const char *hex="0123456789abcdef";std::string out;for(int i=0;i<16;++i){out+=hex[p[i]>>4];out+=hex[p[i]&15];}return out;}
void prop(xmlNode *node,const char *name,const std::string &value){if(!xmlNewProp(node,BAD_CAST name,BAD_CAST value.c_str()))throw std::bad_alloc();}
bool table(sqlite3 *db,const char *name){Query q(db,"SELECT type,sql FROM sqlite_master WHERE name=?1");sqlite3_bind_text(q.s,1,name,-1,SQLITE_STATIC);if(!q.next())return false;if(text(q.s,0)!="table"||text(q.s,1).find("CREATE VIRTUAL")==0)invalid();return true;}
}
AndBibleImport read_andbible_bookmarks(const std::string &path){
 if(path.empty()||path.size()>4096||!editable_document_text(path))invalid();auto source=fs::u8path(path);if(!fs::is_regular_file(source))invalid();
 for(const auto *suffix:{"-wal","-journal"}){auto side=fs::u8path(path+suffix);if(fs::exists(side)&&fs::file_size(side)>0)throw std::invalid_argument("Choose a closed, standalone AndBible backup without pending WAL/journal data");}
 auto raw=detail::read_bounded_file(source,64*1024*1024,"AndBible backup");if(raw.size()<16||raw.compare(0,16,std::string("SQLite format 3\0",16)))invalid();
 AndBibleImport result;std::unique_ptr<gchar,decltype(&g_free)> digest(g_compute_checksum_for_data(G_CHECKSUM_SHA256,reinterpret_cast<const guchar *>(raw.data()),raw.size()),g_free);if(!digest)throw std::bad_alloc();result.revision=digest.get();
 Snapshot copy;{std::ofstream out(copy.file,std::ios::binary);out.exceptions(std::ios::badbit|std::ios::failbit);out.write(raw.data(),raw.size());out.close();}
 Database db;if(sqlite3_open_v2(copy.file.u8string().c_str(),&db.db,SQLITE_OPEN_READONLY|SQLITE_OPEN_NOMUTEX,nullptr)!=SQLITE_OK)invalid();
 sqlite3_db_config(db.db,SQLITE_DBCONFIG_DEFENSIVE,1,nullptr);sqlite3_db_config(db.db,SQLITE_DBCONFIG_TRUSTED_SCHEMA,0,nullptr);sqlite3_db_config(db.db,SQLITE_DBCONFIG_ENABLE_LOAD_EXTENSION,0,nullptr);
 sqlite3_limit(db.db,SQLITE_LIMIT_LENGTH,1024*1024);sqlite3_limit(db.db,SQLITE_LIMIT_SQL_LENGTH,4096);unsigned budget=20000;
 sqlite3_progress_handler(db.db,1000,[](void *p){auto &left=*static_cast<unsigned *>(p);return left?--left==0:1;},&budget);
 sqlite3_set_authorizer(db.db,[](void *,int action,const char *,const char *,const char *,const char *){return action==SQLITE_SELECT||action==SQLITE_READ||action==SQLITE_TRANSACTION?SQLITE_OK:SQLITE_DENY;},nullptr);
 if(sqlite3_exec(db.db,"BEGIN",nullptr,nullptr,nullptr)!=SQLITE_OK)invalid();
 if(!table(db.db,"BibleBookmark"))invalid();bool labels_present=table(db.db,"Label"),links_present=table(db.db,"BibleBookmarkToLabel"),notes_present=table(db.db,"BibleBookmarkNotes");
 struct Label{std::string name,color;xmlNode *folder=nullptr;};std::map<std::string,Label> labels;
 if(labels_present){Query q(db.db,"SELECT id,name,color FROM Label ORDER BY rowid");while(q.next()){if(labels.size()==1000)invalid();auto id=uuid(q.s,0),name=text(q.s,1,4096);if(id.empty()||name.empty()||sqlite3_column_type(q.s,2)!=SQLITE_INTEGER)invalid();char color[8];std::snprintf(color,sizeof color,"#%06X",static_cast<unsigned>(sqlite3_column_int64(q.s,2))&0xffffffU);if(!labels.emplace(id,Label{name,color,nullptr}).second)invalid();}}
 std::unique_ptr<xmlDoc,decltype(&xmlFreeDoc)> doc(xmlNewDoc(BAD_CAST "1.0"),xmlFreeDoc);if(!doc)throw std::bad_alloc();auto *root=xmlNewNode(nullptr,BAD_CAST "SwordBookmarks");if(!root)throw std::bad_alloc();xmlDocSetRootElement(doc.get(),root);prop(root,"syntaxVersion","1.0");prop(root,"andbible-source",result.revision);
 std::unique_ptr<Query> links,notes;if(links_present)links=std::make_unique<Query>(db.db,"SELECT labelId FROM BibleBookmarkToLabel WHERE bookmarkId=?1 ORDER BY orderNumber,rowid");if(notes_present)notes=std::make_unique<Query>(db.db,"SELECT notes FROM BibleBookmarkNotes WHERE bookmarkId=?1 ORDER BY rowid");
 auto folder=[&](Label &label){if(!label.folder){label.folder=xmlNewChild(root,nullptr,BAD_CAST "Folder",nullptr);if(!label.folder)throw std::bad_alloc();prop(label.folder,"caption",label.name);prop(label.folder,"color",label.color);}return label.folder;};
 Label unfiled{"Uncategorized","",nullptr};Query bookmarks(db.db,"SELECT id,v11n,ordinalStart,ordinalEnd,book FROM BibleBookmark ORDER BY rowid");std::size_t rows=0,leaves=0,aggregate=0;std::set<std::string> bookmark_ids;
 auto account=[&](const std::string &s){if(s.size()>maximum_bookmark_snapshot_bytes-aggregate)invalid();aggregate+=s.size();};
 while(bookmarks.next()){
  if(++rows>5000)invalid();auto id=uuid(bookmarks.s,0);if(id.empty()){++result.skipped;++result.invalid_identity;continue;}
  if(!bookmark_ids.insert(id).second)throw std::invalid_argument("AndBible bookmark identities repeat");
  auto v11n=text(bookmarks.s,1,128),module=text(bookmarks.s,4,4096);sword::VerseKey lower;lower.setVersificationSystem(v11n.c_str());if(v11n.empty()||v11n!=lower.getVersificationSystem()){++result.skipped;++result.unknown_versification;continue;}
  auto start=sqlite3_column_int64(bookmarks.s,2),end=sqlite3_column_int64(bookmarks.s,3);
  if(sqlite3_column_type(bookmarks.s,2)!=SQLITE_INTEGER||sqlite3_column_type(bookmarks.s,3)!=SQLITE_INTEGER||start<0||end<start||end>LONG_MAX){++result.skipped;++result.invalid_ordinal;continue;}
  lower.setAutoNormalize(false);lower.setIndex(static_cast<long>(start));sword::VerseKey upper;upper.setVersificationSystem(v11n.c_str());upper.setAutoNormalize(false);upper.setIndex(static_cast<long>(end));
  if(lower.popError()||upper.popError()||lower.getIndex()!=start||upper.getIndex()!=end||lower.getVerse()<1||upper.getVerse()<1){++result.skipped;++result.invalid_ordinal;continue;}
  std::string key=lower.getOSISRef();if(end!=start)key+="-"+std::string(upper.getOSISRef());std::string note;
  auto *blob=sqlite3_column_blob(bookmarks.s,0);
  if(notes){sqlite3_reset(notes->s);sqlite3_clear_bindings(notes->s);sqlite3_bind_blob(notes->s,1,blob,16,SQLITE_TRANSIENT);if(notes->next()){note=text(notes->s,0);if(notes->next())invalid();}}
  std::string description=key+(note.empty()?"":" - "+note);if(description.size()>maximum_bookmark_text_bytes)invalid();
  std::vector<std::pair<xmlNode *,std::string>> targets;std::set<std::string> assigned;
  if(links){sqlite3_reset(links->s);sqlite3_clear_bindings(links->s);sqlite3_bind_blob(links->s,1,blob,16,SQLITE_TRANSIENT);while(links->next()){if(assigned.size()>=64)invalid();auto label_id=uuid(links->s,0);auto found=labels.find(label_id);if(found==labels.end()||!assigned.insert(label_id).second)invalid();auto &label=found->second;if(label.name=="__UNLABELED__"||label.name=="__SPEAK_LABEL__")continue;targets.emplace_back(folder(label),label_id);}}
  if(targets.empty())targets.emplace_back(folder(unfiled),"unfiled");
  for(const auto &target:targets){if(++leaves>=maximum_bookmark_items-1001)invalid();auto *node=xmlNewChild(target.first,nullptr,BAD_CAST "Bookmark",nullptr);if(!node)throw std::bad_alloc();for(const auto &s:{key,module,description,id,v11n})account(s);prop(node,"modulename",module);prop(node,"key",key);prop(node,"description",description);prop(node,"andbible-entry",id+":"+target.second);prop(node,"versification",v11n);prop(node,"andbible-note",note);}
  ++result.imported;
 }
 xmlChar *buffer=nullptr;int length=0;xmlDocDumpFormatMemoryEnc(doc.get(),&buffer,&length,"UTF-8",0);std::unique_ptr<xmlChar,decltype(xmlFree)> encoded(buffer,xmlFree);if(!buffer||length<=0||static_cast<std::size_t>(length)>maximum_bookmark_snapshot_bytes)invalid();result.xml.assign(reinterpret_cast<const char *>(buffer),length);(void)parse_bookmarks(result.xml);return result;
}
}
