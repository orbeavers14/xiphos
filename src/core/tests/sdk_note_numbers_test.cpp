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
int main(int argc, char **argv) {
  try {
    check(argc == 2, "isolated SDK module directory required");
    const std::filesystem::path root(argv[1]); std::filesystem::create_directories(root);
    check(sword::RawText::createModule(root.string().c_str()) == 0, "create original SDK marker module");
    sword::RawText module(root.string().c_str(), "OriginalMarker", "Original marker filter fixture");
    module.setKey("John3:16"); module.setEntry("Original module body.");
    UnrelatedFilter unrelated; AttachedFilter attached(module, unrelated);
    inspect<sword::OSISXHTML>("OSIS", "<note", "</note>", module);
    inspect<sword::ThMLXHTML>("ThML", "<note", "</note>", module);
    inspect<sword::GBFXHTML>("GBF", "<RF", "<Rf>", module);
    inspect<sword::TEIXHTML>("TEI", "<note", "</note>", module);
    check(module.getRenderFilters().size() == 1 && unrelated.calls == 0,
          "filter mutation ignores unrelated filters and fixture detaches every temporary filter");
  } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
