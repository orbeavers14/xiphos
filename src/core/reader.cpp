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
  sword::SWModule &verse_module(const std::string &id, const std::string &reference) {
    if (reference.empty()) throw std::invalid_argument("Reference is empty");
    auto *module = manager->getModule(id.c_str());
    if (!module) throw std::out_of_range("Module is not installed");
    if (!dynamic_cast<sword::VerseKey *>(module->getKey()))
      throw std::invalid_argument("This slice only supports verse-key modules");
    return *module;
  }
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
PassageLink Reader::resolve_link(const std::string &current_module, const std::string &url) {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  auto target = passage_link(url, current_module);
  impl_->verse_module(target.module, target.reference);
  return target;
}
Entry Reader::read(const std::string &id, const std::string &reference) {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  auto *module = &impl_->verse_module(id, reference);
  // Resolve using the module's own versification and SWORD normalization,
  // never an independent canon/reference parser in a frontend.
  Entry result;
  result.html = render_entry(*module, reference.c_str());
  if (module->getKey()->getError())
    throw std::invalid_argument("SWORD could not resolve the reference");
  result.reference = module->getKeyText();
  return result;
}
Chapter Reader::read_chapter(const std::string &id, const std::string &reference,
                             const DisplayOptions &options) {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  auto &module = impl_->verse_module(id, reference);
  // Clone the module key so its own locale/versification drives enumeration.
  // Work on this clone, avoiding persistent changes to the module key flags.
  sword::VerseKey key(*static_cast<sword::VerseKey *>(module.getKey()));
  key.setAutoNormalize(true);
  key.setText(reference.c_str());
  if (key.getError() || key.getChapter() < 1 || key.getVerse() < 1)
    throw std::invalid_argument("SWORD could not resolve a chapter");
  Chapter result;
  result.reference = key.getText();
  result.selected_verse = key.getVerse();
  result.selected_anchor = "verse-" + std::to_string(result.selected_verse);
  result.title = std::string(key.getBookName()) + " " + std::to_string(key.getChapter());
  key.setSuffix(0);
  const auto count = key.getVerseMax();
  if (count <= 0) throw std::invalid_argument("Chapter has no verse positions");
  result.verses.reserve(count);
  for (int verse = 1; verse <= count; ++verse) {
    key.setVerse(verse);
    const std::string resolved = key.getText();
    result.verses.push_back({resolved, render_entry(module, resolved.c_str())});
  }
  // Keep the module at the requested anchor, as with a single-entry read.
  module.setKey(result.reference.c_str());
  result.html = display_chapter(result, options);
  return result;
}
std::string Reader::adjacent_chapter(const std::string &id, const std::string &reference,
                                    int direction) {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  if (direction != -1 && direction != 1) throw std::invalid_argument("Invalid chapter direction");
  auto &module = impl_->verse_module(id, reference);
  sword::VerseKey key(*static_cast<sword::VerseKey *>(module.getKey()));
  key.setIntros(false);
  key.setAutoNormalize(true);
  key.setText(reference.c_str());
  if (key.getError()) throw std::invalid_argument("SWORD could not resolve the reference");
  key.setSuffix(0);
  key.setVerse(1);
  const std::string before = key.getText();
  key.setChapter(key.getChapter() + direction);
  if (key.getError() || before == key.getText())
    throw std::invalid_argument("Chapter destination is unavailable");
  return key.getText();
}
}
