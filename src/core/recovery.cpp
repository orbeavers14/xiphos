// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/recovery.hpp"
#include "preference_file.hpp"
#include <glib.h>
#include <charconv>
#include <fstream>
#include <memory>
#include <set>
namespace xiphos::core {
namespace {
void invalid(){throw std::invalid_argument("Invalid recovery snapshot schema or bounds");}
std::string hash(const std::string &s){std::unique_ptr<gchar,decltype(&g_free)> p(g_compute_checksum_for_data(G_CHECKSUM_SHA256,reinterpret_cast<const guchar *>(s.data()),s.size()),g_free);if(!p)throw std::bad_alloc();return p.get();}
void value(std::string &out,const std::string &s){auto prefix=std::to_string(s.size())+":";if(prefix.size()+s.size()>maximum_recovery_bytes-out.size())invalid();out+=prefix;out+=s;}
void number(std::string &out,std::uint64_t n){value(out,std::to_string(n));}
struct Parser {
 const std::string &bytes;std::size_t at;
 std::string value(std::size_t maximum){auto end=bytes.find(':',at);if(end==bytes.npos||end==at||end-at>10)invalid();std::size_t n=0;auto p=std::from_chars(bytes.data()+at,bytes.data()+end,n);if(p.ec!=std::errc{}||p.ptr!=bytes.data()+end||(end-at>1&&bytes[at]=='0')||n>maximum||n>bytes.size()-end-1)invalid();at=end+1;auto s=bytes.substr(at,n);at+=n;return s;}
 std::uint64_t number(std::uint64_t max){auto s=value(20);std::uint64_t n=0;auto p=std::from_chars(s.data(),s.data()+s.size(),n);if(s.empty()||p.ec!=std::errc{}||p.ptr!=s.data()+s.size()||n>max||(s.size()>1&&s[0]=='0'))invalid();return n;}
};
void path(const std::string &s){if(s.empty()||s.size()>4096||!editable_document_text(s))invalid();}
void write(const std::filesystem::path &p,const std::string &bytes){std::ofstream out(p,std::ios::binary);out.exceptions(std::ios::badbit|std::ios::failbit);out.write(bytes.data(),static_cast<std::streamsize>(bytes.size()));out.close();if(detail::read_preference_file(p,maximum_recovery_bytes)!=bytes)throw std::runtime_error("Staged recovery differs");}
bool boundary(const std::string &s,std::size_t at){return at<=s.size()&&(at==s.size()||(static_cast<unsigned char>(s[at])&0xc0)!=0x80);}
}
void validate_recovery_snapshot(const RecoverySnapshot &s){
 validate_native_session(s.session);if(s.drafts.size()>64)invalid();std::set<std::string> ids;std::size_t bytes=0;
 for(const auto &d:s.drafts){if(d.id.empty()||d.id.size()>128||!editable_document_text(d.id)||!ids.insert(d.id).second||d.title.size()>4096||!editable_document_text(d.title)||!editable_document_text(d.text)||!editable_document_text(d.saved_text)||(d.origin_revision&&!editable_document_revision(*d.origin_revision)))invalid();
  bytes+=d.text.size()+d.saved_text.size();if(bytes>16*1024*1024||d.selection_start>d.selection_end||!boundary(d.text,d.selection_start)||!boundary(d.text,d.selection_end))invalid();
  if(d.target==RecoveryTarget::untitled){if(!d.path.empty()||!d.module.module.empty()||!d.module.reference.empty()||d.origin_revision)invalid();}
  else if(d.target==RecoveryTarget::file){path(d.path);if(!d.module.module.empty()||!d.module.reference.empty())invalid();}
  else if(d.target==RecoveryTarget::module){if(!d.path.empty()||!module_edit_arguments(d.module)||!d.origin_revision)invalid();}
  else invalid();
 }
}
std::string serialize_recovery_snapshot(const RecoverySnapshot &s){validate_recovery_snapshot(s);std::string payload;value(payload,serialize_native_session(s.session));number(payload,s.drafts.size());for(const auto &d:s.drafts){value(payload,d.id);value(payload,d.title);number(payload,static_cast<unsigned>(d.target));value(payload,d.path);value(payload,d.module.module);value(payload,d.module.reference);value(payload,d.origin_revision.value_or(""));value(payload,d.saved_text);value(payload,d.text);number(payload,d.selection_start);number(payload,d.selection_end);}auto out="XiphosRecovery:1\n"+hash(payload)+"\n"+payload;if(out.size()>maximum_recovery_bytes)invalid();return out;}
RecoverySnapshot parse_recovery_snapshot(const std::string &bytes){
 const std::string magic="XiphosRecovery:1\n";const auto offset=magic.size()+65;if(bytes.size()<offset||bytes.size()>maximum_recovery_bytes||bytes.compare(0,magic.size(),magic)||bytes[offset-1]!='\n')invalid();auto expected=bytes.substr(magic.size(),64);if(!editable_document_revision(expected)||hash(bytes.substr(offset))!=expected)invalid();Parser p{bytes,offset};RecoverySnapshot s;s.session=parse_native_session(p.value(maximum_session_bytes));auto count=p.number(64);for(std::uint64_t k=0;k<count;++k){RecoveryDraft d;d.id=p.value(128);d.title=p.value(4096);d.target=static_cast<RecoveryTarget>(p.number(2));d.path=p.value(4096);d.module.module=p.value(4096);d.module.reference=p.value(4096);auto revision=p.value(64);if(!revision.empty())d.origin_revision=std::move(revision);d.saved_text=p.value(maximum_editable_document_bytes);d.text=p.value(maximum_editable_document_bytes);d.selection_start=p.number(maximum_editable_document_bytes);d.selection_end=p.number(maximum_editable_document_bytes);s.drafts.push_back(std::move(d));}if(p.at!=bytes.size())invalid();validate_recovery_snapshot(s);s.revision=hash(bytes);return s;
}
RecoverySnapshot read_recovery_snapshot(const std::string &file){path(file);try{return parse_recovery_snapshot(detail::read_preference_file(std::filesystem::u8path(file),maximum_recovery_bytes));}catch(const std::bad_alloc &){throw;}catch(const std::exception &){auto previous=parse_recovery_snapshot(detail::read_preference_file(std::filesystem::u8path(file+".previous"),maximum_recovery_bytes));previous.recovered_from_previous=true;return previous;}}
RecoverySnapshot save_recovery_snapshot(const std::string &file,const std::optional<std::string> &expected,const RecoverySnapshot &s){
 path(file);if(expected&&!editable_document_revision(*expected))invalid();if(s.recovered_from_previous&&expected)throw std::invalid_argument("Recovered backup requires a new destination or explicit resolution before saving");auto bytes=serialize_recovery_snapshot(s);auto result=parse_recovery_snapshot(bytes);
 detail::replace_preference_file(file,[&](const std::filesystem::path &temp,const std::string &original,bool existed){
  if(existed!=expected.has_value()||(existed&&parse_recovery_snapshot(original).revision!=*expected))throw std::invalid_argument("Recovery snapshot changed; reload before saving");
  if(existed)detail::replace_preference_file(file+".previous",[&](const std::filesystem::path &backup,const std::string &prior,bool had_prior){if(had_prior)(void)parse_recovery_snapshot(prior);write(backup,original);},maximum_recovery_bytes);
  write(temp,bytes);
 },maximum_recovery_bytes);return result;
}
bool recovery_draft_source_matches(const RecoveryDraft &draft,const std::optional<std::string> &current){RecoverySnapshot s;s.drafts.push_back(draft);validate_recovery_snapshot(s);if(current&&!editable_document_revision(*current))invalid();return draft.target!=RecoveryTarget::untitled&&draft.origin_revision==current;}
}
