// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/c_api.h"
#include "xiphos/core/reader.hpp"
#include <stdexcept>
#include <chrono>
#include <type_traits>
#include "xiphos/core/worker.hpp"
#include "xiphos/core/session.hpp"
struct xc_reader { std::shared_ptr<xiphos::core::Reader> value; explicit xc_reader(const char *p) : value(std::make_shared<xiphos::core::Reader>(p ? p : "")) {} };
struct xc_session {
  std::shared_ptr<xiphos::core::Reader> owner;
  xiphos::core::StudySession value;
  explicit xc_session(xc_reader &r) : owner(r.value), value(*owner) {}
};
struct xc_chapter { xiphos::core::Chapter value; };
struct xc_choices { xiphos::core::ReferenceChoices value; };
struct xc_link { xiphos::core::PassageLink value; };
struct xc_note { xiphos::core::StudyNote value; };
struct xc_entry { xiphos::core::Entry value; };
struct xc_worker { xiphos::core::StudyWorker value; explicit xc_worker(const char *root) : value(root ? root : "") {} };
struct xc_catalog { std::vector<xiphos::core::Module> value; };
struct xc_history { xiphos::core::HistorySnapshot value; };
struct xc_request {
  xc_status status = XC_PENDING;
  uint64_t tab = 0;
  bool options_result = false;
  std::optional<xiphos::core::EngineOptions> options;
  std::unique_ptr<xc_note> note;
  std::unique_ptr<xc_entry> entry;
  std::unique_ptr<xc_chapter> chapter;
  std::unique_ptr<xc_choices> choices;
  std::unique_ptr<xc_link> link;
  std::unique_ptr<xc_catalog> catalog;
  std::unique_ptr<xc_history> history;
  virtual ~xc_request() = default;
  virtual void update() = 0;
  void store(uint64_t v) { tab = v; }
  void store(std::optional<xiphos::core::EngineOptions> v) { options = v; options_result = true; }
  void store(xiphos::core::StudyNote v) { note = std::make_unique<xc_note>(xc_note{std::move(v)}); }
  void store(xiphos::core::Entry v) { entry = std::make_unique<xc_entry>(xc_entry{std::move(v)}); }
  void store(xiphos::core::Chapter v) { chapter = std::make_unique<xc_chapter>(xc_chapter{std::move(v)}); }
  void store(xiphos::core::ReferenceChoices v) { choices = std::make_unique<xc_choices>(xc_choices{std::move(v)}); }
  void store(xiphos::core::PassageLink v) { link = std::make_unique<xc_link>(xc_link{std::move(v)}); }
  void store(std::vector<xiphos::core::Module> v) { catalog = std::make_unique<xc_catalog>(xc_catalog{std::move(v)}); }
  void store(xiphos::core::HistorySnapshot v) { history = std::make_unique<xc_history>(xc_history{std::move(v)}); }
};
namespace {
xiphos::core::EngineOptions options_from_flags(unsigned flags) {
  xiphos::core::EngineOptions options;
  options.strongs = (flags & XC_ENGINE_STRONGS) != 0;
  options.morphs = (flags & XC_ENGINE_MORPHOLOGY) != 0;
  options.footnotes = (flags & XC_ENGINE_FOOTNOTES) != 0;
  options.greekaccents = (flags & XC_ENGINE_GREEK_ACCENTS) != 0;
  options.lemmas = (flags & XC_ENGINE_LEMMAS) != 0;
  options.scripturerefs = (flags & XC_ENGINE_CROSS_REFERENCES) != 0;
  options.hebrewpoints = (flags & XC_ENGINE_HEBREW_VOWELS) != 0;
  options.hebrewcant = (flags & XC_ENGINE_HEBREW_CANTILLATION) != 0;
  options.italic_headings = (flags & XC_ENGINE_ITALIC_HEADINGS) != 0;
  options.words_in_red = (flags & XC_ENGINE_WORDS_IN_RED) != 0;
  options.xlit = (flags & XC_ENGINE_TRANSLITERATED_FORMS) != 0;
  options.enumerated = (flags & XC_ENGINE_ENUMERATIONS) != 0;
  options.glosses = (flags & XC_ENGINE_GLOSSES) != 0;
  options.morphseg = (flags & XC_ENGINE_MORPHEME_SEGMENTATION) != 0;
  options.transliteration = (flags & XC_ENGINE_TRANSLITERATION) != 0;
  options.variants_primary = (flags & XC_ENGINE_VARIANTS_PRIMARY) != 0;
  options.variants_secondary = (flags & XC_ENGINE_VARIANTS_SECONDARY) != 0;
  options.variants_all = (flags & XC_ENGINE_VARIANTS_ALL) != 0;
  return options;
}
unsigned options_to_flags(const xiphos::core::EngineOptions &o) {
  unsigned flags = 0;
  if (o.strongs) flags |= XC_ENGINE_STRONGS;
  if (o.morphs) flags |= XC_ENGINE_MORPHOLOGY;
  if (o.footnotes) flags |= XC_ENGINE_FOOTNOTES;
  if (o.greekaccents) flags |= XC_ENGINE_GREEK_ACCENTS;
  if (o.lemmas) flags |= XC_ENGINE_LEMMAS;
  if (o.scripturerefs) flags |= XC_ENGINE_CROSS_REFERENCES;
  if (o.hebrewpoints) flags |= XC_ENGINE_HEBREW_VOWELS;
  if (o.hebrewcant) flags |= XC_ENGINE_HEBREW_CANTILLATION;
  if (o.italic_headings) flags |= XC_ENGINE_ITALIC_HEADINGS;
  if (o.words_in_red) flags |= XC_ENGINE_WORDS_IN_RED;
  if (o.xlit) flags |= XC_ENGINE_TRANSLITERATED_FORMS;
  if (o.enumerated) flags |= XC_ENGINE_ENUMERATIONS;
  if (o.glosses) flags |= XC_ENGINE_GLOSSES;
  if (o.morphseg) flags |= XC_ENGINE_MORPHEME_SEGMENTATION;
  if (o.transliteration) flags |= XC_ENGINE_TRANSLITERATION;
  if (o.variants_primary) flags |= XC_ENGINE_VARIANTS_PRIMARY;
  if (o.variants_secondary) flags |= XC_ENGINE_VARIANTS_SECONDARY;
  if (o.variants_all) flags |= XC_ENGINE_VARIANTS_ALL;
  return flags;
}
xc_status failure() noexcept {
  try { throw; }
  catch (const std::out_of_range &) { return XC_MODULE_NOT_FOUND; }
  catch (const std::invalid_argument &) { return XC_INVALID_ARGUMENT; }
  catch (...) { return XC_ENGINE_ERROR; }
}
template<class T> struct Request final : xc_request {
  std::future<T> future;
  void update() override {
    if (status != XC_PENDING || future.wait_for(std::chrono::seconds(0)) != std::future_status::ready) return;
    try {
      if constexpr (std::is_void_v<T>) future.get();
      else store(future.get());
      status = XC_OK;
    } catch (...) { status = failure(); }
  }
};
template<class F> struct FutureValue;
template<class T> struct FutureValue<std::future<T>> { using Type = T; };
template<class F> xc_status queue_request(xc_worker *w, xc_request **out, F operation) {
  if (!out) return XC_INVALID_ARGUMENT;
  *out = nullptr;
  if (!w) return XC_INVALID_ARGUMENT;
  try {
    using T = typename FutureValue<decltype(operation(w->value))>::Type;
    // Allocate the observer BEFORE accepting an operation that may mutate state.
    auto request = std::make_unique<Request<T>>();
    request->future = operation(w->value);
    *out = request.release(); return XC_OK;
  } catch (...) { return failure(); }
}
const xiphos::core::Module *catalog_at(const xc_catalog *c, size_t i) {
  return c && i < c->value.size() ? &c->value[i] : nullptr;
}
const std::vector<xiphos::core::ReferenceChoice> *choices_at(const xc_choices *c, xc_choice_kind kind) {
  if (!c) return nullptr;
  switch (kind) {
  case XC_BOOK: return &c->value.books;
  case XC_CHAPTER: return &c->value.chapters;
  case XC_VERSE: return &c->value.verses;
  default: return nullptr;
  }
}
const xiphos::core::Module *module_at(const xc_reader *r, size_t i) {
  return r && i < r->value->modules().size() ? &r->value->modules()[i] : nullptr;
}
}
extern "C" {
xc_status xc_worker_open(const char *root, xc_worker **out) {
  if (!out) return XC_INVALID_ARGUMENT;
  *out = nullptr;
  try { *out = new xc_worker(root); return XC_OK; } catch (...) { return failure(); }
}
void xc_worker_close(xc_worker *w) { delete w; }
xc_status xc_worker_modules(xc_worker *w, xc_request **out) {
  return queue_request(w, out, [](auto &worker) { return worker.modules(); });
}
xc_status xc_worker_create_tab(xc_worker *w, xc_request **out) {
  return queue_request(w, out, [](auto &worker) { return worker.create_tab(); });
}
xc_status xc_worker_close_tab(xc_worker *w, uint64_t tab, xc_request **out) {
  return queue_request(w, out, [tab](auto &worker) { return worker.close_tab(tab); });
}
xc_status xc_worker_visit(xc_worker *w, uint64_t tab, const char *id, const char *ref, xc_request **out) {
  if (!id || !*id || !ref || !*ref) { if (out) *out = nullptr; return XC_INVALID_ARGUMENT; }
  return queue_request(w, out, [=](auto &worker) { return worker.visit(tab, id, ref); });
}
xc_status xc_worker_navigate(xc_worker *w, uint64_t tab, int direction, xc_request **out) {
  return queue_request(w, out, [=](auto &worker) { return worker.navigate(tab, direction); });
}
xc_status xc_worker_navigate_chapter(xc_worker *w, uint64_t tab, const char *id, const char *ref, int direction, xc_request **out) {
  if (!id || !*id || !ref || !*ref) { if (out) *out = nullptr; return XC_INVALID_ARGUMENT; }
  return queue_request(w, out, [=](auto &worker) { return worker.navigate_chapter(tab, id, ref, direction); });
}
xc_status xc_worker_history(xc_worker *w, uint64_t tab, xc_request **out) {
  return queue_request(w, out, [=](auto &worker) { return worker.history(tab); });
}
xc_status xc_worker_clear_history(xc_worker *w, uint64_t tab, xc_request **out) {
  return queue_request(w, out, [=](auto &worker) { return worker.clear_history(tab); });
}
xc_status xc_worker_set_engine_options(xc_worker *w, uint64_t tab, unsigned flags, xc_request **out) {
  constexpr unsigned known = (1u << 18) - 1;
  if (flags & ~known) { if (out) *out = nullptr; return XC_INVALID_ARGUMENT; }
  const auto options = options_from_flags(flags);
  return queue_request(w, out, [=](auto &worker) { return worker.set_engine_options(tab, options); });
}
xc_status xc_worker_save_module_preferences(xc_worker *w, uint64_t tab, const char *path, xc_request **out) {
  if (!path || !*path) { if (out) *out = nullptr; return XC_INVALID_ARGUMENT; }
  return queue_request(w, out, [=](auto &worker) { return worker.save_module_preferences(tab, path); });
}
xc_status xc_worker_import_module_preferences(xc_worker *w, uint64_t tab, const char *path, xc_request **out) {
  if (!path || !*path) { if (out) *out = nullptr; return XC_INVALID_ARGUMENT; }
  return queue_request(w, out, [=](auto &worker) { return worker.import_module_preferences(tab, path); });
}
xc_status xc_worker_enable_module_preferences(xc_worker *w, uint64_t tab, xc_request **out) {
  return queue_request(w, out, [=](auto &worker) { return worker.enable_module_preferences(tab); });
}
xc_status xc_worker_set_module_engine_options(xc_worker *w, uint64_t tab, const char *id, unsigned flags, xc_request **out) {
  if (!id || !*id || (flags & ~((1u << 18) - 1))) { if (out) *out = nullptr; return XC_INVALID_ARGUMENT; }
  const auto options = options_from_flags(flags);
  return queue_request(w, out, [=](auto &worker) { return worker.set_module_engine_options(tab, id, options); });
}
xc_status xc_worker_clear_module_engine_options(xc_worker *w, uint64_t tab, const char *id, xc_request **out) {
  if (!id || !*id) { if (out) *out = nullptr; return XC_INVALID_ARGUMENT; }
  return queue_request(w, out, [=](auto &worker) { return worker.clear_module_engine_options(tab, id); });
}
xc_status xc_worker_module_engine_options(xc_worker *w, uint64_t tab, const char *id, xc_request **out) {
  if (!id || !*id) { if (out) *out = nullptr; return XC_INVALID_ARGUMENT; }
  return queue_request(w, out, [=](auto &worker) { return worker.module_engine_options(tab, id); });
}
xc_status xc_request_engine_options(const xc_request *r, unsigned *flags, int *configured) {
  if (flags) *flags = 0;
  if (configured) *configured = 0;
  if (!r || !flags || !configured) return XC_INVALID_ARGUMENT;
  if (r->status != XC_OK) return r->status;
  if (!r->options_result) return XC_INVALID_ARGUMENT;
  if (r->options) { *flags = options_to_flags(*r->options); *configured = 1; }
  return XC_OK;
}
xc_status xc_worker_clear_engine_options(xc_worker *w, uint64_t tab, xc_request **out) {
  return queue_request(w, out, [=](auto &worker) { return worker.clear_engine_options(tab); });
}
xc_status xc_worker_study_note_link(xc_worker *w, uint64_t tab, const char *id, const char *uri, xc_request **out) {
  if (!uri || !*uri) { if (out) *out = nullptr; return XC_INVALID_ARGUMENT; }
  return queue_request(w, out, [=](auto &worker) { return worker.study_note_link(tab, id ? id : "", uri); });
}
xc_status xc_worker_study_note(xc_worker *w, uint64_t tab, const char *id, const char *ref, const char *identifier, xc_request **out) {
  if (!id || !*id || !ref || !*ref || !identifier || !*identifier) {
    if (out) *out = nullptr; return XC_INVALID_ARGUMENT;
  }
  return queue_request(w, out, [=](auto &worker) { return worker.study_note(tab, id, ref, identifier); });
}
xc_status xc_worker_read_tab_entry(xc_worker *w, uint64_t tab, const char *id, const char *ref, xc_request **out) {
  if (!id || !*id || !ref || !*ref) { if (out) *out = nullptr; return XC_INVALID_ARGUMENT; }
  return queue_request(w, out, [=](auto &worker) { return worker.read_entry(tab, id, ref); });
}
xc_status xc_worker_read_tab_chapter(xc_worker *w, uint64_t tab, const char *id, const char *ref, unsigned flags, xc_request **out) {
  if (!id || !*id || !ref || !*ref || (flags & ~(XC_CHAPTER_VERSE_NUMBERS | XC_CHAPTER_HIGHLIGHT_SELECTED))) {
    if (out) *out = nullptr; return XC_INVALID_ARGUMENT;
  }
  const xiphos::core::DisplayOptions options{(flags & XC_CHAPTER_VERSE_NUMBERS) != 0, (flags & XC_CHAPTER_HIGHLIGHT_SELECTED) != 0};
  return queue_request(w, out, [=](auto &worker) { return worker.read_chapter(tab, id, ref, options); });
}
xc_status xc_worker_read_chapter(xc_worker *w, const char *id, const char *ref, unsigned flags, xc_request **out) {
  if (!id || !*id || !ref || !*ref || (flags & ~(XC_CHAPTER_VERSE_NUMBERS | XC_CHAPTER_HIGHLIGHT_SELECTED))) {
    if (out) *out = nullptr; return XC_INVALID_ARGUMENT;
  }
  const xiphos::core::DisplayOptions options{(flags & XC_CHAPTER_VERSE_NUMBERS) != 0, (flags & XC_CHAPTER_HIGHLIGHT_SELECTED) != 0};
  return queue_request(w, out, [=](auto &worker) { return worker.read_chapter(id, ref, options); });
}
xc_status xc_worker_reference_choices(xc_worker *w, const char *id, const char *ref, xc_request **out) {
  if (!id || !*id || !ref || !*ref) { if (out) *out = nullptr; return XC_INVALID_ARGUMENT; }
  return queue_request(w, out, [=](auto &worker) { return worker.reference_choices(id, ref); });
}
xc_status xc_worker_resolve_link(xc_worker *w, const char *id, const char *uri, xc_request **out) {
  if (!uri || !*uri) { if (out) *out = nullptr; return XC_INVALID_ARGUMENT; }
  return queue_request(w, out, [=](auto &worker) { return worker.resolve_link(id ? id : "", uri); });
}
xc_status xc_request_poll(xc_request *r) {
  if (!r) return XC_INVALID_ARGUMENT;
  try { r->update(); } catch (...) { r->status = failure(); }
  return r->status;
}
void xc_request_release(xc_request *r) { delete r; }
uint64_t xc_request_tab(const xc_request *r) { return r && r->status == XC_OK ? r->tab : 0; }
xc_note *xc_request_take_note(xc_request *r) { return r && r->status == XC_OK ? r->note.release() : nullptr; }
xc_entry *xc_request_take_entry(xc_request *r) { return r && r->status == XC_OK ? r->entry.release() : nullptr; }
xc_chapter *xc_request_take_chapter(xc_request *r) { return r && r->status == XC_OK ? r->chapter.release() : nullptr; }
xc_choices *xc_request_take_choices(xc_request *r) { return r && r->status == XC_OK ? r->choices.release() : nullptr; }
xc_link *xc_request_take_link(xc_request *r) { return r && r->status == XC_OK ? r->link.release() : nullptr; }
xc_catalog *xc_request_take_catalog(xc_request *r) { return r && r->status == XC_OK ? r->catalog.release() : nullptr; }
xc_history *xc_request_take_history(xc_request *r) { return r && r->status == XC_OK ? r->history.release() : nullptr; }
size_t xc_catalog_count(const xc_catalog *c) { return c ? c->value.size() : 0; }
const char *xc_catalog_id(const xc_catalog *c, size_t i) { auto m = catalog_at(c, i); return m ? m->id.c_str() : nullptr; }
const char *xc_catalog_description(const xc_catalog *c, size_t i) { auto m = catalog_at(c, i); return m ? m->description.c_str() : nullptr; }
const char *xc_catalog_type(const xc_catalog *c, size_t i) { auto m = catalog_at(c, i); return m ? m->type.c_str() : nullptr; }
const char *xc_catalog_language(const xc_catalog *c, size_t i) { auto m = catalog_at(c, i); return m ? m->language.c_str() : nullptr; }
unsigned xc_catalog_declared_engine_flags(const xc_catalog *c, size_t i) {
  const auto *module = catalog_at(c, i);
  if (!module) return 0;
  const auto &o = module->options.declared;
  unsigned flags = 0;
  if (o.strongs) flags |= XC_ENGINE_STRONGS;
  if (o.morphs) flags |= XC_ENGINE_MORPHOLOGY;
  if (o.footnotes) flags |= XC_ENGINE_FOOTNOTES;
  if (o.greekaccents) flags |= XC_ENGINE_GREEK_ACCENTS;
  if (o.lemmas) flags |= XC_ENGINE_LEMMAS;
  if (o.scripturerefs) flags |= XC_ENGINE_CROSS_REFERENCES;
  if (o.hebrewpoints) flags |= XC_ENGINE_HEBREW_VOWELS;
  if (o.hebrewcant) flags |= XC_ENGINE_HEBREW_CANTILLATION;
  if (o.italic_headings) flags |= XC_ENGINE_ITALIC_HEADINGS;
  if (o.words_in_red) flags |= XC_ENGINE_WORDS_IN_RED;
  if (o.xlit) flags |= XC_ENGINE_TRANSLITERATED_FORMS;
  if (o.enumerated) flags |= XC_ENGINE_ENUMERATIONS;
  if (o.glosses) flags |= XC_ENGINE_GLOSSES;
  if (o.morphseg) flags |= XC_ENGINE_MORPHEME_SEGMENTATION;
  if (o.transliteration) flags |= XC_ENGINE_TRANSLITERATION;
  if (o.variants_primary) flags |= XC_ENGINE_VARIANTS_PRIMARY;
  if (o.variants_secondary) flags |= XC_ENGINE_VARIANTS_SECONDARY;
  if (o.variants_all) flags |= XC_ENGINE_VARIANTS_ALL;
  return flags;
}
int xc_catalog_has_headings(const xc_catalog *c, size_t i) {
  const auto *module = catalog_at(c, i); return module && module->options.headings;
}
int xc_catalog_transliteration_available(const xc_catalog *c, size_t i) {
  const auto *module = catalog_at(c, i); return module && module->transliteration_available;
}
void xc_catalog_release(xc_catalog *c) { delete c; }
int xc_history_can_back(const xc_history *h) { return h && h->value.can_back; }
int xc_history_can_forward(const xc_history *h) { return h && h->value.can_forward; }
size_t xc_history_count(const xc_history *h) { return h ? h->value.entries.size() : 0; }
const char *xc_history_module(const xc_history *h) {
  return h && h->value.cursor < h->value.entries.size() ? h->value.entries[h->value.cursor].module.c_str() : nullptr;
}
void xc_history_release(xc_history *h) { delete h; }
unsigned xc_api_version(void) { return 1; }
xc_status xc_open(const char *root, xc_reader **out) {
  if (!out) return XC_INVALID_ARGUMENT;
  *out = nullptr;
  try { *out = new xc_reader(root); return XC_OK; } catch (...) { return failure(); }
}
void xc_close(xc_reader *r) { delete r; }
size_t xc_module_count(const xc_reader *r) { return r ? r->value->modules().size() : 0; }
const char *xc_module_id(const xc_reader *r, size_t i) { auto m = module_at(r, i); return m ? m->id.c_str() : nullptr; }
const char *xc_module_description(const xc_reader *r, size_t i) { auto m = module_at(r, i); return m ? m->description.c_str() : nullptr; }
const char *xc_module_type(const xc_reader *r, size_t i) { auto m = module_at(r, i); return m ? m->type.c_str() : nullptr; }
const char *xc_module_language(const xc_reader *r, size_t i) { auto m = module_at(r, i); return m ? m->language.c_str() : nullptr; }
xc_status xc_read(xc_reader *r, const char *id, const char *ref, xc_entry **out) {
  if (!out) return XC_INVALID_ARGUMENT;
  *out = nullptr;
  if (!r || !id || !*id || !ref || !*ref) return XC_INVALID_ARGUMENT;
  try { *out = new xc_entry{r->value->read(id, ref)}; return XC_OK; }
  catch (...) { return failure(); }
}
xc_status xc_session_open(xc_reader *r, xc_session **out) {
  if (!out) return XC_INVALID_ARGUMENT;
  *out = nullptr;
  if (!r) return XC_INVALID_ARGUMENT;
  try { *out = new xc_session(*r); return XC_OK; } catch (...) { return failure(); }
}
void xc_session_close(xc_session *s) { delete s; }
xc_status xc_session_visit(xc_session *s, const char *id, const char *ref, xc_entry **out) {
  if (!out) return XC_INVALID_ARGUMENT;
  *out = nullptr;
  if (!s || !id || !*id || !ref || !*ref) return XC_INVALID_ARGUMENT;
  try {
    auto result = std::make_unique<xc_entry>();
    result->value = s->value.visit(id, ref);
    *out = result.release(); return XC_OK;
  } catch (...) { return failure(); }
}
xc_status xc_session_navigate(xc_session *s, int direction, xc_entry **out) {
  if (!out) return XC_INVALID_ARGUMENT;
  *out = nullptr;
  if (!s) return XC_INVALID_ARGUMENT;
  try {
    auto result = std::make_unique<xc_entry>();
    result->value = s->value.navigate(direction);
    *out = result.release(); return XC_OK;
  } catch (...) { return failure(); }
}
int xc_session_can_back(const xc_session *s) { return s && s->value.history().can_back(); }
int xc_session_can_forward(const xc_session *s) { return s && s->value.history().can_forward(); }
size_t xc_session_history_count(const xc_session *s) { return s ? s->value.history().entries().size() : 0; }
const char *xc_session_module(const xc_session *s) {
  if (!s || s->value.history().entries().empty()) return nullptr;
  const auto &h = s->value.history(); return h.entries()[h.cursor()].module.c_str();
}
void xc_session_clear_history(xc_session *s) { if (s) s->value.clear_history(); }
xc_status xc_reference_choices(xc_reader *r, const char *id, const char *ref, xc_choices **out) {
  if (!out) return XC_INVALID_ARGUMENT;
  *out = nullptr;
  if (!r || !id || !*id || !ref || !*ref) return XC_INVALID_ARGUMENT;
  try { *out = new xc_choices{r->value->reference_choices(id, ref)}; return XC_OK; }
  catch (...) { return failure(); }
}
size_t xc_choices_count(const xc_choices *c, xc_choice_kind kind) { auto v = choices_at(c, kind); return v ? v->size() : 0; }
const char *xc_choices_label(const xc_choices *c, xc_choice_kind kind, size_t i) {
  auto v = choices_at(c, kind); return v && i < v->size() ? (*v)[i].label.c_str() : nullptr;
}
const char *xc_choices_reference(const xc_choices *c, xc_choice_kind kind, size_t i) {
  auto v = choices_at(c, kind); return v && i < v->size() ? (*v)[i].reference.c_str() : nullptr;
}
const char *xc_choices_current_label(const xc_choices *c, xc_choice_kind kind) {
  if (!c) return nullptr;
  switch (kind) {
  case XC_BOOK: return c->value.book_label.c_str();
  case XC_CHAPTER: return c->value.chapter_label.c_str();
  case XC_VERSE: return c->value.verse_label.c_str();
  default: return nullptr;
  }
}
void xc_choices_release(xc_choices *c) { delete c; }
xc_status xc_resolve_link(xc_reader *r, const char *current, const char *uri, xc_link **out) {
  if (!out) return XC_INVALID_ARGUMENT;
  *out = nullptr;
  if (!r || !uri || !*uri) return XC_INVALID_ARGUMENT;
  try { *out = new xc_link{r->value->resolve_link(current ? current : "", uri)}; return XC_OK; }
  catch (...) { return failure(); }
}
const char *xc_link_module(const xc_link *link) { return link ? link->value.module.c_str() : nullptr; }
const char *xc_link_reference(const xc_link *link) { return link ? link->value.reference.c_str() : nullptr; }
void xc_link_release(xc_link *link) { delete link; }
xc_status xc_read_chapter(xc_reader *r, const char *id, const char *ref, xc_chapter **out) {
  return xc_read_chapter_with_options(r, id, ref,
          XC_CHAPTER_VERSE_NUMBERS | XC_CHAPTER_HIGHLIGHT_SELECTED, out);
}
xc_status xc_read_chapter_with_options(xc_reader *r, const char *id, const char *ref,
                                      unsigned flags, xc_chapter **out) {
  if (!out) return XC_INVALID_ARGUMENT;
  *out = nullptr;
  if (!r || !id || !*id || !ref || !*ref) return XC_INVALID_ARGUMENT;
  if (flags & ~(XC_CHAPTER_VERSE_NUMBERS | XC_CHAPTER_HIGHLIGHT_SELECTED)) return XC_INVALID_ARGUMENT;
  const xiphos::core::DisplayOptions options{
    (flags & XC_CHAPTER_VERSE_NUMBERS) != 0,
    (flags & XC_CHAPTER_HIGHLIGHT_SELECTED) != 0};
  try { *out = new xc_chapter{r->value->read_chapter(id, ref, options)}; return XC_OK; }
  catch (...) { return failure(); }
}
size_t xc_chapter_selected_verse(const xc_chapter *c) { return c ? c->value.selected_verse : 0; }
const char *xc_chapter_selected_anchor(const xc_chapter *c) { return c ? c->value.selected_anchor.c_str() : nullptr; }
const char *xc_chapter_html(const xc_chapter *c) { return c ? c->value.html.c_str() : nullptr; }
const char *xc_chapter_reference(const xc_chapter *c) { return c ? c->value.reference.c_str() : nullptr; }
const char *xc_chapter_title(const xc_chapter *c) { return c ? c->value.title.c_str() : nullptr; }
size_t xc_chapter_count(const xc_chapter *c) { return c ? c->value.verses.size() : 0; }
const char *xc_chapter_verse_reference(const xc_chapter *c, size_t i) {
  return c && i < c->value.verses.size() ? c->value.verses[i].reference.c_str() : nullptr;
}
const char *xc_chapter_verse_html(const xc_chapter *c, size_t i) {
  return c && i < c->value.verses.size() ? c->value.verses[i].html.c_str() : nullptr;
}
void xc_chapter_release(xc_chapter *c) { delete c; }
xc_status xc_session_navigate_chapter(xc_session *s, const char *id, const char *ref,
                                     int direction, xc_entry **out) {
  if (!out) return XC_INVALID_ARGUMENT;
  *out = nullptr;
  if (!s || !id || !*id || !ref || !*ref) return XC_INVALID_ARGUMENT;
  try {
    auto result = std::make_unique<xc_entry>();
    result->value = s->value.navigate_chapter(id, ref, direction);
    *out = result.release(); return XC_OK;
  } catch (...) { return failure(); }
}
const char *xc_note_reference(const xc_note *n) { return n ? n->value.reference.c_str() : nullptr; }
const char *xc_note_identifier(const xc_note *n) { return n ? n->value.identifier.c_str() : nullptr; }
const char *xc_note_type(const xc_note *n) { return n ? n->value.type.c_str() : nullptr; }
const char *xc_note_label(const xc_note *n) { return n ? n->value.label.c_str() : nullptr; }
const char *xc_note_html(const xc_note *n) { return n ? n->value.html.c_str() : nullptr; }
const char *xc_note_reference_list(const xc_note *n) { return n ? n->value.reference_list.c_str() : nullptr; }
void xc_note_release(xc_note *n) { delete n; }
const char *xc_entry_reference(const xc_entry *e) { return e ? e->value.reference.c_str() : nullptr; }
const char *xc_entry_html(const xc_entry *e) { return e ? e->value.html.c_str() : nullptr; }
void xc_entry_release(xc_entry *e) { delete e; }
}
