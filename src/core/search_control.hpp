// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "xiphos/core/search.hpp"
#include <stdexcept>
namespace xiphos::core::detail {
class SearchControlAccess {
public:
  static bool begin(SearchControl &control) noexcept {
    auto expected=SearchPhase::queued;
    return control.phase_.compare_exchange_strong(expected,SearchPhase::running,std::memory_order_acq_rel);
  }
  static void progress(SearchControl &control,unsigned value) noexcept {
    if(value>100)value=100;
    auto prior=control.percent_.load(std::memory_order_relaxed);
    while(prior<value&&!control.percent_.compare_exchange_weak(prior,value,std::memory_order_relaxed)){}
  }
  static void finish(SearchControl &control,bool cancelled) noexcept {
    if(!cancelled)progress(control,100);
    control.phase_.store(cancelled?SearchPhase::cancelled:SearchPhase::complete,std::memory_order_release);
  }
  static void fail_if_queued(const std::shared_ptr<SearchControl> &control) noexcept {
    if(!control)return;auto expected=SearchPhase::queued;
    control->phase_.compare_exchange_strong(expected,SearchPhase::failed,std::memory_order_acq_rel);
  }
  static void failed(SearchControl &control) noexcept {control.phase_.store(SearchPhase::failed,std::memory_order_release);}
};
class SearchControlScope {
  std::shared_ptr<SearchControl> control_;bool finished_=false;
public:
  explicit SearchControlScope(std::shared_ptr<SearchControl> control):control_(std::move(control)) {
    if(control_&&!SearchControlAccess::begin(*control_))throw std::invalid_argument("Search control was already used");
  }
  ~SearchControlScope(){if(control_&&!finished_)SearchControlAccess::failed(*control_);}
  void finish(bool cancelled) noexcept{if(control_)SearchControlAccess::finish(*control_,cancelled);finished_=true;}
};
}
