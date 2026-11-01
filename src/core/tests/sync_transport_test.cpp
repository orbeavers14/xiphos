// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/sync_transport.hpp"
#include "xiphos/core/c_api.h"
#include <filesystem>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <array>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <thread>
using namespace xiphos::core;
static int sink_port=0,datagrams=0;static bool expect_private=true,wrong_ttl=false;
// Test executable interposition only. Every SDK send goes to our ephemeral
// loopback sink via sendmsg; no fixture beacon/presence can reach the LAN.
extern "C" ssize_t sendto(int fd,const void *bytes,size_t size,int flags,const sockaddr *,socklen_t){
 int ttl=-1;socklen_t length=sizeof(ttl);getsockopt(fd,IPPROTO_IP,IP_MULTICAST_TTL,&ttl,&length);++datagrams;if(expect_private&&ttl!=0)wrong_ttl=true;
 sockaddr_in destination{};destination.sin_family=AF_INET;destination.sin_port=htons(sink_port);destination.sin_addr.s_addr=htonl(INADDR_LOOPBACK);iovec part{const_cast<void *>(bytes),size};msghdr msg{};msg.msg_name=&destination;msg.msg_namelen=sizeof(destination);msg.msg_iov=&part;msg.msg_iovlen=1;return sendmsg(fd,&msg,flags);
}
static void check(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
template<class F>void rejects(F f){try{f();}catch(const std::exception &){return;}throw std::runtime_error("Expected rejection");}
static void peer(int fd,unsigned kind,const std::string &phrase){
 std::string body="app.name=Original fixture peer\napp.inst.uuid=11111111-1111-4111-8111-111111111111\napp.user=Original fixture\nmsg.sync.passPhrase="+phrase+"\n";
 if(kind==2)body+="msg.sync.bibleAbbrev=Original\nmsg.sync.domain=BIBLE-VERSE\nmsg.sync.verse=John.3.16\nmsg.sync.group=2\n";
 std::string packet(32,'\0');packet[0]=0x40;packet[1]=static_cast<char>(0x9c);packet[2]=static_cast<char>(0xaf);packet[3]=0x11;packet[4]=3;packet[5]=static_cast<char>(kind);std::uint16_t count=1;std::memcpy(packet.data()+6,&count,2);std::memset(packet.data()+16,0x11,16);packet[22]=0x41;packet[24]=static_cast<char>(0x81);packet+=body;
 sockaddr_in to{};to.sin_family=AF_INET;to.sin_port=htons(22272);to.sin_addr.s_addr=htonl(INADDR_LOOPBACK);iovec part{packet.data(),packet.size()};msghdr msg{};msg.msg_name=&to;msg.msg_namelen=sizeof(to);msg.msg_iov=&part;msg.msg_iovlen=1;check(sendmsg(fd,&msg,0)==static_cast<ssize_t>(packet.size()),"Loopback peer send");
}
static void pause_poll(){std::this_thread::sleep_for(std::chrono::milliseconds(1050));}
int main(int argc,char **argv){try{
 SyncTransport transport;SyncSettings settings;SyncConnection connection{"Original fixture","1","Original fixture","Original session phrase",true};
 if(!SyncTransport::available()){settings.mode=SyncMode::personal;rejects([&]{transport.configure(settings,connection);});check(!transport.active(),"Unavailable transport stays disabled");return 0;}
 // Do not join an existing user's BibleSync socket in this fixture.
 int probe=socket(AF_INET,SOCK_DGRAM,0);sockaddr_in address{};address.sin_family=AF_INET;address.sin_port=htons(22272);address.sin_addr.s_addr=htonl(INADDR_ANY);if(bind(probe,reinterpret_cast<sockaddr *>(&address),sizeof(address))<0){close(probe);std::cout<<"BibleSync fixture port already in use\n";return 77;}close(probe);
 int sink=socket(AF_INET,SOCK_DGRAM,0);address.sin_port=0;address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);check(bind(sink,reinterpret_cast<sockaddr *>(&address),sizeof(address))==0,"Bind ephemeral loopback sink");socklen_t length=sizeof(address);getsockname(sink,reinterpret_cast<sockaddr *>(&address),&length);sink_port=ntohs(address.sin_port);int sender=socket(AF_INET,SOCK_DGRAM,0);
 settings.mode=SyncMode::personal;settings.listen=SyncListen::all;transport.configure(settings,connection);check(transport.active()&&datagrams>=2&&!wrong_ttl,"Private TTL applies before initial beacon and presence");
 SyncTransport second;rejects([&]{second.configure(settings,connection);});check(transport.active()&&!second.active(),"One active conduit per process");(void)transport.poll();peer(sender,3,connection.passphrase);pause_poll();auto events=transport.poll();check(events.active,"Poll reports active conduit");bool joined=false;for(const auto &event:events.events)joined=joined||event.kind=='S';check(joined,"Actual UDP beacon accepted by SDK");peer(sender,2,connection.passphrase);pause_poll();events=transport.poll();bool navigated=false;for(const auto &event:events.events)if(event.kind=='N')navigated=event.navigation.reference=="John.3.16"&&event.navigation.module=="Original";check(navigated,"Actual UDP navigation copied from official SDK callback");
 peer(sender,2,"Different original phrase");pause_poll();events=transport.poll();for(const auto &event:events.events)check(event.kind!='N',"Wrong session phrase cannot navigate");
 SyncNavigation navigation{"local","Original","John.3.16","2","BIBLE-VERSE"};check(!transport.transmit(navigation,true)&&transport.transmit(navigation),"Echo suppression and actual outbound UDP");check(!wrong_ttl,"Subsequent private sends retain TTL0");navigation.reference=std::string(500,'a');rejects([&]{transport.transmit(navigation);});transport.stop();check(!transport.active()&&!transport.transmit({"local","Original","John.3.16","2","BIBLE-VERSE"}),"Disable stops sending");
 // Failed reconfiguration disables a previous public/private connection.
 transport.configure(settings,connection);connection.network_acknowledged=false;rejects([&]{transport.configure(settings,connection);});check(!transport.active(),"Failed configure leaves conduit disabled");check(argc==2,"Named fixture root required");auto root=std::filesystem::u8path(argv[1]);check(root.filename()=="sync-network-fixture","Named disposable root only");std::filesystem::remove_all(root);std::filesystem::create_directories(root/"mods.d");
 {xc_worker *worker=nullptr;check(xc_worker_open(root.u8string().c_str(),&worker)==XC_OK,"C network worker");xc_request *request=nullptr;
 auto wait=[](xc_request *r){auto until=std::chrono::steady_clock::now()+std::chrono::seconds(10);xc_status status;while((status=xc_request_poll(r))==XC_PENDING){check(std::chrono::steady_clock::now()<until,"C request deadline");std::this_thread::sleep_for(std::chrono::milliseconds(1));}return status;};
 xc_sync_settings config{1,2,1,1,0,nullptr,nullptr,nullptr,0};std::string phrase="Original C fixture phrase";xc_sync_connection conn{"Original fixture","1","Original fixture",phrase.c_str(),1};
 check(xc_worker_configure_sync_network(worker,&config,&conn,&request)==XC_OK,"C configure admitted");phrase="Changed after admission";check(wait(request)==XC_OK,"C copied configure succeeds");xc_request_release(request);
 peer(sender,3,"Original C fixture phrase");peer(sender,2,"Original C fixture phrase");check(xc_worker_poll_sync_network(worker,&request)==XC_OK&&wait(request)==XC_OK,"C network poll");auto owned=xc_request_take_sync_network_events(request);check(owned&&xc_sync_network_events_active(owned)&&!xc_request_take_sync_network_events(request),"Take active event snapshot once");xc_request_release(request);xc_worker_close(worker);
 bool copied=false;for(size_t i=0;i<xc_sync_network_event_count(owned);++i){char kind=0;xc_sync_navigation n{};check(xc_sync_network_event_get(owned,i,&kind,&n)==XC_OK,"Owned event view");if(kind=='N')copied=std::string(n.reference)=="John.3.16";}check(copied,"C navigation survives worker teardown and input mutation");xc_sync_network_events_release(owned);}
 std::filesystem::remove_all(root);check(!wrong_ttl,"C worker first sends are also private");
 close(sender);close(sink);std::cout<<"Safe BibleSync real loopback UDP checks passed\n";return 0;
 }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
