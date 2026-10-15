// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/note_markers.hpp"
#include "xiphos/core/note_rendering.hpp"
#include "xiphos/core/word_blocks.hpp"
#include "xiphos/core/link.hpp"
#include <osisxhtml.h>
#include <thmlxhtml.h>
#include <gbfxhtml.h>
#include <teixhtml.h>
#include <rawtext.h>
#include <versekey.h>
#include <filesystem>
#include <fstream>
#include <swmgr.h>
#include <markupfiltmgr.h>
#include <algorithm>
#include <iostream>
#include <stdexcept>
using namespace xiphos::core;
using namespace sword;
struct GLOBAL_OPS { int xrefnotenumbers; };
namespace frozen {
#include "legacy_render_note_numbers.inc"
}
namespace current {
#include "legacy_render_note_numbers_actual.inc"
}
struct UnrelatedFilter : sword::SWFilter {
  int calls = 0;
  char processText(sword::SWBuf &, const sword::SWKey *, const sword::SWModule *) override { ++calls; return 0; }
};
struct AttachedFilter {
  sword::SWModule &module; sword::SWFilter &filter;
  AttachedFilter(sword::SWModule &owner, sword::SWFilter &value) : module(owner), filter(value) { module.addRenderFilter(&filter); }
  ~AttachedFilter() { module.removeRenderFilter(&filter); }
  AttachedFilter(const AttachedFilter &) = delete;
  AttachedFilter &operator=(const AttachedFilter &) = delete;
};
void check(bool ok, const char *message) { if (!ok) throw std::runtime_error(message); }
std::string marker(const std::string &html) {
  const auto start = html.find(">*");
  check(start != std::string::npos, "SDK emits a note marker");
  const auto end = html.find('<', start + 1);
  check(end != std::string::npos, "SDK marker has a closing element");
  return html.substr(start + 1, end - start - 1);
}
template<class Filter> void inspect(const char *name, const char *open, const char *close, sword::RawText &module) {
  Filter filter, extra;
  AttachedFilter attached(module, filter), attached_extra(module, extra);
  sword::VerseKey key("John3:16");
  auto render_with = [&](Filter &target, const std::string &input) {
    sword::SWBuf text(input.c_str());
    check(target.processText(text, &key, &module) == 0, "SDK note filter returns success");
    return std::string(text.c_str());
  };
  auto render = [&](const std::string &input) { return render_with(filter, input); };
  unsigned cases = 0;
  for (const auto *type : {"study", "crossReference"}) for (const auto *label : {"", "a", "12", "α", "A B"}) {
    const std::string input = std::string("Original ") + open + " swordFootnote=\"7\" type=\"" + type + "\" n=\"" + label + "\">Original note body." + close + " end.";
    filter.setRenderNoteNumbers(false); const auto off = render(input);
    Filter fresh; sword::SWBuf baseline(input.c_str());
    check(fresh.processText(baseline, &key, &module) == 0 && std::string(baseline.c_str()) == off,
          "fresh SDK note filter defaults to explicit Off output");
    filter.setRenderNoteNumbers(true); const auto on = render(input);
    filter.setRenderNoteNumbers(false); check(render(input) == off, "SDK explicit off restores output after on");
    for (int flag : {0, 1, -1, 2}) {
      GLOBAL_OPS options{flag};
      filter.setRenderNoteNumbers(flag == 0); extra.setRenderNoteNumbers(flag == 0);
      frozen::set_render_numbers(module, &options);
      check(render(input) == (flag ? on : off) && render_with(extra, input) == (flag ? on : off), "frozen switch sets every matching SDK filter");
      filter.setRenderNoteNumbers(flag == 0); extra.setRenderNoteNumbers(flag == 0);
      current::set_render_numbers(module, &options);
      check(render(input) == (flag ? on : off) && render_with(extra, input) == (flag ? on : off), "actual legacy switch matches frozen filter state");
      filter.setRenderNoteNumbers(flag == 0); extra.setRenderNoteNumbers(flag == 0);
      set_render_note_numbers(module, flag != 0);
      check(render(input) == (flag ? on : off) && render_with(extra, input) == (flag ? on : off), "owned core switch matches all format filters and nonzero legacy flags");
    }
    filter.setRenderNoteNumbers(true); frozen::set_render_numbers(module, nullptr);
    check(render(input) == on, "frozen null options preserve SDK state");
    current::set_render_numbers(module, nullptr); check(render(input) == on, "actual null options preserve SDK state");
    {
      NoteRenderingState owner;
      filter.setRenderNoteNumbers(true); extra.setRenderNoteNumbers(true);
      bool rejected = false;
      try { ScopedNoteNumbers unknown(owner, module, false); } catch (const std::invalid_argument &) { rejected = true; }
      check(rejected && render(input) == on && render_with(extra, input) == on,
            "unowned note filters reject before any SDK mutation");
      owner.register_module(module, false);
      check(render(input) == off && render_with(extra, input) == off, "registration establishes an explicit baseline");
      {
        ScopedNoteNumbers outer(owner, module, true);
        owner.register_module(module, false);
        check(render(input) == on, "registering known filter aliases never overwrites active owned state");
        {
          ScopedNoteNumbers inner(owner, module, false);
          check(render(input) == off && render_with(extra, input) == off, "nested note scope applies its own value");
        }
        check(render(input) == on && render_with(extra, input) == on, "nested note scope restores every previous filter");
      }
      check(render(input) == off && render_with(extra, input) == off, "outer scope restores baseline");
      bool caught = false;
      try { ScopedNoteNumbers exceptional(owner, module, true); throw std::runtime_error("Original scope test"); }
      catch (const std::runtime_error &) { caught = true; }
      check(caught && render(input) == off && render_with(extra, input) == off, "exceptional scope restores owned SDK values");
    }
    const bool tei = std::string(name) == "TEI", gbf = std::string(name) == "GBF";
    const std::string prefix = !tei && !gbf && std::string(type) == "crossReference" ? "*x" : "*n";
    const std::string encoded = std::string(label) == "α" ? "%CE%B1" : std::string(label) == "A B" ? "A+B" : label;
    check(marker(off) == prefix && marker(on) == prefix + (tei ? "" : gbf ? encoded : label),
          "SDK labels preserve actual format-specific encoding and note-type behavior");
    const auto href = on.find("href=\"");
    check(href != std::string::npos, "SDK emits a semantic note URI");
    const auto end = on.find('"', href + 6);
    const auto uri = on.substr(href + 6, end - href - 6);
    const auto off_href = off.find("href=\"");
    const auto off_end = off.find('"', off_href + 6);
    check(off.substr(off_href + 6, off_end - off_href - 6) == uri, "SDK numbering flag affects label, never note target");
    if (tei) {
      bool rejected = false;
      try { study_note_link(uri, "OriginalMarker"); } catch (const std::invalid_argument &) { rejected = true; }
      check(rejected && uri.find("value=&") != std::string::npos,
            "characterize SDK TEI plain closing-note empty identifier; native parity remains gated");
    } else {
      const auto target = study_note_link(uri, "OriginalMarker");
      check(target.module == "OriginalMarker" && target.identifier == "7" && target.reference.find("3:16") != std::string::npos,
            "SDK labels do not change copied semantic note context");
    }
    const auto formatted = layout_lexical_content(on, true, false, false, false);
    check(formatted.html.find(uri) != std::string::npos, "owned lexical layout preserves SDK note URI");
    const auto numbered = number_note_markers(on, true, *label != '\0');
    check(numbered.html.find(uri) != std::string::npos &&
          marker(numbered.html) == (*label ? marker(on) : prefix + "1"),
          "owned literal numbering preserves SDK URI and respects module-supplied labels");
    ++cases;
  }
  std::cout << name << " label/default/toggle/URI/layout cases passed: " << cases << '\n';
}
void shared_filter_scopes(const std::filesystem::path &root) {
  std::filesystem::create_directories(root / "mods.d"); std::filesystem::create_directories(root / "data");
  check(sword::RawText::createModule((root / "data").string().c_str()) == 0, "create original shared-filter data");
  {
    sword::RawText writer((root / "data").string().c_str()); writer.setKey("John3:16");
    writer.setEntry("Original <note swordFootnote=\"7\" n=\"a\" type=\"study\">Original note.</note> body.");
  }
  for (const auto *name : {"First", "Second", "Third"}) {
    std::ofstream config(root / "mods.d" / (std::string(name) + ".conf"));
    config << '[' << name << "]\nDataPath=./data/\nModDrv=RawText\nEncoding=UTF-8\nSourceType=" << (std::string(name) == "Third" ? "ThML" : "OSIS") << '\n';
  }
  std::filesystem::remove(root / "mods.d/modules-conf.cache");
  sword::SWMgr manager(root.string().c_str(), true, new sword::MarkupFilterMgr(sword::FMT_XHTML), false, false);
  auto *first = manager.getModule("First"), *second = manager.getModule("Second"), *third = manager.getModule("Third");
  check(first && second && third, "load shared SDK renderer modules");
  bool alias = false;
  for (auto *filter : first->getRenderFilters()) if (dynamic_cast<sword::OSISXHTML *>(filter))
    alias = alias || std::find(second->getRenderFilters().begin(), second->getRenderFilters().end(), filter) != second->getRenderFilters().end();
  check(alias, "SDK reuses an OSIS XHTML render filter across modules");
  auto label = [](sword::SWModule &module) { module.setKey("John3:16"); return marker(module.renderText().c_str()); };
  set_render_note_numbers(*first, true);
  check(label(*first) == "*na" && label(*second) == "*na" && label(*third) == "*n", "unscoped SDK mutation is shared by renderer aliases only");
  {
    NoteRenderingState owner;
    owner.register_module(*first, false); owner.register_module(*second, true); owner.register_module(*third, false);
    check(label(*first) == "*n" && label(*second) == "*n" && label(*third) == "*n", "alias registration retains the established per-filter baseline");
    {
      ScopedNoteNumbers outer(owner, *first, true);
      check(label(*first) == "*na" && label(*second) == "*na" && label(*third) == "*n", "owned scope follows actual SDK pointer sharing");
      {
        ScopedNoteNumbers inner(owner, *second, false);
        check(label(*first) == "*n" && label(*third) == "*n", "alias inner scope temporarily overrides shared filter only");
      }
      check(label(*first) == "*na" && label(*second) == "*na", "alias inner scope restores outer shared state");
    }
    check(label(*first) == "*n" && label(*second) == "*n", "alias outer scope restores baseline");
    try { ScopedNoteNumbers one(owner, *first, true); ScopedNoteNumbers two(owner, *third, true); throw std::runtime_error("Original unwind test"); }
    catch (const std::runtime_error &) {}
    check(label(*first) == "*n" && label(*second) == "*n" && label(*third) == "*n", "exception unwinds separate shared renderer types");
  }
  {
    NoteRenderingState owner; owner.register_module(*first, true); owner.register_module(*third, false);
    {
      ScopedNoteNumbers off(owner, *second, false), on(owner, *third, true);
      check(label(*first) == "*n" && label(*third) == "*na", "different renderer baselines compose independently");
    }
    check(label(*first) == "*na" && label(*second) == "*na" && label(*third) == "*n", "owned scope restores true and false baselines exactly");
  }
  {
    NoteRenderingState partial; partial.register_module(*second, false);
    sword::SWFilter *third_filter = nullptr;
    for (auto *filter : third->getRenderFilters()) if (dynamic_cast<sword::ThMLXHTML *>(filter)) third_filter = filter;
    check(third_filter, "find separate renderer for partial-owner failure");
    {
      AttachedFilter borrowed(*first, *third_filter);
      bool rejected = false;
      try { ScopedNoteNumbers incomplete(partial, *first, true); } catch (const std::invalid_argument &) { rejected = true; }
      check(rejected && label(*second) == "*n" && label(*third) == "*n",
            "later unregistered filter rejects before changing an earlier registered alias");
    }
  }
  std::cout << "Shared SDK filter alias/nesting/baseline/exception cases passed\n";
}
int main(int argc, char **argv) {
  try {
    check(argc == 2, "isolated SDK module directory required");
    const std::filesystem::path root(argv[1]); std::filesystem::create_directories(root);
    check(sword::RawText::createModule(root.string().c_str()) == 0, "create original SDK marker module");
    {
    sword::RawText module(root.string().c_str(), "OriginalMarker", "Original marker filter fixture");
    module.setKey("John3:16"); module.setEntry("Original module body.");
    UnrelatedFilter unrelated; AttachedFilter attached(module, unrelated);
    inspect<sword::OSISXHTML>("OSIS", "<note", "</note>", module);
    inspect<sword::ThMLXHTML>("ThML", "<note", "</note>", module);
    inspect<sword::GBFXHTML>("GBF", "<RF", "<Rf>", module);
    inspect<sword::TEIXHTML>("TEI", "<note", "</note>", module);
    check(module.getRenderFilters().size() == 1 && unrelated.calls == 0,
          "filter mutation ignores unrelated filters and fixture detaches every temporary filter");
    }
    shared_filter_scopes(root / "shared");
  } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
