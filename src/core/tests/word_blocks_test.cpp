// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/word_blocks.hpp"
#include <atomic>
#include <thread>
#include <swbuf.h>
#include <swmgr.h>
#include <rawtext.h>
#include <markupfiltmgr.h>
#include <glib.h>
#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstring>
#include <cstdarg>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
using sword::SWBuf;
using std::max;
std::string warning;
void log_warning(const char *format, ...) {
  va_list args; va_start(args, format); char buffer[4096];
  vsnprintf(buffer, sizeof(buffer), format, args); va_end(args); warning += buffer;
}
#define XI_warning(args) log_warning args
namespace frozen {
#include "legacy_word_blocks.inc"
}
#undef ALIGN_WORD
#undef EMPTY_WORD
namespace current {
#include "legacy_word_blocks_actual.inc"
}
#undef ALIGN_WORD
#undef EMPTY_WORD
#undef XI_warning
#include "legacy_lexical_style.inc"
#undef CSS_BLOCK_BOTH
#undef CSS_BLOCK_ONE
#undef DOUBLE_SPACE
void check(bool ok, const char *message) { if (!ok) throw std::runtime_error(message); }
struct Snapshot { std::string html, warning, input; };
Snapshot run(const char *(*render)(const char *), const std::string &input) {
  std::vector<char> copy(input.begin(), input.end()); copy.resize(copy.size() + 8, '\0');
  warning.clear(); const std::string html = render(copy.data());
  return {html, warning, copy.data()};
}
int main(int argc, char **argv) {
  try {
    check(argc == 2, "original word block fixture directory required");
    const auto strong = [](const std::string &code, bool classes, bool bang = false) {
      return "<small><em>&lt;<a href=\"passagestudy.jsp?action=showStrongs&type=Greek&value=" +
          (bang ? std::string("!") : std::string()) + code + "\"" + (classes ? " class=\"strongs\"" : "") + ">" + code + "</a>&gt;</em></small>";
    };
    const auto morph = [](const std::string &code, bool classes) {
      return "<small><em>(<a href=\"passagestudy.jsp?action=showMorph&type=Greek&value=" + code + "\"" +
          (classes ? " class=\"morph\"" : "") + ">" + code + "</a>)</em></small>";
    };
    unsigned comparisons = 0;
    auto compare = [&](const std::string &input) {
      const auto old = run(frozen::block_render, input), actual = run(current::block_render, input);
      check(old.html == actual.html && old.warning == actual.warning && old.input == actual.input,
            "complete frozen/current word block dataflow agrees on bounded valid markup");
      const auto original = input;
      const auto owned = xiphos::core::render_word_blocks(input);
      auto expected = old.html;
      // Correct the characterized recursive static-tag defect for outer font
      // switches; valid lexical content/spacing remains the frozen contract.
      for (const auto *tag : {"i", "b"}) {
        const std::string opening = std::string("<") + tag + ">", closing = std::string("</") + tag + ">";
        if (input.compare(0, opening.size(), opening) == 0 && input.size() >= closing.size() &&
            input.compare(input.size() - closing.size(), closing.size(), closing) == 0) {
          const auto last = expected.rfind("</");
          if (last != std::string::npos) expected.replace(last, expected.size() - last, closing);
        }
      }
      if (!owned.applied || owned.html != expected) { std::cerr << "Owned mismatch for: " << input << "\n"; }
      check(owned.applied && owned.html == expected && input == original, "owned valid rendering preserves frozen layout with local closing-tag correction and immutable input");
      ++comparisons; return old;
    };
    for (const auto &word : {"", "Original", "Original café", "λόγος", "L<font size=\"-1\">ORD</font>", "<a href=\"sword:///John%203:16\">Original link</a>"})
      for (const auto &code : {"3056", "00001", "123456789"}) for (bool classes : {false, true}) for (bool bang : {false, true}) {
        const auto s = strong(code, classes, bang);
        for (const auto &m : {std::string(), morph("N-NSM", classes), morph("V IMP AAI", classes), morph("HR/Ncmsc/Sp2mp", classes), morph("Ncmsc/Sp2mp", classes)}) {
          const auto plain = compare(std::string(word) + " " + s + m);
          check(plain.html.find("class=\"word\"") != std::string::npos && plain.html.find("class=\"strongs\"") != std::string::npos &&
                plain.html.find("class=\"morph\"") != std::string::npos, "annotation block classes retained");
          compare(std::string(word) + s + s + m + " Original following word.");
          compare("<i>" + std::string(word) + s + m + "</i>");
          compare("<b>" + std::string(word) + s + m + "</b>");
        }
      }
    for (const auto &input : {"Original plain text.", "Original\twords", "<i><i>Original</i></i>", "<i><b>Original</b></i>", "<p>Original paragraph.</p>"}) compare(input);
    const auto lemma = compare("Original " + strong("λόγος", true));
    check(lemma.html.find("λόγος") != std::string::npos && lemma.html.find("value=λόγος") != std::string::npos,
          "Unicode lemma annotation retains displayed text and lexical value");
    const auto lower = compare("Original " + morph("V IMP AAI", true));
    check(lower.html.find("v-imp-aai") != std::string::npos && lower.html.find("value=V IMP AAI") != std::string::npos,
          "morph display lowercases/spaces become hyphens without changing original URI value");
    const auto suffix = compare("Original " + morph("HR/Ncmsc/Sp2mp", true));
    check(suffix.html.find("hr/ncmsc</a>") != std::string::npos && suffix.html.find("value=HR/Ncmsc/Sp2mp") != std::string::npos,
          "Hebrew display suffix truncates while original link retains full code");
    for (const auto *code : {"", "S", "N/", "N/S", "N/Sp", "N/Sd", "N/SP", "N/SD", "N/Other", "N/Other/Third", "N//", "/Sp2mp", "V (IMP) AAI", "N\tNSM"})
      for (bool classes : {false, true}) compare("Original " + morph(code, classes));
    const auto duplicate = compare("<i><i>Original</i></i>");
    check(duplicate.input != "<i><i>Original</i></i>", "characterize inherited duplicate-style caller-buffer mutation");
    const auto nested = compare("<i><b>Original</b></i>");
    check(nested.html.find("</b></b>") != std::string::npos, "characterize inherited recursive static closing-tag defect");
    for (const auto &annotation : {strong("3056", true), morph("N-NSM", true), strong("3056", true) + morph("N-NSM", true)})
      for (const auto *tag : {"i", "b", "u", "I", "B", "U"}) for (const auto *word : {"", "Original"}) {
        const auto styled = std::string("<") + tag + ">" + word + "</" + tag + ">";
        compare(annotation + styled);
        compare(styled + annotation);
        compare(annotation + styled + annotation);
      }
    // Multiple anchors may arrive in one SDK annotation, rather than as repeated
    // small elements. Exercise the complete legacy formatting, not just labels.
    for (unsigned count = 1; count <= 8; ++count) for (bool classes : {false, true}) {
      std::string annotation = "<small><em>&lt;";
      for (unsigned n = 0; n < count; ++n) {
        const auto single = strong(n % 2 ? "4487" : "λόγος", classes);
        const auto anchor = single.find("<a ");
        annotation += (n ? ", " : "") + single.substr(anchor, single.find("</a>") + 4 - anchor);
      }
      annotation += "&gt;</em></small>";
      compare("Original " + annotation + morph("N-NSM", classes));
    }
    const std::filesystem::path root(argv[1]); std::filesystem::create_directories(root / "mods.d");
    std::filesystem::create_directories(root / "data");
    check(sword::RawText::createModule((root / "data").string().c_str()) == 0, "create original lexical block module");
    { sword::RawText writer((root / "data").string().c_str());
      writer.setKey("John 3:16"); writer.setEntry("<w lemma=\"strong:G3056\" morph=\"robinson:N-NSM\">Original λόγος</w> original body.");
      writer.setKey("John 3:17"); writer.setEntry("<w lemma=\"strong:H0123\" morph=\"oshm:HR/Ncmsc/Sp2mp\">Original Hebrew word</w> original body.");
      writer.setKey("John 3:18"); writer.setEntry("<p>Original paragraph <w lemma=\"strong:G3056 strong:G4487\" morph=\"robinson:N-NSM\">λόγος</w> with original following words.</p>");
      writer.setKey("John 3:19"); writer.setEntry("Original note <note type=\"study\" n=\"a\">Original note body.</note> and <reference osisRef=\"John.3.16\">original reference</reference>.");
      writer.setKey("John 3:20"); writer.setEntry("<p>Original line.</p><p>Original second paragraph with <w lemma=\"strong:H0123\" morph=\"oshm:Ncmsc/Sp2mp\">original word</w>.</p>");
      std::ofstream file(root / "mods.d/words.conf"); file << "[Words]\nDataPath=./data/\nModDrv=RawText\nSourceType=OSIS\nEncoding=UTF-8\nGlobalOptionFilter=OSISStrongs\nGlobalOptionFilter=OSISMorph\n";
    }
    std::filesystem::create_directories(root / "thml");
    check(sword::RawText::createModule((root / "thml").string().c_str()) == 0, "create original ThML lexical module");
    { sword::RawText writer((root / "thml").string().c_str());
      writer.setKey("John 3:16"); writer.setEntry("Original λόγος<sync type=\"Strongs\" value=\"G3056\"/><sync type=\"morph\" class=\"Robinson\" value=\"N-NSM\"/> original body.");
      writer.setKey("John 3:17"); writer.setEntry("<p>Original Hebrew word<sync type=\"Strongs\" value=\"H0123\"/> in an original paragraph.</p>");
      writer.setKey("John 3:18"); writer.setEntry("Original <scripRef passage=\"John 3:16\">passage link</scripRef> with original following words.");
      std::ofstream file(root / "mods.d/thml.conf");
      file << "[ThMLWords]\nDataPath=./thml/\nModDrv=RawText\nSourceType=ThML\nEncoding=UTF-8\nGlobalOptionFilter=ThMLStrongs\nGlobalOptionFilter=ThMLMorph\nGlobalOptionFilter=ThMLScripref\n";
    }
    std::filesystem::remove(root / "mods.d/modules-conf.cache");
    sword::SWMgr manager(root.string().c_str(), true, new sword::MarkupFilterMgr(sword::FMT_XHTML), false, false);
    auto *module = manager.getModule("Words"); check(module, "load lexical fixture");
    for (bool strongs : {false, true}) for (bool morphology : {false, true}) for (const auto *reference : {"John 3:16", "John 3:17", "John 3:18", "John 3:19", "John 3:20"}) {
      manager.setGlobalOption("Strong's Numbers", strongs ? "On" : "Off");
      manager.setGlobalOption("Morphological Tags", morphology ? "On" : "Off");
      module->setKey(reference); const std::string sdk = module->renderText().c_str();
      if (std::string(reference) == "John 3:19") {
        check(sdk.find("action=showNote") != std::string::npos && sdk.find("action=showRef") != std::string::npos &&
              compare(sdk).html.find("action=showNote") != std::string::npos,
              "SDK note-marker small elements retain complete note/reference links while formatting words");
        continue;
      }
      const auto rendered = compare(sdk);
      check(!rendered.html.empty() && rendered.warning.empty(), "complete block renderer accepts original SDK lexical output");
    }
    auto *thml = manager.getModule("ThMLWords"); check(thml, "load ThML lexical fixture");
    for (bool strongs : {false, true}) for (bool morphology : {false, true})
      for (const auto *reference : {"John 3:16", "John 3:17", "John 3:18"}) {
        manager.setGlobalOption("Strong's Numbers", strongs ? "On" : "Off");
        manager.setGlobalOption("Morphological Tags", morphology ? "On" : "Off");
        thml->setKey(reference); const std::string sdk = thml->renderText().c_str();
        const auto rendered = compare(sdk);
        check(!rendered.html.empty() && rendered.warning.empty(), "ThML SDK lexical/paragraph/reference output agrees with frozen layout");
        if (strongs && std::string(reference) == "John 3:16")
          check(sdk.find("action=showStrongs") != std::string::npos, "ThML fixture actually emits lexical links when enabled");
      }
    for (const auto &input : {"<small>unrecognized note</small>", "<b>unfinished", "Original > text", "<a href=\"unfinished>Original</a>",
                             "<small><em>(<a href=\"original\">λ</a>)</em></small>", "<img alt=\"a > b\" />"}) {
      const auto result = xiphos::core::render_word_blocks(input);
      check(!result.applied && result.html == input, "unsupported/malformed block markup preserves original HTML");
    }
    for (auto annotation : {strong("3056", true), morph("N-NSM", true)}) {
      annotation.replace(annotation.find("<a "), 3, "<A ");
      annotation.replace(annotation.find("</a>"), 4, "</A>");
      const auto result = xiphos::core::render_word_blocks("Original " + annotation);
      check(!result.applied && result.html == "Original " + annotation,
            "uppercase annotation anchors fall back before inherited lowercase-only pointer searches");
    }
    compare("<A href=\"sword:///John%203:16\">Original ordinary uppercase link</A>");
    for (const auto *label : {"a", "λόγος", "*n"}) {
      const std::string marker = "<a class=\"noteMarker study\" href=\"passagestudy.jsp?action=showNote&type=n&value=1&module=Words&passage=John+3%3A16\"><small><sup class=\"n\">" +
          std::string(label) + "</sup></small></a>";
      const auto rendered = compare("Original " + strong("3056", true) + marker + " following " + morph("N-NSM", true));
      check(rendered.html.find(marker) != std::string::npos, "lexical layout preserves complete opaque note marker byte-for-byte");
    }
    for (const auto *input : {"<a><small>Unrecognized marker</small></a>",
                             "<A href=\"x\"><small>Unrecognized marker</small></A>",
                             "<a href=\"x\"><A href=\"y\">Nested</A></a>",
                             "<a href=\"x\"><small>Missing close</a>",
                             "<a href=\"x\">Original</a><small>Not an annotation</small>"}) {
      const auto result = xiphos::core::render_word_blocks(input);
      check(!result.applied && result.html == input, "only balanced opaque supported anchors exempt note markers from lexical grammar");
    }
    std::string deep;
    for (unsigned n = 0; n < 65; ++n) deep += "<i>";
    deep += "Original"; for (unsigned n = 0; n < 65; ++n) deep += "</i>";
    check(!xiphos::core::render_word_blocks(deep).applied && !xiphos::core::render_word_blocks(std::string(1024 * 1024 + 1, 'x')).applied,
          "depth and input-size bounds preserve unsupported data");
    bool nul = false; try { xiphos::core::render_word_blocks(std::string("a\0b", 3)); } catch (const std::invalid_argument &) { nul = true; }
    check(nul, "embedded NUL rejected");
    for (const auto &styled : {std::string("<font face=\"Original\">Original styled word</font> "),
                              std::string("<em>Original styled word</em> ")}) {
      const auto input = styled + strong("3056", true);
      const auto inherited = xiphos::core::render_word_blocks(input);
      check(inherited.applied, "characterized low-level scanner remains compatible");
      for (bool spacing : {false, true}) {
        const auto result = xiphos::core::layout_lexical_content(input, true, false, false, spacing);
        check(result.requested && !result.applied && result.html == input &&
              result.css == frozen_lexical_style(false, false, false, spacing),
              "layout rejects scanner output whose inline tags cross word-span boundaries");
      }
    }
    const auto safe_font = xiphos::core::layout_lexical_content("<font face=\"Original\">Original</font> word " + strong("3056", true), true, false, false, false);
    check(safe_font.applied && !safe_font.css.empty(), "balanced single-word font retains supported layout");
    for (char level = '1'; level <= '6'; ++level) {
      const auto open = std::string("<h") + level + ">", close = std::string("</h") + level + ">";
      const auto body = "Original heading " + strong("3056", true);
      const auto result = xiphos::core::layout_lexical_content(open + body + close, true, false, false, false);
      check(result.applied && result.html == open + xiphos::core::render_word_blocks(body).html + close,
            "captured heading block wraps balanced word layout without entering the inline scanner");
    }
    unsigned policies = 0;
    const std::string layout_input = "Original " + strong("3056", true) + morph("N-NSM", true);
    for (unsigned bits = 0; bits < 16; ++bits) {
      const bool strongs = bits & 1, lemmas = bits & 2, morphology = bits & 4, spacing = bits & 8;
      const bool requested = strongs || lemmas || morphology;
      const auto result = xiphos::core::layout_lexical_content(layout_input, strongs, lemmas, morphology, spacing);
      check(result.requested == requested && result.applied == requested &&
            result.html == (requested ? run(frozen::block_render, layout_input).html : layout_input) &&
            result.css == frozen_lexical_style(strongs, lemmas, morphology, spacing),
            "layout HTML and CSS share the complete legacy option decision");
      const std::string unsupported = "Original <small>unsupported note</small>";
      const auto fallback = xiphos::core::layout_lexical_content(unsupported, strongs, lemmas, morphology, spacing);
      check(fallback.requested == requested && !fallback.applied && fallback.html == unsupported &&
            fallback.css == frozen_lexical_style(false, false, false, spacing),
            "unsupported content never receives annotation positioning CSS without word spans");
      bool rejected = false;
      try { xiphos::core::layout_lexical_content(std::string("a\0b", 3), strongs, lemmas, morphology, spacing); }
      catch (const std::invalid_argument &) { rejected = true; }
      check(rejected, "layout NUL validation is independent of selected display options");
      policies += 3;
    }
    unsigned random = 0x5eed;
    const std::string alphabet = "Original <>/smallabiuem=\"'&;()\t";
    for (unsigned n = 0; n < 8000; ++n) {
      std::string input;
      for (unsigned i = 0; i < n % 120; ++i) { random = random * 1664525u + 1013904223u; input += alphabet[random % alphabet.size()]; }
      const auto result = xiphos::core::render_word_blocks(input);
      check(result.applied || result.html == input, "bounded malformed corpus never drops unsupported input");
    }
    unsigned mutations = 0;
    for (const auto &seed : {std::string("Original ") + strong("3056", true) + morph("N-NSM", true),
                            std::string("<i><b>Original</b></i>"),
                            std::string("Original<a href=\"note\"><small><sup>a</sup></small></a> following")}) {
      auto verify = [&](const std::string &input) {
        const auto original = input;
        const auto result = xiphos::core::render_word_blocks(input);
        check(input == original && (result.applied || result.html == original),
              "structured grammar mutations preserve input and unsupported fallback");
        ++mutations;
      };
      for (std::size_t at = 0; at <= seed.size(); ++at) {
        if (at < seed.size()) { auto input = seed; input.erase(at, 1); verify(input); }
        for (const auto byte : std::string("<>/aA\"'&")) { auto input = seed; input.insert(at, 1, byte); verify(input); }
      }
    }
    const std::string concurrent_input = "<i><b>Original " + strong("3056", true) + morph("N-NSM", true) + "</b></i>";
    const auto concurrent_expected = xiphos::core::render_word_blocks(concurrent_input).html;
    std::atomic<bool> mismatch{false}; std::vector<std::thread> renderers;
    for (unsigned n = 0; n < 4; ++n) renderers.emplace_back([&] {
      for (unsigned i = 0; i < 100; ++i) {
        const auto result = xiphos::core::render_word_blocks(concurrent_input);
        if (!result.applied || result.html != concurrent_expected) mismatch = true;
        const auto layout = xiphos::core::layout_lexical_content(concurrent_input, i & 1, i & 2, i & 4, i & 8);
        if (layout.html != ((i & 7) ? concurrent_expected : concurrent_input) ||
            layout.css != frozen_lexical_style(i & 1, i & 2, i & 4, i & 8)) mismatch = true;
      }
    });
    for (auto &renderer : renderers) renderer.join(); check(!mismatch, "owned output and recursion state isolated across concurrent calls");
    std::cout << "Structured scanner mutations checked: " << mutations << " cases\n";
    std::cout << "Lexical layout policy checks: " << policies << " cases\n";
    std::cout << "Word block frozen/current checks passed: " << comparisons << " valid cases\n";
  } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
