// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/sync.hpp"
#include "xiphos/core/editable_document.hpp"
#include <algorithm>
#include <set>
#include <stdexcept>
namespace xiphos::core {
namespace {
void text(const std::string &s,std::size_t max,bool required=false){if((required&&s.empty())||s.size()>max||!editable_document_text(s)||std::any_of(s.begin(),s.end(),[](unsigned char c){return c<32||c==127;}))throw std::invalid_argument("Invalid sync input bounds/text");}
}
void validate_sync_settings(const SyncSettings &s){if(static_cast<int>(s.mode)<0||static_cast<int>(s.mode)>3||static_cast<int>(s.listen)<0||static_cast<int>(s.listen)>2||s.allowed_speakers.size()>256)throw std::invalid_argument("Invalid sync settings");text(s.group,128);text(s.domain,128);std::set<std::string> ids;for(const auto &id:s.allowed_speakers){text(id,128,true);if(!ids.insert(id).second)throw std::invalid_argument("Duplicate sync speaker");}}
void validate_sync_navigation(const SyncNavigation &n){text(n.speaker,128,true);text(n.module,4096,true);text(n.reference,4096,true);text(n.group,128);text(n.domain,128);}
bool sync_transmit_allowed(const SyncSettings &s,bool received){validate_sync_settings(s);return !received&&(s.mode==SyncMode::personal||s.mode==SyncMode::speaker);}
bool sync_private_transport(const SyncSettings &s){validate_sync_settings(s);return s.mode==SyncMode::personal&&s.private_personal;}
SyncDecision plan_sync_navigation(const SyncSettings &s,const SyncNavigation &n,const std::optional<std::string> &resolved){
 validate_sync_settings(s);validate_sync_navigation(n);if(resolved)text(*resolved,4096,true);SyncDecision result;
 if((s.mode!=SyncMode::personal&&s.mode!=SyncMode::audience)||s.listen==SyncListen::none||(!s.group.empty()&&n.group!=s.group)||(!s.domain.empty()&&n.domain!=s.domain)||(s.listen==SyncListen::selected&&std::find(s.allowed_speakers.begin(),s.allowed_speakers.end(),n.speaker)==s.allowed_speakers.end()))return result;
 result.navigation=n;result.route=s.direct_navigation&&resolved&&n.reference.find_first_of("-;,")==std::string::npos?SyncRoute::direct:SyncRoute::reference_list;
 if(resolved)result.navigation.module=*resolved;
 if(result.route==SyncRoute::direct&&s.group_tabs&&n.group.size()==1&&n.group[0]>='1'&&n.group[0]<='9')result.tab_index=static_cast<unsigned>(n.group[0]-'1');
 return result;
}
}
