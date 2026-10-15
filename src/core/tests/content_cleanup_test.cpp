// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/html_cleanup.hpp"
#include "xiphos/core/note_markers.hpp"
#include <limits>
#include <thread>
#include <atomic>
#include <glib.h>
#include <cstdarg>
#include <clocale>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <stdexcept>
using namespace xiphos::core;
struct GLOBAL_OPS { int image_content, respect_font_faces; bool xrefnotenumbers; };
struct Backend {
  bool reported = false;
  char *get_entry_attribute(const char *, const char *, const char *, bool) { return reported ? g_strdup("a") : nullptr; }
} backend_value;
Backend *backend = &backend_value;
int footnote = 0, xref = 0;
std::string diagnostic;
std::vector<std::string> saved, timeline;
void log_message(const char *format, ...) {
  va_list args; va_start(args, format); char buffer[4096];
  vsnprintf(buffer, sizeof(buffer), format, args); va_end(args); diagnostic += buffer; timeline.push_back(std::string("diagnostic:") + buffer);
}
int main_save_module_options(const char *module, const char *key, int value) {
  saved.push_back(std::string(module) + ":" + key + ":" + std::to_string(value)); timeline.push_back("save:" + saved.back()); return 1;
}
#define XI_message(args) log_message args
namespace frozen {
#include "legacy_html_cleanup.inc"
#include "legacy_content_cleanup.inc"
}
#undef CLEAR_FONT_NAME
#undef CLEAR_FONT_NAME_LENGTH
namespace current {
#include "legacy_html_cleanup_actual.inc"
#include "legacy_content_cleanup_actual.inc"
}
#undef XI_message
#undef CLEAR_FONT_NAME
#undef CLEAR_FONT_NAME_LENGTH
void check(bool ok, const char *message) { if (!ok) throw std::runtime_error(message); }
int main() {
  try {
    using LegacyCleanup = GString *(*)(GString *, GLOBAL_OPS *, const char *, bool);
    struct Snapshot { std::string html, diagnostic; GLOBAL_OPS options; std::vector<std::string> saves, timeline; int notes, references; };
    auto run = [](LegacyCleanup cleanup, const std::string &input, int images, int fonts, bool reset, bool numbering = false, NoteNumberingState initial = NoteNumberingState()) {
      GLOBAL_OPS options{images, fonts, numbering}; diagnostic.clear(); saved.clear(); timeline.clear(); footnote = initial.footnotes; xref = initial.cross_references;
      auto *text = cleanup(g_string_new(input.c_str()), &options, "Original Café Module", reset);
      Snapshot snapshot{text->str, diagnostic, options, saved, timeline, footnote, xref}; g_string_free(text, true); return snapshot;
    };
    const std::vector<std::string> tokens = {
      "", "original café text.", "<img src=\"original.png\" />", "<IMG src=\"upper.png\">", "<img\tsrc=\"tab.png\">",
      "<img ", "<img alt=\"a > b\" src=\"original.png\">", "<font face=\"Original\">", "<FONT FACE=\"upper\">",
      "<font face='single'>", "<font size=\"2\" face=\"ordered\">", "<font face=\"unfinished", "</font>", "\"", ">"
    };
    unsigned comparisons = 0;
    for (const auto &first : tokens) for (const auto &second : tokens) for (const auto &third : tokens)
      for (int images : {-1, 0, 1}) for (int fonts : {-1, 0, 1}) for (bool reset : {false, true}) {
        const auto input = first + second + third;
        const auto old = run(frozen::CleanupContent, input, images, fonts, reset);
        const auto actual = run(current::CleanupContent, input, images, fonts, reset);
        const auto result = cleanup_html_content(input, static_cast<ContentPreference>(images), static_cast<ContentPreference>(fonts));
        check(old.html == actual.html && old.html == result.html && old.html.size() == input.size(), "owned content stage matches complete frozen/current cleanup bytes");
        check(old.options.image_content == actual.options.image_content && old.options.image_content == static_cast<int>(result.image_content) &&
              old.options.respect_font_faces == actual.options.respect_font_faces && old.options.respect_font_faces == static_cast<int>(result.respect_font_faces),
              "content discovery agrees with complete cleanup state");
        std::vector<std::string> events;
        if (result.discovered_images) events.push_back("Original Café Module:Image Content:1");
        if (result.discovered_font_faces) events.push_back("Original Café Module:Respect Font Faces:1");
        check(old.saves == actual.saves && old.saves == events && old.timeline == actual.timeline, "discovery events preserve legacy save intent/order");
        std::string messages;
        for (const auto &issue : result.diagnostics)
          messages += std::string(issue.kind == ContentCleanupKind::image ? "ClearImages: no img end: " : "ClearFontFaces: no closing double quote: ") + issue.suffix + "\n";
        check(old.diagnostic == actual.diagnostic && old.diagnostic == messages, "malformed diagnostics retain original suffix/order across later cleanup");
        ++comparisons;
      }
    const std::string initial_locale = std::setlocale(LC_ALL, nullptr);
    unsigned locale_checks = 0, locales = 0;
    for (const auto *locale : {"C", "en_US.UTF-8", "de_DE.UTF-8", "tr_TR.UTF-8"}) {
      if (!std::setlocale(LC_ALL, locale)) continue;
      ++locales;
      for (const auto &input : {"<IMG SRC=\"Original\">", "<FONT FACE=\"Original\">", "<ImG >", "Original café <font face=\"Body\">"})
        for (int images : {-1, 0, 1}) for (int fonts : {-1, 0, 1}) {
          const auto old = run(frozen::CleanupContent, input, images, fonts, false);
          const auto actual = run(current::CleanupContent, input, images, fonts, false);
          const auto result = cleanup_html_content(input, static_cast<ContentPreference>(images), static_cast<ContentPreference>(fonts));
          check(old.html == result.html && old.html == actual.html && old.timeline == actual.timeline &&
                old.options.image_content == static_cast<int>(result.image_content) &&
                old.options.respect_font_faces == static_cast<int>(result.respect_font_faces),
                "case-insensitive discovery agrees with local libc across available host locales");
          ++locale_checks;
        }
    }
    check(std::setlocale(LC_ALL, initial_locale.c_str()), "restore test locale");
    unsigned numbering_checks = 0;
    for (bool reported : {false, true}) for (bool reset : {false, true})
      for (int images : {-1, 0, 1}) for (int fonts : {-1, 0, 1}) {
        backend_value.reported = reported;
        const std::string input = "<img src=\"original.png\"><font face=\"Original\">Original *n *x *n body.</font>";
        const auto old = run(frozen::CleanupContent, input, images, fonts, reset, true);
        const auto actual = run(current::CleanupContent, input, images, fonts, reset, true);
        check(old.html == actual.html && old.notes == actual.notes && old.references == actual.references &&
              old.timeline == actual.timeline && old.options.image_content == actual.options.image_content &&
              old.options.respect_font_faces == actual.options.respect_font_faces,
              "adapted content stage preserves subsequent legacy numbering and callback ordering");
        ++numbering_checks;
      }
    unsigned owned_numbering_checks = 0;
    const std::vector<std::string> markers = {"", "Original café ", "*n", "*x", "*N *X", "*n7*x12", "**n*x*n",
      "<a href=\"passagestudy.jsp?value=1\"><small><sup>*n</sup></small></a>", "<span title=\"*x\">Original *n</span>"};
    for (const auto &left : markers) for (const auto &right : markers) for (bool enabled : {false, true})
      for (bool labelled : {false, true}) for (int start : {0, 8, 98, 998, 9998, 10000, std::numeric_limits<int>::max() - 16}) {
        backend_value.reported = labelled;
        const auto input = left + right;
        const NoteNumberingState initial(start, start / 2);
        const auto old = run(frozen::CleanupContent, input, 1, 1, false, enabled, initial);
        const auto actual = run(current::CleanupContent, input, 1, 1, false, enabled, initial);
        const auto result = number_note_markers(input, enabled, labelled, initial);
        check(old.html == actual.html && old.html == result.html && old.notes == actual.notes && old.notes == result.counters.footnotes &&
              old.references == actual.references && old.references == result.counters.cross_references,
              "owned literal numbering preserves complete frozen/current output, independent counters and four-digit limits");
        ++owned_numbering_checks;
      }
    const auto first = number_note_markers("Original *n *x", true, false);
    const auto second = number_note_markers("Next *x *n", true, false, first.counters);
    check(first.html == "Original *n1 *x1" && second.html == "Next *x2 *n2", "caller-owned counters continue across entries without global state");
    for (bool enabled : {false, true}) for (bool labelled : {false, true}) {
      bool nul = false, negative = false;
      try { number_note_markers(std::string("a\0*n", 4), enabled, labelled); } catch (const std::invalid_argument &) { nul = true; }
      try { number_note_markers("*n", enabled, labelled, {-1, 0}); } catch (const std::invalid_argument &) { negative = true; }
      check(nul && negative, "invalid text/counter state rejects independently of numbering eligibility");
    }
    bool overflow = false;
    try { number_note_markers("*n", true, false, {std::numeric_limits<int>::max(), 0}); } catch (const std::overflow_error &) { overflow = true; }
    check(overflow, "owned numbering rejects counter overflow instead of signed overflow");
    std::atomic<bool> mismatch(false); std::vector<std::thread> threads;
    for (int worker = 0; worker < 4; ++worker) threads.emplace_back([worker, &mismatch] {
      for (int i = 0; i < 100; ++i) {
        const auto value = number_note_markers("Original *n *x", true, false, {worker * 100, i});
        if (value.counters.footnotes != worker * 100 + 1 || value.counters.cross_references != i + 1) mismatch = true;
      }
    });
    for (auto &thread : threads) thread.join(); check(!mismatch, "owned numbering counters/output isolate concurrent callers");
    backend_value.reported = false;
    const auto hidden = cleanup_html_content("<img alt='<font face=\"Original\">'>", ContentPreference::off, ContentPreference::unknown);
    check(hidden.respect_font_faces == ContentPreference::unknown && !hidden.discovered_font_faces, "font discovery sees post-image-cleanup content");
    const auto uppercase = cleanup_html_content("<IMG src=\"original.png\"><FONT FACE=\"Original\">", ContentPreference::unknown, ContentPreference::unknown);
    check(uppercase.discovered_images && uppercase.discovered_font_faces, "unknown-content detection uses case-insensitive legacy matching");
    bool rejected = false;
    try { cleanup_html_content("original", static_cast<ContentPreference>(2), ContentPreference::on); } catch (const std::invalid_argument &) { rejected = true; }
    check(rejected, "invalid content state rejected");
    std::cout << "Content cleanup frozen/current/owned checks passed: " << comparisons << " content combinations + " << numbering_checks << " legacy numbering checks + " << owned_numbering_checks << " owned numbering comparisons + 400 concurrent calls + " << locale_checks << " cases in " << locales << " host locales\n";
  } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
