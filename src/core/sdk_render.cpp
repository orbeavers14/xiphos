// SPDX-License-Identifier: GPL-2.0-or-later
#include "sdk_render.hpp"
#include <swmodule.h>
#include <type_traits>
namespace xiphos { namespace core { namespace detail {
namespace {
// Released 1.9 stripText wraps renderText(text, -1, false) in a static
// pointer-returning buffer. Use its sized operation before NULs lose visibility.
sword::SWBuf strip_buffer(sword::SWModule &module, const char *text, std::true_type) {
  return module.renderText(text, -1, false);
}
sword::SWBuf strip_buffer(sword::SWModule &module, const char *text, std::false_type) {
  return module.stripText(text);
}
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
sword::SWBuf strip_sdk_buffer(sword::SWModule &module, const char *text) {
  RestoreAttributeProcessing scope(module);
  return strip_buffer(module, text, std::is_same<decltype(module.stripText(text)), const char *>{});
}
}}}
