// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/worker.hpp"
#include "xiphos/core/c_api.h"
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>
using namespace xiphos::core;
void check(bool ok,const char *message) { if(!ok) throw std::runtime_error(message); }
template<class E,class F> void rejects(F call) { try { call(); } catch(const E &) { return; } throw std::runtime_error("Expected rejected alias search"); }
xc_status wait(xc_request *r) { xc_status status;const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(15);while((status=xc_request_poll(r))==XC_PENDING){check(std::chrono::steady_clock::now()<end,"Alias search deadline");std::this_thread::sleep_for(std::chrono::milliseconds(1));}return status; }
int main(int argc,char **argv) { try {
 check(argc==2,"Generated alias fixture required");
 const std::string root=argv[1];
 {
  Reader reader(root); StudySession session(reader),other(reader);session.visit("AFirst","John1:1");
  SearchQuery query{"Shared","Original"};query.range=SearchRange{"AFirst","John1:1"};
  const auto result=session.search_module(query);
  check(result.requested.module=="Shared" && result.resolved_module=="BSecond" && result.source_module()=="BSecond" && result.requested.range->module=="AFirst" && result.references==std::vector<std::string>{"John 1:1"},"Alias execution preserves raw request and explicit provider with actual source");
  const auto retained=session.read_search_generation(result.generation.id,0);
  check(retained.requested->module=="Shared" && retained.result->source_module()=="BSecond","Retained generation owns both identities");
  SearchTextExportRequest visible;visible.generation=result.generation.id;visible.rows={{0,0}};visible.include_scripture=true;visible.include_search_context=true;
  const auto text=session.export_search_visible_text(visible);
  check(text.status==SearchExportStatus::ok&&text.text.find("Module: Shared\nResolved module: BSecond")!=std::string::npos && text.text.find("Original alias fixture prose for BSecond")!=std::string::npos&&text.text.find("prose for AFirst")==std::string::npos,"Visible text exports actual source content and discloses requested/resolved provenance");
  SearchHtmlExportRequest html;html.generation=result.generation.id;html.rows={{0,0}};html.include_scripture=true;html.include_search_context=true;html.plain_text_max_bytes=65536;
  const auto document=session.export_search_html(html);
  check(document.status==SearchExportStatus::ok && document.html.find("prose for BSecond")!=std::string::npos && document.plain_text&&document.plain_text->find("Resolved module: BSecond")!=std::string::npos,"HTML and rich companion use identical actual source identity");
  auto badProvider=query;badProvider.range->module="Shared";
  rejects<std::out_of_range>([&]{session.search_module(badProvider);});
  check(session.search_generation().id==result.generation.id,"Execution aliases never silently resolve the explicit bounds provider");
  std::vector<SearchQuery> queries={{"Shared","Original"},{"AFirst","Original"},{"Shared","Original"},{"Missing","Original"},{"文字 café","Original"}};
  const auto batch=session.search_batch(queries);
  check(batch.failures==1&&batch.groups[3].failure==SearchBatchFailure::module_not_found&&batch.requested[0].module=="Shared"&&batch.groups[0].result->source_module()=="BSecond"&&batch.groups[1].result->source_module()=="AFirst"&&batch.groups[2].result->source_module()=="BSecond"&&batch.groups[4].result->source_module()=="Unicode","Ordered alias batches preserve duplicate requests and actual per-group identities");
  SearchAllTextExportRequest all;all.generation=batch.generation.id;all.include_scripture=true;all.include_search_context=true;
  check(session.export_search_all_text(all).status==SearchExportStatus::source_incomplete,"Alias failure still requires explicit partial-source export");
  all.allow_partial_source=true;const auto complete=session.export_search_all_text(all);
  check(complete.status==SearchExportStatus::ok&&complete.row_count==4&&complete.text.find("prose for BSecond")!=std::string::npos&&complete.text.find("prose for AFirst")!=std::string::npos&&complete.text.find("prose for Unicode")!=std::string::npos&&complete.text.find("Module: Missing")!=std::string::npos,"All retained rows read actual modules while source summary retains failed raw requests");
  html.generation=batch.generation.id;html.selection=SearchExportSelection::all_retained;html.rows.clear();html.allow_partial_source=true;
  check(session.export_search_html(html).status==SearchExportStatus::ok,"All retained HTML shares resolved row selection");
  auto chained=queries[0];chained.within_last_results=true;const auto within=session.search_module(chained);
  check(within.source_module()=="BSecond"&&within.scope.element_count==1&&within.requested.within_last_results,"Alias execution reuses complete session last scope");
  auto stopped=std::make_shared<SearchControl>();stopped->request_cancel();const auto cancelled=session.search_module(query,{},stopped);
  check(cancelled.cancelled&&!cancelled.executed&&cancelled.requested.module=="Shared"&&cancelled.source_module()=="BSecond","Pre-cancelled resolved result retains authoritative source without execution");
  check(session.history().entries().size()==1&&other.history().entries().empty()&&!other.last_search_scope().present,"Alias execution/export does not visit reading history or another tab");
 }
 {
  Reader reader(root+"/bad-metadata");StudySession session(reader);
  check(session.search_module({"Bad","Original"}).source_module()=="Bad","Exact-ID search bypasses unrelated invalid alias catalog");
  const auto id=session.search_generation().id;
  rejects<std::invalid_argument>([&]{session.search_module({"UnknownAlias","Original"});});
  check(session.search_generation().id==id,"Bad alias metadata fails without replacing committed generation");
 }
 xc_worker *worker=nullptr;xc_request *request=nullptr;
 check(xc_worker_open(root.c_str(),&worker)==XC_OK&&xc_worker_create_tab(worker,&request)==XC_OK&&wait(request)==XC_OK,"Create C alias search owner");const auto tab=xc_request_tab(request);xc_request_release(request);
 check(xc_worker_search_module(worker,tab,"Shared","Original",nullptr,&request)==XC_OK&&wait(request)==XC_OK,"C search resolves alias on same worker");auto *held=xc_request_take_search(request);xc_request_release(request);
 check(held&&std::string(xc_search_module(held))=="Shared"&&std::string(xc_search_source_module(held))=="BSecond"&&!xc_search_source_module(nullptr),"Additive C source getter preserves requested getter contract");
 check(xc_worker_search_module(worker,tab,"文字 café","Original",nullptr,&request)==XC_OK,"Accept Unicode alias search before shutdown");xc_worker_close(worker);
 check(wait(request)==XC_OK,"Shutdown drains alias search");auto *drained=xc_request_take_search(request);xc_request_release(request);
 check(drained&&std::string(xc_search_source_module(drained))=="Unicode"&&std::string(xc_search_source_module(held))=="BSecond","Owned actual sources survive request/worker shutdown");xc_search_release(held);xc_search_release(drained);
 std::cout<<"Alias search execution, raw/resolved provenance, retained source/export identity and lifecycle passed\n";return 0;
 } catch(const std::exception &error) { std::cerr<<error.what()<<'\n';return 1; } }
