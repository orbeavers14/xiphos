// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/reader.hpp"
#include <markupfiltmgr.h>
#include <swmgr.h>
#include <swmodule.h>
#include <versekey.h>
#include <filesystem>
#include <mutex>
#include <stdexcept>

namespace xiphos::core {
struct Reader::Impl {
  std::unique_ptr<sword::SWMgr> manager;
  std::vector<Module> modules;
  std::mutex mutex;
};
Reader::Reader(const std::string &root) : impl_(std::make_unique<Impl>()) {
  if (!root.empty() && !std::filesystem::is_directory(root))
    throw std::invalid_argument("Module root must be an existing directory");
  // SWMgr takes ownership of the filter manager, matching the legacy backend.
  impl_->manager = std::make_unique<sword::SWMgr>(
    root.empty() ? nullptr : root.c_str(), true,
    new sword::MarkupFilterMgr(sword::FMT_XHTML), false, root.empty());
  for (const auto &item : impl_->manager->Modules) {
    auto &m = *item.second;
    const char *language = m.getConfigEntry("Lang");
    impl_->modules.push_back({m.getName(), m.getDescription(), m.getType(),
                             language ? language : ""});
  }
}
Reader::~Reader() = default;
const std::vector<Module> &Reader::modules() const { return impl_->modules; }
Entry Reader::read(const std::string &id, const std::string &reference) {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  if (reference.empty()) throw std::invalid_argument("Reference is empty");
  auto *module = impl_->manager->getModule(id.c_str());
  if (!module) throw std::out_of_range("Module is not installed");
  if (!dynamic_cast<sword::VerseKey *>(module->getKey()))
    throw std::invalid_argument("This slice only supports verse-key modules");
  // Resolve using the module's own versification and SWORD normalization,
  // never an independent canon/reference parser in a frontend.
  Entry result;
  result.html = render_entry(*module, reference.c_str());
  if (module->getKey()->getError())
    throw std::invalid_argument("SWORD could not resolve the reference");
  result.reference = module->getKeyText();
  return result;
}
}
