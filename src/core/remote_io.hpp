// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "xiphos/core/remote_install.hpp"
#include <filesystem>
namespace xiphos::core::detail {
struct TransferAccess {
 static void check(TransferControl &c){if(c.gate_.load()==1)throw TransferCancelled();}
 static void phase(TransferControl &c,TransferPhase p){check(c);c.phase_=p;}
 static void download(TransferControl &c,std::uint64_t bytes,std::uint64_t total){c.downloaded_=bytes;c.total_=total;}
 static void expanded(TransferControl &c,std::uint64_t bytes,std::uint64_t entries){c.expanded_=bytes;c.entries_=entries;}
 static void commit(TransferControl &c){unsigned expected=0;if(!c.gate_.compare_exchange_strong(expected,2))throw TransferCancelled();c.phase_=TransferPhase::committing;}
 static void finish(TransferControl &c){unsigned expected=0;if(!c.gate_.compare_exchange_strong(expected,3)){if(expected==1)throw TransferCancelled();c.gate_=3;}c.phase_=TransferPhase::complete;}
 static void fail(TransferControl &c){const bool cancelled=c.gate_.exchange(3)==1;c.phase_=cancelled?TransferPhase::cancelled:TransferPhase::failed;}
};
class TransferWorkspace {
 std::filesystem::path root_;
public:
 explicit TransferWorkspace(const std::string &parent);
 ~TransferWorkspace();
 const std::filesystem::path &root()const{return root_;}
 TransferWorkspace(const TransferWorkspace &)=delete;
};
struct DownloadedArchive {std::string sha256;std::uint64_t bytes=0;};
DownloadedArchive download_https(const std::string &url,const std::filesystem::path &file,const TransferOptions &options,TransferControl &control);
enum class RepositoryArchive {catalog,module};
void extract_repository_archive(const std::filesystem::path &archive,const std::filesystem::path &destination,RepositoryArchive kind,const TransferOptions &options,TransferControl &control);
}
