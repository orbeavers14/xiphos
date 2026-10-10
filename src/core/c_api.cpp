// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/c_api.h"
#include "xiphos/core/reader.hpp"
#include <stdexcept>
struct xc_reader { xiphos::core::Reader value; explicit xc_reader(const char *p) : value(p ? p : "") {} };
struct xc_entry { xiphos::core::Entry value; };
namespace {
xc_status failure() noexcept {
  try { throw; }
  catch (const std::out_of_range &) { return XC_MODULE_NOT_FOUND; }
  catch (const std::invalid_argument &) { return XC_INVALID_ARGUMENT; }
  catch (...) { return XC_ENGINE_ERROR; }
}
const xiphos::core::Module *module_at(const xc_reader *r, size_t i) {
  return r && i < r->value.modules().size() ? &r->value.modules()[i] : nullptr;
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
size_t xc_module_count(const xc_reader *r) { return r ? r->value.modules().size() : 0; }
const char *xc_module_id(const xc_reader *r, size_t i) { auto m = module_at(r, i); return m ? m->id.c_str() : nullptr; }
const char *xc_module_description(const xc_reader *r, size_t i) { auto m = module_at(r, i); return m ? m->description.c_str() : nullptr; }
const char *xc_module_type(const xc_reader *r, size_t i) { auto m = module_at(r, i); return m ? m->type.c_str() : nullptr; }
const char *xc_module_language(const xc_reader *r, size_t i) { auto m = module_at(r, i); return m ? m->language.c_str() : nullptr; }
xc_status xc_read(xc_reader *r, const char *id, const char *ref, xc_entry **out) {
  if (!out) return XC_INVALID_ARGUMENT;
  *out = nullptr;
  if (!r || !id || !*id || !ref || !*ref) return XC_INVALID_ARGUMENT;
  try { *out = new xc_entry{r->value.read(id, ref)}; return XC_OK; }
  catch (...) { return failure(); }
}
const char *xc_entry_reference(const xc_entry *e) { return e ? e->value.reference.c_str() : nullptr; }
const char *xc_entry_html(const xc_entry *e) { return e ? e->value.html.c_str() : nullptr; }
void xc_entry_release(xc_entry *e) { delete e; }
}
