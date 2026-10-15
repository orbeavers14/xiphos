// SPDX-License-Identifier: GPL-2.0-or-later
#include "reader_model.hpp"
#include "reader_document.hpp"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <thread>
using namespace xiphos::linux_preview;
void check(bool ok, const char *message) { if (!ok) throw std::runtime_error(message); }
void settle(ReaderModel &model) {
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
  while (model.busy()) {
    model.tick(); check(std::chrono::steady_clock::now() < deadline, "lexical model deadline");
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
}
int main(int argc, char **argv) {
  try {
    check(argc == 2, "private lexical fixture required");
    ReaderModel model(argv[1]); settle(model);
    check(model.visit("Text", "", "John4:1"), "read original styled fixture"); settle(model);
    check(!model.passage().bible_lexical.requested && model.passage().bible_lexical.css.empty(), "disabled preferences have no annotation CSS");
    check(model.set_filter(XC_ENGINE_STRONGS, true), "enable declared Strong's option"); settle(model);
    const auto css = model.passage().bible_lexical.css;
    check(model.error().empty() && model.passage().bible_lexical.applied && css.find("line-height: 2.7em") != std::string::npos &&
          model.passage().bible_html.find("class=\"word\"") != std::string::npos && !model.can_back(), "Linux model copies matching core HTML/CSS without a history visit");
    const auto html = document(model.passage().bible_html, css);
    check(html.find(css) != std::string::npos && html.find("default-src 'none'") != std::string::npos &&
          html.find("script-src 'none'") != std::string::npos, "Linux document embeds copied core CSS and retains resource restrictions");
    check(model.set_content(false, false, false), "hide images"); settle(model);
    check(model.set_content(false, true, false), "ignore font faces"); settle(model);
    check(model.passage().bible_html.find("<img ") == std::string::npos && model.passage().bible_html.find("Original Lexical Font") == std::string::npos &&
          model.passage().bible_lexical.applied, "Linux model retains core layout/cleanup ordering");
    check(model.set_display(true, XC_CHAPTER_VERSE_NUMBERS | XC_CHAPTER_HIGHLIGHT_SELECTED), "chapter display"); settle(model);
    check(model.bible_chapter().lexical.applied && model.bible_chapter().lexical.css == css && model.bible_chapter().anchor == "verse-1",
          "Linux chapter exposes matching CSS and core anchor");
    const auto profile = std::filesystem::path(argv[1]) / "linux-lexical-profile.conf";
    { std::ofstream file(profile); file << "[Text]\nStrong's Numbers=On\nImage Content=Off\nRespect Font Faces=Off\n[ThMLNotes]\nStrong's Numbers=On\n"; }
    check(model.reading_profile(profile.string(), false), "import generated options"); settle(model);
    check(model.choose_commentary("ThMLNotes"), "paired commentary"); settle(model);
    check(model.passage().commentary_lexical.applied && model.commentary_chapter().lexical.applied &&
          model.commentary_chapter().lexical.css == css, "Linux paired commentary copies independently owned layout metadata");
    const auto retained = model.passage();
    check(model.visit("Missing", "", "John4:1"), "submit unavailable module"); settle(model);
    check(!model.error().empty() && model.passage().bible_html == retained.bible_html && model.passage().bible_lexical.css == retained.bible_lexical.css,
          "failed Linux action retains HTML and CSS together");
    check(model.visit("Text", "ThMLNotes", "John5:1"), "unsupported chapter"); settle(model);
    check(model.error().empty() && model.bible_chapter().lexical.requested && !model.bible_chapter().lexical.applied &&
          model.bible_chapter().lexical.css.empty() && !model.commentary_chapter().lexical.applied &&
          document(model.bible_chapter().html, model.bible_chapter().lexical.css).find("position: absolute") == std::string::npos,
          "Linux raw fallback never inherits annotation positioning CSS");
    auto *worker = model.detach(); std::thread([worker] { xc_worker_close(worker); }).join();
    check(retained.bible_lexical.css == css && retained.commentary_lexical.css == css, "Linux copied layout outlives model and worker");
    std::cout << "Linux lexical model/document copy, cleanup, fallback and failed-action guards passed\n";
  } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
