// SPDX-License-Identifier: GPL-2.0-or-later
#include "remote_io.hpp"
#include "bounded_file.hpp"
#include "xiphos/core/editable_document.hpp"
#include <archive.h>
#include <archive_entry.h>
#include <zlib.h>
#include <glib.h>
#include <algorithm>
#include <array>
#include <fstream>
#include <locale.h>
#include <map>
#include <set>
namespace xiphos::core::detail {
namespace {
namespace fs=std::filesystem;
// libarchive converts names through the calling thread's C locale, even when
// the ZIP declares UTF-8. Never change the application's global locale.
class Utf8ArchiveLocale {
#ifdef _WIN32
 std::string previous_;int previous_mode_;
public:
 Utf8ArchiveLocale():previous_(setlocale(LC_CTYPE,nullptr)),previous_mode_(_configthreadlocale(_ENABLE_PER_THREAD_LOCALE)){
  if(!setlocale(LC_CTYPE,".UTF8")){_configthreadlocale(previous_mode_);throw std::runtime_error("UTF-8 archive locale is unavailable");}
 }
 ~Utf8ArchiveLocale(){setlocale(LC_CTYPE,previous_.c_str());_configthreadlocale(previous_mode_);}
#else
 locale_t selected_=nullptr,previous_=nullptr;
public:
 Utf8ArchiveLocale(){
  for(const auto *name:{"C.UTF-8","en_US.UTF-8","UTF-8"}){selected_=newlocale(LC_CTYPE_MASK,name,nullptr);if(selected_)break;}
  if(!selected_)throw std::runtime_error("UTF-8 archive locale is unavailable");previous_=uselocale(selected_);if(!previous_){freelocale(selected_);throw std::runtime_error("Cannot select UTF-8 archive locale");}
 }
 ~Utf8ArchiveLocale(){uselocale(previous_);freelocale(selected_);}
#endif
};
void invalid(){throw std::invalid_argument("Unsupported, unsafe or oversized repository archive");}
std::string folded(const std::string &s){std::unique_ptr<gchar,decltype(&g_free)> normalized(g_utf8_normalize(s.c_str(),s.size(),G_NORMALIZE_NFC),g_free);if(!normalized)throw std::bad_alloc();std::unique_ptr<gchar,decltype(&g_free)> value(g_utf8_casefold(normalized.get(),-1),g_free);if(!value)throw std::bad_alloc();return value.get();}
std::string safe_path(const char *value,bool directory){
 if(!value)invalid();std::size_t n=0;while(n<=4096&&value[n])++n;if(n>4096)invalid();std::string name(value,n);
 if(!editable_document_text(name)||name.empty()||name.front()=='/'||name.find('\\')!=name.npos||name.find(':')!=name.npos||std::any_of(name.begin(),name.end(),[](unsigned char c){return c<32||c==127;}))invalid();
 while(name.rfind("./",0)==0)name.erase(0,2);if(directory){while(!name.empty()&&name.back()=='/')name.pop_back();if(name==".")name.clear();}if(name.empty()){if(directory)return {};invalid();}
 std::size_t at=0,depth=0;while(at<name.size()){auto end=name.find('/',at);auto part=name.substr(at,end==name.npos?name.npos:end-at);if(++depth>64||part.empty()||part=="."||part==".."||part.back()=='.'||part.back()==' ')invalid();
  auto stem=folded(part.substr(0,part.find('.')));if(stem=="con"||stem=="prn"||stem=="aux"||stem=="nul"||(stem.size()==4&&(stem.rfind("com",0)==0||stem.rfind("lpt",0)==0)&&stem[3]>='1'&&stem[3]<='9'))invalid();
  if(end==name.npos)break;at=end+1;if(at==name.size())invalid();
 }return name;
}
std::uint32_t u32(const unsigned char *p){return p[0]|(std::uint32_t(p[1])<<8)|(std::uint32_t(p[2])<<16)|(std::uint32_t(p[3])<<24);}
unsigned u16(const unsigned char *p){return p[0]|(unsigned(p[1])<<8);}
// Bound ZIP central-directory allocation before libarchive parses it. The
// execution slice is below4GiB: reject ZIP64, split/encrypted archives and tails.
void zip_bounds(const fs::path &file,const TransferOptions &o){
 const auto size=fs::file_size(file);if(size<22||size>o.maximum_download_bytes)invalid();std::ifstream in(file,std::ios::binary);const auto tail=static_cast<std::size_t>(std::min<std::uint64_t>(size,65557));std::vector<unsigned char> bytes(tail);in.seekg(size-tail);in.read(reinterpret_cast<char *>(bytes.data()),tail);if(!in)invalid();
 std::size_t end=tail;for(std::size_t i=tail-22;;--i){if(u32(bytes.data()+i)==0x06054b50&&i+22+u16(bytes.data()+i+20)==tail){end=i;break;}if(i==0)break;}if(end==tail)invalid();auto *p=bytes.data()+end;
 const auto count=u16(p+10);const std::uint64_t offset=u32(p+16),length=u32(p+12);if(u16(p+4)||u16(p+6)||u16(p+8)!=count||count==65535||count>o.maximum_entries||offset==0xffffffffu||offset+length!=size-tail+end)invalid();
 in.clear();in.seekg(offset);std::uint64_t expanded=0,used=0;std::array<unsigned char,46> header{};
 for(unsigned i=0;i<count;++i){in.read(reinterpret_cast<char *>(header.data()),header.size());if(!in||u32(header.data())!=0x02014b50||(u16(header.data()+8)&1))invalid();auto name=u16(header.data()+28),extra=u16(header.data()+30),comment=u16(header.data()+32);auto bytes=u32(header.data()+24);if(!name||name>4096||bytes==0xffffffffu||bytes>o.maximum_expanded_bytes-expanded)invalid();expanded+=bytes;used+=46ull+name+extra+comment;if(used>length)invalid();in.seekg(name+extra+comment,std::ios::cur);if(!in)invalid();}
 if(used!=length)invalid();
}
void inflate_catalog(const fs::path &file,const fs::path &tar,std::uint64_t maximum,TransferControl &control){
 std::ifstream input(file,std::ios::binary);std::ofstream output(tar,std::ios::binary);if(!input||!output)throw std::runtime_error("Cannot stage catalog archive");output.exceptions(std::ios::badbit|std::ios::failbit);
 z_stream stream{};if(inflateInit2(&stream,16+MAX_WBITS)!=Z_OK)invalid();struct End {z_stream *s;~End(){inflateEnd(s);}} end{&stream};
 std::array<unsigned char,65536> from{},to{};std::uint64_t bytes=0;int status=Z_OK;
 while(status!=Z_STREAM_END){TransferAccess::check(control);if(!stream.avail_in){input.read(reinterpret_cast<char *>(from.data()),from.size());auto got=input.gcount();if(got<=0)invalid();stream.next_in=from.data();stream.avail_in=static_cast<uInt>(got);}
  stream.next_out=to.data();stream.avail_out=to.size();status=inflate(&stream,Z_NO_FLUSH);if(status!=Z_OK&&status!=Z_STREAM_END)invalid();auto count=to.size()-stream.avail_out;if(count>maximum-bytes)invalid();bytes+=count;output.write(reinterpret_cast<char *>(to.data()),count);
 }
 if(stream.avail_in||input.peek()!=std::char_traits<char>::eof())invalid();output.close();
}
}
void extract_repository_archive(const fs::path &file,const fs::path &destination,RepositoryArchive kind,const TransferOptions &o,TransferControl &control){
 Utf8ArchiveLocale locale;
 if(!transfer_options_valid(o)||!fs::is_regular_file(file)||fs::is_symlink(file)||fs::file_size(file)>o.maximum_download_bytes||fs::exists(destination))invalid();TransferAccess::phase(control,TransferPhase::extracting);
 fs::path input=file,tar;
 if(kind==RepositoryArchive::module)zip_bounds(file,o);else{tar=file.parent_path()/"catalog-uncompressed.tar";if(fs::exists(tar))invalid();inflate_catalog(file,tar,o.maximum_expanded_bytes,control);input=tar;}
 std::unique_ptr<archive,decltype(&archive_read_free)> reader(archive_read_new(),archive_read_free);if(!reader)throw std::bad_alloc();
 if((kind==RepositoryArchive::module?archive_read_support_format_zip(reader.get()):archive_read_support_format_tar(reader.get()))!=ARCHIVE_OK||archive_read_support_filter_none(reader.get())!=ARCHIVE_OK||archive_read_open_filename(reader.get(),input.u8string().c_str(),65536)!=ARCHIVE_OK)invalid();
 fs::create_directory(destination);fs::permissions(destination,fs::perms::owner_all,fs::perm_options::replace);
 try{
  struct Seen {std::string path;bool directory,explicit_entry;};std::map<std::string,Seen> paths;std::uint64_t expanded=0;unsigned entries=0;
  auto remember=[&](const std::string &path,bool directory,bool explicit_entry){auto key=folded(path);auto found=paths.find(key);if(found!=paths.end()){if(found->second.path!=path||!directory||!found->second.directory||(explicit_entry&&found->second.explicit_entry))invalid();if(explicit_entry)found->second.explicit_entry=true;return;}if(paths.size()>=o.maximum_entries)invalid();paths.emplace(std::move(key),Seen{path,directory,explicit_entry});};
  archive_entry *entry=nullptr;int status;
  while((status=archive_read_next_header(reader.get(),&entry))!=ARCHIVE_EOF){
   TransferAccess::check(control);if(status!=ARCHIVE_OK||++entries>o.maximum_entries||archive_entry_symlink(entry)||archive_entry_hardlink(entry)||archive_entry_is_encrypted(entry)>0||archive_entry_sparse_count(entry)>0)invalid();
   auto type=archive_entry_filetype(entry);const bool directory=type==AE_IFDIR;if(!directory&&type!=AE_IFREG)invalid();auto name=safe_path(archive_entry_pathname_utf8(entry),directory);if(name.empty()){if(archive_entry_size(entry))invalid();continue;}
   if(kind==RepositoryArchive::catalog){if(name!="mods.d"&&name.rfind("mods.d/",0)!=0)invalid();if(!directory&&(name.find('/',7)!=name.npos||fs::u8path(name).extension()!=".conf"))invalid();}
   else if(name!="mods.d"&&name!="modules"&&name!="data"&&name.rfind("mods.d/",0)!=0&&name.rfind("modules/",0)!=0&&name.rfind("data/",0)!=0)invalid();
   std::size_t at=0;while((at=name.find('/',at))!=name.npos){auto parent=name.substr(0,at);remember(parent,true,false);auto path=destination/fs::u8path(parent);if(!fs::exists(path)){fs::create_directory(path);fs::permissions(path,fs::perms::owner_all,fs::perm_options::replace);}++at;}
   remember(name,directory,true);auto path=destination/fs::u8path(name);const auto declared=archive_entry_size(entry);if(declared<0||static_cast<std::uint64_t>(declared)>o.maximum_expanded_bytes-expanded)invalid();
   if(directory){if(declared)invalid();if(!fs::exists(path))fs::create_directory(path);fs::permissions(path,fs::perms::owner_all,fs::perm_options::replace);continue;}
   std::ofstream out(path,std::ios::binary);if(!out)throw std::runtime_error("Cannot stage extracted module data");out.exceptions(std::ios::badbit|std::ios::failbit);fs::permissions(path,fs::perms::owner_read|fs::perms::owner_write,fs::perm_options::replace);
   std::array<char,65536> buffer{};std::uint64_t written=0;for(;;){TransferAccess::check(control);auto count=archive_read_data(reader.get(),buffer.data(),buffer.size());if(count<0)invalid();if(!count)break;if(static_cast<std::uint64_t>(count)>o.maximum_expanded_bytes-expanded)invalid();expanded+=count;written+=count;out.write(buffer.data(),count);TransferAccess::expanded(control,expanded,entries);}
   if(archive_entry_size_is_set(entry)&&written!=static_cast<std::uint64_t>(declared))invalid();out.close();
  }
  if(archive_read_has_encrypted_entries(reader.get())>0||archive_read_close(reader.get())!=ARCHIVE_OK||!fs::is_directory(destination/"mods.d"))invalid();TransferAccess::expanded(control,expanded,entries);
 }catch(...){std::error_code ec;fs::remove_all(destination,ec);throw;}
 if(!tar.empty()){std::error_code ec;fs::remove(tar,ec);}
}
}
