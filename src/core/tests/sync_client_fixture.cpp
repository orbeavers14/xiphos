// SPDX-License-Identifier: GPL-2.0-or-later
// Two-process fixture only: redirect every SDK send/bind to isolated loopback
// ports. Production transport is unchanged; no test packet reaches the LAN.
#include "xiphos/core/worker.hpp"
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <dlfcn.h>
#include <cerrno>
#include <iostream>
#include <sstream>
#include <stdexcept>
using namespace xiphos::core;
static unsigned relay_port=0,receive_port=0;
static bool fail_send=false;
extern "C" int bind(int fd,const sockaddr *address,socklen_t size){
 using Function=int (*)(int,const sockaddr *,socklen_t);
 static auto real=reinterpret_cast<Function>(dlsym(RTLD_NEXT,"bind"));
 if(!real){errno=ENOSYS;return -1;}
 if(address&&address->sa_family==AF_INET&&size>=sizeof(sockaddr_in)&&ntohs(reinterpret_cast<const sockaddr_in *>(address)->sin_port)==22272){
  auto local=*reinterpret_cast<const sockaddr_in *>(address);local.sin_port=htons(receive_port);local.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
  return real(fd,reinterpret_cast<sockaddr *>(&local),sizeof(local));
 }
 return real(fd,address,size);
}
extern "C" ssize_t sendto(int fd,const void *bytes,size_t size,int flags,const sockaddr *,socklen_t){
 if(fail_send){errno=ENETDOWN;return -1;}
 sockaddr_in destination{};destination.sin_family=AF_INET;destination.sin_port=htons(relay_port);destination.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
 iovec part{const_cast<void *>(bytes),size};msghdr msg{};msg.msg_name=&destination;msg.msg_namelen=sizeof(destination);msg.msg_iov=&part;msg.msg_iovlen=1;return sendmsg(fd,&msg,flags);
}
int main(int argc,char **argv){try{
 if(argc==2&&std::string(argv[1])=="--available")return SyncTransport::available()?0:77;
 if(argc!=5)throw std::invalid_argument("Fixture arguments");
 if(!SyncTransport::available()){std::cout<<"UNAVAILABLE"<<std::endl;return 77;}
 receive_port=static_cast<unsigned>(std::stoul(argv[2]));relay_port=static_cast<unsigned>(std::stoul(argv[3]));
 if(!receive_port||receive_port>65535||!relay_port||relay_port>65535)throw std::invalid_argument("Fixture ports");
 StudyWorker worker(argv[1]);std::cout<<"READY"<<std::endl;
 std::string line;
 while(std::getline(std::cin,line)){
  std::istringstream input(line);std::string command;input>>command;
  try{
   if(command=="configure"){
    int mode=0,listen=0;std::string phrase;input>>mode>>listen>>phrase;
    SyncSettings settings;settings.mode=static_cast<SyncMode>(mode);settings.listen=static_cast<SyncListen>(listen);
    worker.configure_sync_network(settings,{"Original pair fixture","1",argv[4],phrase,true}).get();std::cout<<"OK";
   }else if(command=="poll"){
    auto batch=worker.poll_sync_network().get();std::cout<<"POLL "<<batch.active<<' '<<batch.overflow;
    for(const auto &event:batch.events)if(event.kind=='N'){
     auto decision=worker.plan_incoming_sync(event.navigation).get();
     std::cout<<" NAV:"<<event.navigation.reference<<":"<<static_cast<int>(decision.route);
    }
   }else if(command=="send"||command=="echo"){
    std::string reference;input>>reference;
    auto sent=worker.transmit_sync_network({"local","Original",reference,"1","BIBLE-VERSE"},command=="echo").get();std::cout<<"SENT "<<sent;
   }else if(command=="fail"){fail_send=true;std::cout<<"OK";
   }else if(command=="repair"){fail_send=false;std::cout<<"OK";
   }else if(command=="quit"){worker.configure_sync({}).get();std::cout<<"BYE"<<std::endl;return 0;
   }else throw std::invalid_argument("Fixture command");
  }catch(const std::exception &){std::cout<<"ERROR";}
  std::cout<<std::endl;
 }
 return 0;
 }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
