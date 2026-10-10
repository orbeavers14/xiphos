// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/c_api.h"
#include "xiphos/core/reader.hpp"
#include <stdexcept>
#include "xiphos/core/session.hpp"
struct xc_reader { std::shared_ptr<xiphos::core::Reader> value; explicit xc_reader(const char *p) : value(std::make_shared<xiphos::core::Reader>(p ? p : "")) {} };
struct xc_session {
  std::shared_ptr<xiphos::core::Reader> owner;
  xiphos::core::StudySession value;
  explicit xc_session(xc_reader &r) : owner(r.value), value(*owner) {}
};
struct xc_chapter { xiphos::core::Chapter value; };
struct xc_link { xiphos::core::PassageLink value; };
struct xc_entry { xiphos::core::Entry value; };
namespace {
xc_status failure() noexcept {
  try { throw; }
  catch (const std::out_of_range &) { return XC_MODULE_NOT_FOUND; }
  catch (const std::invalid_argument &) { return XC_INVALID_ARGUMENT; }
  catch (...) { return XC_ENGINE_ERROR; }
}
const xiphos::core::Module *module_at(const xc_reader *r, size_t i) {
  return r && i < r->value->modules().size() ? &r->value->modules()[i] : nullptr;
}
}
extern "C" {
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
