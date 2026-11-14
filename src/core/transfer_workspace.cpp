// SPDX-License-Identifier: GPL-2.0-or-later
#include "remote_io.hpp"
#include "xiphos/core/editable_document.hpp"
#include <atomic>
#include <chrono>
#include <stdexcept>

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
}
}
