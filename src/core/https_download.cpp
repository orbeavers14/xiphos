// SPDX-License-Identifier: GPL-2.0-or-later
#include "remote_io.hpp"
#include "xiphos/core/editable_document.hpp"
#include <curl/curl.h>
#include <glib.h>
#include <atomic>
#include <chrono>
#include <exception>
#include <fstream>
#include <mutex>
namespace xiphos::core {
bool transfer_options_valid(const TransferOptions &o) noexcept {
 return o.remote_access_acknowledged&&!o.workspace.empty()&&o.workspace.size()<=4096&&editable_document_text(o.workspace)&&o.ca_bundle.size()<=4096&&editable_document_text(o.ca_bundle)&&o.maximum_download_bytes&&o.maximum_download_bytes<=512ull*1024*1024&&o.maximum_expanded_bytes&&o.maximum_expanded_bytes<=512ull*1024*1024&&o.maximum_entries&&o.maximum_entries<=16384&&o.timeout_seconds&&o.timeout_seconds<=300;
}
namespace detail {
namespace fs=std::filesystem;
TransferWorkspace::TransferWorkspace(const std::string &parent){
 auto directory=fs::u8path(parent);if(!fs::is_directory(directory)||fs::is_symlink(directory))throw std::invalid_argument("Transfer workspace requires an existing directory");directory=fs::canonical(directory);
 if(directory==directory.root_path())throw std::invalid_argument("Transfer workspace parent is too broad");
 static std::atomic<unsigned long> serial{0};for(unsigned attempt=0;attempt<64;++attempt){root_=directory/(".xiphos-transfer-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+"-"+std::to_string(serial++));if(fs::create_directory(root_))break;root_.clear();}if(root_.empty())throw std::runtime_error("Cannot create private transfer workspace");
 try{fs::permissions(root_,fs::perms::owner_all,fs::perm_options::replace);}catch(...){std::error_code ec;fs::remove(root_,ec);throw;}
}
TransferWorkspace::~TransferWorkspace(){std::error_code ec;fs::remove_all(root_,ec);}
namespace {
struct Sink {
 std::ofstream output;TransferControl &control;std::uint64_t limit,bytes=0,headers=0;std::exception_ptr error;
 std::unique_ptr<GChecksum,decltype(&g_checksum_free)> hash{g_checksum_new(G_CHECKSUM_SHA256),g_checksum_free};
 Sink(const fs::path &file,TransferControl &c,std::uint64_t max):output(file,std::ios::binary),control(c),limit(max){if(!output||!hash)throw std::runtime_error("Cannot create transfer file");output.exceptions(std::ios::badbit|std::ios::failbit);fs::permissions(file,fs::perms::owner_read|fs::perms::owner_write,fs::perm_options::replace);}
};
size_t write(char *data,size_t size,size_t count,void *opaque)noexcept{
 auto &sink=*static_cast<Sink *>(opaque);try{TransferAccess::check(sink.control);if(size&&count>SIZE_MAX/size)throw std::invalid_argument("Invalid transfer chunk size");auto bytes=size*count;if(bytes>sink.limit-sink.bytes)throw std::invalid_argument("Download exceeds byte limit");sink.output.write(data,static_cast<std::streamsize>(bytes));g_checksum_update(sink.hash.get(),reinterpret_cast<guchar *>(data),bytes);sink.bytes+=bytes;auto progress=sink.control.progress();TransferAccess::download(sink.control,sink.bytes,progress.download_total);return bytes;}catch(...){sink.error=std::current_exception();return 0;}
}
size_t header(char *,size_t size,size_t count,void *opaque)noexcept{auto &s=*static_cast<Sink *>(opaque);if(size&&count>SIZE_MAX/size)return 0;auto bytes=size*count;if(bytes>65536-s.headers)return 0;s.headers+=bytes;return bytes;}
int progress(void *opaque,curl_off_t total,curl_off_t,curl_off_t,curl_off_t)noexcept{auto &s=*static_cast<Sink *>(opaque);try{TransferAccess::check(s.control);if(total>static_cast<curl_off_t>(s.limit))throw std::invalid_argument("Declared download exceeds byte limit");TransferAccess::download(s.control,s.bytes,total>0?static_cast<std::uint64_t>(total):0);return 0;}catch(...){s.error=std::current_exception();return 1;}}
}
DownloadedArchive download_https(const std::string &url,const fs::path &file,const TransferOptions &o,TransferControl &control){
 if(!transfer_options_valid(o)||url.rfind("https://",0)!=0||url.size()>16384)throw std::invalid_argument("Invalid HTTPS transfer request");
 static std::once_flag initialized;std::call_once(initialized,[]{if(curl_global_init(CURL_GLOBAL_DEFAULT)!=CURLE_OK)throw std::runtime_error("Cannot initialize HTTPS transport");});
 TransferAccess::phase(control,TransferPhase::downloading);std::unique_ptr<CURL,decltype(&curl_easy_cleanup)> curl(curl_easy_init(),curl_easy_cleanup);if(!curl)throw std::bad_alloc();Sink sink(file,control,o.maximum_download_bytes);
 auto set=[&](CURLoption option,auto value){if(curl_easy_setopt(curl.get(),option,value)!=CURLE_OK)throw std::runtime_error("Required HTTPS transport option is unavailable");};
 set(CURLOPT_URL,url.c_str());
#if LIBCURL_VERSION_NUM >= 0x075500
 set(CURLOPT_PROTOCOLS_STR,"https");set(CURLOPT_REDIR_PROTOCOLS_STR,"https");
#else
 set(CURLOPT_PROTOCOLS,static_cast<long>(CURLPROTO_HTTPS));set(CURLOPT_REDIR_PROTOCOLS,static_cast<long>(CURLPROTO_HTTPS));
#endif
 set(CURLOPT_DISALLOW_USERNAME_IN_URL,1L);set(CURLOPT_SSL_VERIFYPEER,1L);set(CURLOPT_SSL_VERIFYHOST,2L);set(CURLOPT_SSLVERSION,static_cast<long>(CURL_SSLVERSION_TLSv1_2));
 set(CURLOPT_FOLLOWLOCATION,1L);set(CURLOPT_MAXREDIRS,5L);set(CURLOPT_CONNECTTIMEOUT,15L);set(CURLOPT_TIMEOUT,static_cast<long>(o.timeout_seconds));set(CURLOPT_NOSIGNAL,1L);set(CURLOPT_FAILONERROR,1L);
 set(CURLOPT_MAXFILESIZE_LARGE,static_cast<curl_off_t>(o.maximum_download_bytes));set(CURLOPT_WRITEFUNCTION,&write);set(CURLOPT_WRITEDATA,&sink);set(CURLOPT_HEADERFUNCTION,&header);set(CURLOPT_HEADERDATA,&sink);
 set(CURLOPT_XFERINFOFUNCTION,&progress);set(CURLOPT_XFERINFODATA,&sink);set(CURLOPT_NOPROGRESS,0L);set(CURLOPT_USERAGENT,"XiphosCore/1");
 if(!o.ca_bundle.empty())set(CURLOPT_CAINFO,o.ca_bundle.c_str());
 const auto status=curl_easy_perform(curl.get());if(sink.error)std::rethrow_exception(sink.error);TransferAccess::check(control);
 long response=0;curl_easy_getinfo(curl.get(),CURLINFO_RESPONSE_CODE,&response);
 if(status!=CURLE_OK||response!=200)throw std::runtime_error("HTTPS download failed or did not return a complete 200 response");
 sink.output.close();return {g_checksum_get_string(sink.hash.get()),sink.bytes};
}
}
}
