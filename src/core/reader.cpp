#include "xiphos/core/dictionary.hpp"
// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/reader.hpp"
#include "xiphos/core/reference_list.hpp"
#include <markupfiltmgr.h>
#include "xiphos/core/engine_manager.hpp"
#include <swmgr.h>
#include <swmodule.h>
#include <swld.h>
#include <versekey.h>
#include <filesystem>
#include <mutex>
#include <stdexcept>

namespace xiphos::core {
struct Reader::Impl {
  std::unique_ptr<EngineManager> manager;
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
  impl_->manager = std::make_unique<EngineManager>(
    root.empty() ? nullptr : root.c_str(), true,
    new sword::MarkupFilterMgr(sword::FMT_XHTML), false, root.empty());
  bool transliteration_available = false;
  for (const auto &name : impl_->manager->getGlobalOptions())
    if (name == "Transliteration") transliteration_available = true;
  for (const auto &item : impl_->manager->Modules) {
    auto &m = *item.second;
    const char *language = m.getConfigEntry("Lang");
    const char *feature = m.getConfigEntry("Feature");
    const bool dictionary = std::string(m.getType()) == "Lexicons / Dictionaries" && dynamic_cast<sword::SWLD *>(&m) &&
      (!feature || std::string(feature) != "DailyDevotion");
    impl_->modules.push_back({m.getName(), m.getDescription(), m.getType(),
                             language ? language : "", module_options(m), transliteration_available, dictionary,
                             dictionary ? xiphos_core_lexicon_role(feature) : 0});
  }
}
Reader::~Reader() = default;
const std::vector<Module> &Reader::modules() const { return impl_->modules; }
ReferenceChoices Reader::reference_choices(const std::string &id, const std::string &reference) {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  return xiphos::core::reference_choices(impl_->verse_module(id, reference), reference);
}
PassageLink Reader::resolve_link(const std::string &current_module, const std::string &url) {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  auto target = passage_link(url, current_module);
  impl_->verse_module(target.module, target.reference);
  return target;
}
ReferenceList Reader::read_reference_list(const std::string &id, const std::string &expression,
                                         const std::string &context) {
  return reference_list_impl(id, expression, context, nullptr);
}
ReferenceList Reader::read_reference_list(const std::string &id, const std::string &expression,
                                         const std::string &context, const EngineOptions &options) {
  return reference_list_impl(id, expression, context, &options);
}
ReferenceList Reader::reference_list_impl(const std::string &id, const std::string &expression,
                                         const std::string &context, const EngineOptions *options) {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  auto &module = impl_->verse_module(id, context);
  const auto parsed = parse_reference_list(module, expression, context);
  struct RestoreKey {
    sword::SWModule &module;
    std::string reference;
    ~RestoreKey() { module.getKey()->setText(reference.c_str()); }
  } restore{module, module.getKeyText()};
  std::unique_ptr<ScopedEngineOptions> scope;
  if (options) scope = std::make_unique<ScopedEngineOptions>(*impl_->manager, *options);
  ReferenceList result;
  result.module = id; result.context = parsed.context; result.expression = expression;
  result.limited = parsed.limited; result.skipped = parsed.skipped;
  result.entries.reserve(parsed.references.size());
  for (const auto &reference : parsed.references) {
    auto html = render_entry(module, reference.c_str());
    if (module.getKey()->getError()) throw std::invalid_argument("SWORD could not render a reference-list entry");
    result.entries.push_back({module.getKeyText(), std::move(html)});
  }
  result.html = display_reference_list(result);
  return result;
}
StudyNote Reader::study_note(const std::string &id, const std::string &reference,
                             const std::string &identifier) {
  return note_impl(id, reference, identifier, nullptr);
}
StudyNote Reader::study_note(const std::string &id, const std::string &reference,
                             const std::string &identifier, const EngineOptions &options) {
  return note_impl(id, reference, identifier, &options);
}
StudyNote Reader::note_impl(const std::string &id, const std::string &reference,
                            const std::string &identifier, const EngineOptions *options) {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  auto &module = impl_->verse_module(id, reference);
  std::unique_ptr<ScopedEngineOptions> scope;
  if (options) scope = std::make_unique<ScopedEngineOptions>(*impl_->manager, *options);
  return render_study_note(module, reference, identifier);
}
std::string Reader::select_lexicon(const LexicalLink &command, const std::string &source_reference,
                                  int origin, const LexiconPreferences &preferences) {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  if (origin != XIPHOS_LEXICON_MAIN && origin != XIPHOS_LEXICON_DIALOG)
    throw std::invalid_argument("Invalid lexicon origin");
  if (command.kind != LexicalLinkKind::strongs && command.kind != LexicalLinkKind::morphology)
    throw std::invalid_argument("Invalid lexical kind");
  if (command.type.find('\0') != std::string::npos || command.module.find('\0') != std::string::npos ||
      source_reference.find('\0') != std::string::npos)
    throw std::invalid_argument("Lexical source context contains NUL");
  const auto borrowed = borrow_lexicon_preferences(preferences);
  struct Context { Impl *impl; const LexicalLink *command; const std::string *reference; };
  Context source{impl_.get(), &command, &source_reference};
  const XiphosLexiconContext context{&source,
    [](void *user, const char *id) -> int {
      return static_cast<Context *>(user)->impl->manager->getModule(id) != nullptr;
    },
    [](void *user) -> int {
      auto &source = *static_cast<Context *>(user);
      auto &module = source.impl->verse_module(source.command->module, *source.reference);
      sword::VerseKey key(*static_cast<sword::VerseKey *>(module.getKey()));
      key.setAutoNormalize(true);
      key.setText(source.reference->c_str());
      if (key.getError() || key.getChapter() < 1 || key.getVerse() < 1)
        throw std::invalid_argument("SWORD could not resolve lexical source context");
      return key.getTestament();
    }};
  const auto *selected = xiphos_core_select_lexicon(origin,
    command.kind == LexicalLinkKind::strongs ? XIPHOS_LEXICON_STRONGS : XIPHOS_LEXICON_MORPHOLOGY,
    command.type.c_str(), &borrowed, &context);
  if (!selected || !*selected) throw std::invalid_argument("No lexicon selected for this command");
  return selected;
}
DictionaryEntry Reader::read_dictionary(const std::string &module, const std::string &key) {
  return dictionary_impl(module, key, nullptr);
}
DictionaryEntry Reader::read_dictionary(const std::string &module, const std::string &key, const EngineOptions &options) {
  return dictionary_impl(module, key, &options);
}
DictionaryChoices Reader::dictionary_choices(const std::string &id, const std::string &key) {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  if (id.empty() || key.empty() || id.find('\0') != std::string::npos || key.find('\0') != std::string::npos)
    throw std::invalid_argument("Dictionary module/key is empty or contains NUL");
  auto *module = impl_->manager->getModule(id.c_str());
  if (!module) throw std::out_of_range("Module is not installed");
  if (!dynamic_cast<sword::SWLD *>(module)) throw std::invalid_argument("Module is not a dictionary");
  const std::string saved = module->getKeyText();
  struct Restore { sword::SWModule &module; const std::string &key; ~Restore() { module.setKey(key.c_str()); } } restore{*module, saved};
  module->setKey(key.c_str());
  const std::string resolved = module->getKeyText();
  return {id, resolved, dictionary_nearby_keys(*module)};
}
DictionaryEntry Reader::navigate_dictionary(const std::string &module, const std::string &key, int direction) {
  if (direction != -1 && direction != 1) throw std::invalid_argument("Invalid dictionary direction");
  return dictionary_impl(module, key, nullptr, direction);
}
DictionaryEntry Reader::navigate_dictionary(const std::string &module, const std::string &key, int direction, const EngineOptions &options) {
  if (direction != -1 && direction != 1) throw std::invalid_argument("Invalid dictionary direction");
  return dictionary_impl(module, key, &options, direction);
}
DictionaryEntry Reader::dictionary_impl(const std::string &id, const std::string &key, const EngineOptions *options, int direction) {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  if (key.empty() || id.empty() || key.find('\0') != std::string::npos || id.find('\0') != std::string::npos)
    throw std::invalid_argument("Dictionary module/key is empty or contains NUL");
  auto *module = impl_->manager->getModule(id.c_str());
  if (!module) throw std::out_of_range("Module is not installed");
  if (!dynamic_cast<sword::SWLD *>(module)) throw std::invalid_argument("Module is not a dictionary");
  std::unique_ptr<ScopedEngineOptions> scope;
  if (options) scope = std::make_unique<ScopedEngineOptions>(*impl_->manager, *options);
  std::string target = key;
  if (direction) {
    // Legacy main_dictionary_button_clicked: set current key, step, copy key,
    // then activate the entry. Preserve SDK boundary/matching behavior.
    module->setKey(key.c_str());
    target = dictionary_step_key(*module, direction);
  }
  DictionaryEntry result{id, "", render_entry(*module, target.c_str())};
  result.key = module->getKeyText();
  return result;
}
Entry Reader::read(const std::string &id, const std::string &reference) {
  return read_impl(id, reference, nullptr);
}
Entry Reader::read(const std::string &id, const std::string &reference, const EngineOptions &options) {
  return read_impl(id, reference, &options);
}
Entry Reader::read_impl(const std::string &id, const std::string &reference, const EngineOptions *options) {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  auto *module = &impl_->verse_module(id, reference);
  std::unique_ptr<ScopedEngineOptions> scope;
  if (options) scope = std::make_unique<ScopedEngineOptions>(*impl_->manager, *options);
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
  return chapter_impl(id, reference, options, nullptr);
}
Chapter Reader::read_chapter(const std::string &id, const std::string &reference,
                             const DisplayOptions &display, const EngineOptions &engine) {
  return chapter_impl(id, reference, display, &engine);
}
Chapter Reader::chapter_impl(const std::string &id, const std::string &reference,
                             const DisplayOptions &options, const EngineOptions *engine) {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  auto &module = impl_->verse_module(id, reference);
  std::unique_ptr<ScopedEngineOptions> scope;
  if (engine) scope = std::make_unique<ScopedEngineOptions>(*impl_->manager, *engine);
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
