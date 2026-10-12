// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/html_cleanup.hpp"
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
    auto run = [](LegacyCleanup cleanup, const std::string &input, int images, int fonts, bool reset, bool numbering = false) {
      GLOBAL_OPS options{images, fonts, numbering}; diagnostic.clear(); saved.clear(); timeline.clear(); footnote = xref = 0;
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
    backend_value.reported = false;
    const auto hidden = cleanup_html_content("<img alt='<font face=\"Original\">'>", ContentPreference::off, ContentPreference::unknown);
    check(hidden.respect_font_faces == ContentPreference::unknown && !hidden.discovered_font_faces, "font discovery sees post-image-cleanup content");
    const auto uppercase = cleanup_html_content("<IMG src=\"original.png\"><FONT FACE=\"Original\">", ContentPreference::unknown, ContentPreference::unknown);
    check(uppercase.discovered_images && uppercase.discovered_font_faces, "unknown-content detection uses case-insensitive legacy matching");
    bool rejected = false;
    try { cleanup_html_content("original", static_cast<ContentPreference>(2), ContentPreference::on); } catch (const std::invalid_argument &) { rejected = true; }
    check(rejected, "invalid content state rejected");
    std::cout << "Content cleanup frozen/current/owned checks passed: " << comparisons << " content combinations + " << numbering_checks << " legacy numbering checks + " << locale_checks << " cases in " << locales << " host locales\n";
  } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
