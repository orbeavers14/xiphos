// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/legacy_repositories.hpp"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <installmgr.h>
#include "xiphos/core/c_api.h"
#include <chrono>
#include <thread>
#include <stdexcept>
using namespace xiphos::core;
void check(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
template<class F>void rejects(F f){try{f();}catch(const std::invalid_argument &){return;}throw std::runtime_error("Expected rejection");}
int main(int argc,char **argv){try{
 check(argc==2,"Named fixture path required");auto root=std::filesystem::u8path(argv[1]);check(root.filename()=="legacy-repositories-fixture","Named disposable fixture only");
 std::filesystem::remove_all(root);std::filesystem::create_directories(root);
 // InstallSource::getConfEnt() emits caption|source|directory|u|p|uid.
 // Repeated INI keys are significant. All names/content below are original.
 const std::string fixture="[General]\nHTTPSSource=Ignored|ignored.invalid|/raw|||uid\n[Sources]\n# Original fixture\nHTTPSSource=Original Ω|example.invalid|/raw|||id-a\nFTPSource=Old|ftp.invalid|/raw|||id-b\nHTTPSource=Old|http.invalid|/raw\nDIRSource=Local|[local]|/original/library|||id-c\nHTTPSSource=Private|private.invalid|/raw|Original-user|Original-secret|id-d\nHTTPSSource=Broken|user:Original-secret@example.invalid|/raw\nUnknownSource=Unknown|unknown.invalid|/raw\nHTTPSSource=Second|second.invalid|/raw\n[Elsewhere]\nHTTPSSource=Ignored|ignored.invalid|/raw\n";
 sword::InstallSource oracle("HTTPS","Original Ω|example.invalid|/raw|||id-a");
 check(std::string(oracle.caption.c_str())=="Original Ω"&&std::string(oracle.source.c_str())=="example.invalid"&&std::string(oracle.directory.c_str())=="/raw"&&std::string(oracle.u.c_str()).empty()&&std::string(oracle.p.c_str()).empty(),"Selected SDK source-field characterization");
 const auto source=root/"InstallMgr.conf";{std::ofstream out(source,std::ios::binary);out<<fixture;}
 rejects([&]{read_legacy_repositories(root.u8string());});
 std::string invalid_path=source.u8string();invalid_path.push_back('\0');invalid_path+="tail";rejects([&]{read_legacy_repositories(invalid_path);});
 auto disk=read_legacy_repositories(source.u8string());check(disk.candidates.size()==8,"Bounded explicit file preview");
 auto preview=preview_legacy_repositories(fixture);check(preview.candidates.size()==8,"Only source-section entries, including duplicate keys, retained in file order");
 using D=LegacyRepositoryDisposition;
 check(preview.candidates[0].line==5&&preview.candidates[0].name=="Original Ω"&&preview.candidates[0].disposition==D::https_review,"HTTPS candidate exact UTF-8 and source line");
 check(preview.candidates[1].disposition==D::insecure_transport&&preview.candidates[2].disposition==D::insecure_transport,"No silent insecure-source upgrade");
 check(preview.candidates[3].disposition==D::local_directory&&preview.candidates[4].disposition==D::credentials&&preview.candidates[5].disposition==D::malformed&&preview.candidates[6].disposition==D::unsupported,"Explicit unsupported categories");
 for(std::size_t i=1;i<7;++i)check(preview.candidates[i].name.empty()&&preview.candidates[i].host.empty()&&preview.candidates[i].catalog_directory.empty(),"Rejected raw source data and credentials never returned");
 auto snapshot=accept_legacy_repository({},preview.candidates[0],"original","/explicit-packages");
 check(snapshot.repositories.size()==1&&!snapshot.repositories[0].enabled&&!snapshot.repositories[0].cached&&snapshot.revision.empty(),"Review produces disabled uncached value, no implicit file save or network");
 check(repository_module_url(snapshot.repositories[0].endpoint,"Module")=="https://example.invalid/explicit-packages/Module.zip","Explicit package path independent of old catalog directory");
 rejects([&]{accept_legacy_repository(snapshot,preview.candidates[0],"original","/packages");});
 rejects([&]{accept_legacy_repository({},preview.candidates[1],"ftp","/packages");});
 rejects([&]{accept_legacy_repository({},preview.candidates[0],"new","");});
 check(serialize_repository_snapshot(snapshot).find("Original-secret")==std::string::npos,"No imported credentials persisted");
 rejects([&]{preview_legacy_repositories(std::string(maximum_legacy_repository_bytes+1,'x'));});
 std::string invalid=fixture;invalid.push_back('\0');invalid+="tail";rejects([&]{preview_legacy_repositories(invalid);});
 std::string many="[Sources]\n";for(int i=0;i<257;++i)many+="HTTPSSource=A|example.invalid|/raw\n";rejects([&]{preview_legacy_repositories(many);});
 std::filesystem::create_directories(root/"library"/"mods.d");
 xc_worker *worker=nullptr;check(xc_worker_open((root/"library").u8string().c_str(),&worker)==XC_OK,"C worker opens isolated library");
 xc_request *request=nullptr;std::string path=source.u8string();check(xc_worker_preview_legacy_repositories(worker,path.c_str(),&request)==XC_OK,"C preview admitted");path="changed-after-admission";
 auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(10);xc_status status;
 while((status=xc_request_poll(request))==XC_PENDING){check(std::chrono::steady_clock::now()<deadline,"C preview deadline");std::this_thread::sleep_for(std::chrono::milliseconds(1));}
 check(status==XC_OK,"FIFO read result");auto owned=xc_request_take_legacy_repository_preview(request);check(owned&&!xc_request_take_legacy_repository_preview(request),"Take-once preview");xc_request_release(request);xc_worker_close(worker);
 check(xc_legacy_repository_preview_count(owned)==8,"Preview owns data after worker teardown/input change");xc_legacy_repository_candidate row{};check(xc_legacy_repository_preview_get(owned,0,&row)==XC_OK&&std::string(row.name)=="Original Ω","Owned C candidate");
 check(xc_legacy_repository_preview_get(owned,4,&row)==XC_OK&&row.disposition==3&&std::string(row.name).empty()&&std::string(row.host).empty(),"C API omits credential-bearing fields");
 xc_repository_snapshot *target=nullptr;check(xc_repository_snapshot_create(&target)==XC_OK,"C target snapshot");check(xc_repository_snapshot_accept_legacy(target,owned,0,"original","/packages")==XC_OK,"Explicit C review acceptance");
 check(xc_repository_snapshot_accept_legacy(target,owned,0,"original","/packages")!=XC_OK&&xc_repository_snapshot_count(target)==1,"Duplicate identity cannot overwrite existing source");
 xc_saved_repository saved{};check(xc_repository_snapshot_get(target,0,&saved)==XC_OK&&!saved.enabled,"C import remains disabled");xc_repository_snapshot_release(target);xc_legacy_repository_preview_release(owned);
 {std::ifstream in(source,std::ios::binary);std::string unchanged((std::istreambuf_iterator<char>(in)),{});check(unchanged==fixture,"Legacy file including unknown sections remains byte-for-byte unchanged");}
 check(std::distance(std::filesystem::directory_iterator(root),std::filesystem::directory_iterator{})==2,"Preview/accept creates no destination profile or staging files");std::filesystem::remove_all(root);
 std::cout<<"Read-only legacy repository preview and explicit import checks passed\n";return 0;
 }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
