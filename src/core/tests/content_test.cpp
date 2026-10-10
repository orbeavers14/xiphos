// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/content.hpp"
#include <iostream>
#include <stdexcept>
using namespace xiphos::core;
void require(bool ok, const char *why) { if (!ok) throw std::runtime_error(why); }
int main() {
  try {
    Chapter chapter;
    chapter.selected_verse = 2;
    chapter.verses = {{"A \"<&'", "<em>Original text</em>"}, {"B", ""}};
    const auto html = display_chapter(chapter, {});
    require(html.find("A &quot;&lt;&amp;&#39;") != std::string::npos, "reference attributes escaped");
    require(html.find("<em>Original text</em>") != std::string::npos, "engine markup retained");
    require(html.find("id=\"verse-2\" class=\"xc-verse xc-current\"") != std::string::npos, "selected position marked");
    require(html.find("aria-current=\"location\"") != std::string::npos, "selection semantics");
    require(html.find("<sup>2</sup>") != std::string::npos, "numbers include missing text");
    const auto plain = display_chapter(chapter, {false, false});
    require(plain.find("<sup>") == std::string::npos && plain.find("xc-current") == std::string::npos,
            "display options respected");
    require(plain.find("aria-current") != std::string::npos && plain.find("verse-2") != std::string::npos,
            "anchor and selection retained without highlighting");
    require(display_chapter({}, {}).empty(), "empty chapter fragment");
    std::cout << "Chapter presentation checks passed\n";
  } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
