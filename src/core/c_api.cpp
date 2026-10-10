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
struct xc_entry { xiphos::core::Entry value; };
struct xc_worker { xiphos::core::StudyWorker value; explicit xc_worker(const char *root) : value(root ? root : "") {} };
struct xc_catalog { std::vector<xiphos::core::Module> value; };
struct xc_history { xiphos::core::HistorySnapshot value; };
struct xc_request {
  xc_status status = XC_PENDING;
  uint64_t tab = 0;
  std::unique_ptr<xc_entry> entry;
  std::unique_ptr<xc_chapter> chapter;
  std::unique_ptr<xc_choices> choices;
  std::unique_ptr<xc_link> link;
  std::unique_ptr<xc_catalog> catalog;
  std::unique_ptr<xc_history> history;
  virtual ~xc_request() = default;
  virtual void update() = 0;
  void store(uint64_t v) { tab = v; }
  void store(xiphos::core::Entry v) { entry = std::make_unique<xc_entry>(xc_entry{std::move(v)}); }
  void store(xiphos::core::Chapter v) { chapter = std::make_unique<xc_chapter>(xc_chapter{std::move(v)}); }
  void store(xiphos::core::ReferenceChoices v) { choices = std::make_unique<xc_choices>(xc_choices{std::move(v)}); }
  void store(xiphos::core::PassageLink v) { link = std::make_unique<xc_link>(xc_link{std::move(v)}); }
  void store(std::vector<xiphos::core::Module> v) { catalog = std::make_unique<xc_catalog>(xc_catalog{std::move(v)}); }
  void store(xiphos::core::HistorySnapshot v) { history = std::make_unique<xc_history>(xc_history{std::move(v)}); }
};
namespace {
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
const char *xc_entry_reference(const xc_entry *e) { return e ? e->value.reference.c_str() : nullptr; }
const char *xc_entry_html(const xc_entry *e) { return e ? e->value.html.c_str() : nullptr; }
void xc_entry_release(xc_entry *e) { delete e; }
}
