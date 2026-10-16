// SPDX-License-Identifier: GPL-2.0-or-later
#include "sdk_render.hpp"
#include <swmodule.h>
namespace xiphos { namespace core { namespace detail {
namespace {
class RestoreAttributeProcessing {
  sword::SWModule &module_;
  bool previous_;
public:
  explicit RestoreAttributeProcessing(sword::SWModule &module)
      : module_(module), previous_(module.isProcessEntryAttributes()) {}
  ~RestoreAttributeProcessing() {
    if (module_.isProcessEntryAttributes() != previous_) module_.setProcessEntryAttributes(previous_);
  }
  RestoreAttributeProcessing(const RestoreAttributeProcessing &) = delete;
  RestoreAttributeProcessing &operator=(const RestoreAttributeProcessing &) = delete;
};
}
sword::SWBuf render_sdk_entry(sword::SWModule &module) {
  RestoreAttributeProcessing scope(module);
  return module.renderText();
}
sword::SWBuf render_sdk_buffer(sword::SWModule &module, const char *text) {
  RestoreAttributeProcessing scope(module);
  return module.renderText(text);
}
}}}
