// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/sync_transport.hpp"
#include "xiphos/core/editable_document.hpp"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <stdexcept>
#if XIPHOS_ENABLE_SAFE_BIBLESYNC
#include <biblesync.hh>
#undef min
#undef _
#endif
namespace xiphos::core {
namespace {
std::atomic<bool> conduit{false};
void field(const std::string &s,std::size_t max){if(s.empty()||s.size()>max||!editable_document_text(s)||std::any_of(s.begin(),s.end(),[](unsigned char c){return c<32||c==127;}))throw std::invalid_argument("Invalid sync connection field");}
}
struct SyncTransport::Impl {
 SyncSettings settings;bool leased=false;SyncNetworkEvents pending;
 std::chrono::steady_clock::time_point next_poll{};
#if XIPHOS_ENABLE_SAFE_BIBLESYNC
 std::unique_ptr<BibleSync> sdk;
 static thread_local Impl *context;
 struct Scope {Impl *prior;explicit Scope(Impl *p):prior(context){context=p;}~Scope(){context=prior;}};
 static void callback(char kind,std::string speaker,std::string bible,std::string ref,std::string,std::string group,std::string domain,std::string,std::string) noexcept {
  auto *self=context;if(!self)return;
  try {
   if(kind=='S'){bool listen=self->settings.listen==SyncListen::all||(self->settings.listen==SyncListen::selected&&std::find(self->settings.allowed_speakers.begin(),self->settings.allowed_speakers.end(),speaker)!=self->settings.allowed_speakers.end());self->sdk->listenToSpeaker(listen,speaker);}
   if(kind!='N'&&kind!='S'&&kind!='D'&&kind!='A'&&kind!='M'&&kind!='E')return;
   if(self->pending.events.size()>=32){self->pending.overflow=true;return;}
   SyncNetworkEvent event;event.kind=kind;
   // Announcements/mismatches may contain sensitive metadata. Publish only kind.
   if(kind=='N'){event.navigation={std::move(speaker),std::move(bible),std::move(ref),std::move(group),std::move(domain)};validate_sync_navigation(event.navigation);}
   else if(kind=='S'||kind=='D'){field(speaker,128);event.navigation.speaker=std::move(speaker);}
   self->pending.events.push_back(std::move(event));
  }catch(...){self->pending.overflow=true;}
 }
#endif
 void stop() noexcept {
#if XIPHOS_ENABLE_SAFE_BIBLESYNC
  Scope scope(this);sdk.reset();
#endif
  if(leased){conduit.store(false);leased=false;}pending.events.clear();pending.overflow=false;settings={};
 }
 ~Impl(){stop();}
};
#if XIPHOS_ENABLE_SAFE_BIBLESYNC
thread_local SyncTransport::Impl *SyncTransport::Impl::context=nullptr;
#endif
SyncTransport::SyncTransport():impl_(std::make_unique<Impl>()){}
SyncTransport::~SyncTransport()=default;
bool SyncTransport::available() noexcept {return XIPHOS_ENABLE_SAFE_BIBLESYNC!=0;}
void SyncTransport::stop() noexcept {impl_->stop();}
bool SyncTransport::active() const noexcept {return impl_->leased;}
void SyncTransport::configure(const SyncSettings &settings,const SyncConnection &connection){
 stop();validate_sync_settings(settings);if(settings.mode==SyncMode::disabled)return;
 field(connection.application,64);field(connection.version,32);field(connection.user,64);field(connection.passphrase,64);
 if(!connection.network_acknowledged)throw std::invalid_argument("Explicit network acknowledgement is required");
#if XIPHOS_ENABLE_SAFE_BIBLESYNC
 bool expected=false;if(!conduit.compare_exchange_strong(expected,true))throw std::invalid_argument("A sync conduit is already active in this process");impl_->leased=true;
 try{impl_->settings=settings;impl_->sdk=std::make_unique<BibleSync>(connection.application,connection.version,connection.user);Impl::Scope scope(impl_.get());
  if(impl_->sdk->setModeWithPrivacy(static_cast<BibleSync_mode>(settings.mode),Impl::callback,connection.passphrase,sync_private_transport(settings))!=static_cast<BibleSync_mode>(settings.mode))throw std::runtime_error("BibleSync network setup failed");
  impl_->next_poll={};
 }catch(...){stop();throw;}
#else
 throw std::runtime_error("This build has no safe BibleSync transport SDK");
#endif
}
SyncNetworkEvents SyncTransport::poll(){
#if XIPHOS_ENABLE_SAFE_BIBLESYNC
 if(active()&&std::chrono::steady_clock::now()>=impl_->next_poll){impl_->next_poll=std::chrono::steady_clock::now()+std::chrono::seconds(1);Impl::Scope scope(impl_.get());BibleSync::ReceiveBounded(impl_->sdk.get(),64);}
#endif
 auto result=std::move(impl_->pending);impl_->pending={};
#if XIPHOS_ENABLE_SAFE_BIBLESYNC
 if(impl_->sdk&&impl_->sdk->getMode()==BSP_MODE_DISABLE)stop();
#endif
 result.active=active();return result;
}
bool SyncTransport::transmit(const SyncNavigation &navigation,bool received){
 validate_sync_navigation(navigation);if(!active()||!sync_transmit_allowed(impl_->settings,received))return false;
 if(navigation.group.size()!=1||navigation.group[0]<'1'||navigation.group[0]>'9'||navigation.domain!="BIBLE-VERSE"||navigation.module.size()+navigation.reference.size()>384)throw std::invalid_argument("Navigation exceeds BibleSync wire bounds or domain/group");
#if XIPHOS_ENABLE_SAFE_BIBLESYNC
 Impl::Scope scope(impl_.get());auto status=impl_->sdk->Transmit(navigation.module,navigation.reference,"",navigation.group,navigation.domain);if(status!=BSP_XMIT_OK){stop();throw std::runtime_error("BibleSync transmission failed; conduit disabled");}return true;
#else
 return false;
#endif
}
}
