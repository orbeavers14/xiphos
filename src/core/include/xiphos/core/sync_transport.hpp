// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "sync.hpp"
#include <memory>
namespace xiphos::core {
struct SyncConnection {
 std::string application,version,user,passphrase;
 bool network_acknowledged=false;
};
struct SyncNetworkEvent {char kind='E';SyncNavigation navigation;};
struct SyncNetworkEvents {std::vector<SyncNetworkEvent> events;bool overflow=false,active=false;};
// One active conduit per process, confined to its FIFO owner. Receive at most
// once per second; up to64 packets and32 copied events per poll. No raw packet
// dumps or passphrases are published. Wire session matching is not encryption.
class SyncTransport {
 struct Impl;std::unique_ptr<Impl> impl_;
public:
 SyncTransport();~SyncTransport();
 static bool available() noexcept;
 void configure(const SyncSettings &settings,const SyncConnection &connection);
 void stop() noexcept;
 bool active() const noexcept;
 SyncNetworkEvents poll();
 bool transmit(const SyncNavigation &navigation,bool received_navigation=false);
};
}
