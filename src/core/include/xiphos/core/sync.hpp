// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <optional>
#include <string>
#include <vector>
namespace xiphos::core {
enum class SyncMode {disabled=0,personal,speaker,audience};
enum class SyncListen {none=0,selected,all};
struct SyncSettings {
 SyncMode mode=SyncMode::disabled;
 SyncListen listen=SyncListen::none;
 bool private_personal=true,direct_navigation=true,group_tabs=false;
 std::string group,domain;
 std::vector<std::string> allowed_speakers;
};
struct SyncNavigation {std::string speaker,module,reference,group,domain;};
enum class SyncRoute {ignore=0,direct,reference_list};
struct SyncDecision {
 SyncRoute route=SyncRoute::ignore;
 SyncNavigation navigation;
 std::optional<unsigned> tab_index;
};
void validate_sync_settings(const SyncSettings &settings);
void validate_sync_navigation(const SyncNavigation &navigation);
// Transport-neutral application policy. A BibleSync adapter must perform wire
// validation/passphrase/session checks before admitting incoming navigation here.
// Personal privacy means host-only multicast TTL, not message encryption.
// No passphrases, sockets, GTK callbacks or persisted network opt-in live here.
bool sync_transmit_allowed(const SyncSettings &settings,bool received_navigation);
bool sync_private_transport(const SyncSettings &settings);
SyncDecision plan_sync_navigation(const SyncSettings &settings,const SyncNavigation &navigation,
 const std::optional<std::string> &resolved_installed_module);
}
