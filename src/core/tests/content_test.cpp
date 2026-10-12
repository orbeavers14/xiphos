// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/content.hpp"
#include "xiphos/core/link.hpp"
#include <iostream>
#include <stdexcept>
using namespace xiphos::core;
void require(bool ok, const char *why) { if (!ok) throw std::runtime_error(why); }
int main() {
  try {
    const auto link = passage_link("sword:///John+3%3A16", "Current");
    require(link.module == "Current" && link.reference == "John 3:16", "legacy decoding and default module");
    const auto explicit_link = passage_link("bible://Other/John%203:17", "Current");
    require(explicit_link.module == "Other" && explicit_link.reference == "John 3:17", "explicit module");
    require(passage_link(passage_uri("Ésaïe 3:1"), "Current").reference == "Ésaïe 3:1", "UTF-8 round trip");
    for (const auto bad : {"https://example/sword:///John3:16", "sword://", "sword:///", "sword:///John3:16-17",
                          "sword:///John3:16#note", "sword:///John3:16!note", "sword:///John%003:16", "passagestudy.jsp?action=showNote"}) {
      bool rejected = false;
      try { passage_link(bad, "Current"); } catch (const std::invalid_argument &) { rejected = true; }
      require(rejected, "unsupported links rejected");
    }
    Chapter chapter;
    chapter.selected_verse = 2;
    chapter.verses = {{"A \"<&'", "<em>Original text</em>"}, {"B", ""}};
    const auto html = display_chapter(chapter, {});
    require(html.find("A &quot;&lt;&amp;&#39;") != std::string::npos, "reference attributes escaped");
    require(html.find("<em>Original text</em>") != std::string::npos, "engine markup retained");
    require(html.find("id=\"verse-2\" class=\"xc-verse xc-current\"") != std::string::npos, "selected position marked");
    require(html.find("aria-current=\"location\"") != std::string::npos, "selection semantics");
    require(html.find(">2</a></sup>") != std::string::npos, "numbers include missing text");
    const auto plain = display_chapter(chapter, {false, false});
    require(plain.find("<sup>") == std::string::npos && plain.find("xc-current") == std::string::npos,
            "display options respected");
    require(plain.find("aria-current") != std::string::npos && plain.find("verse-2") != std::string::npos,
            "anchor and selection retained without highlighting");
    require(display_chapter({}, {}).empty(), "empty chapter fragment");
    std::cout << "Chapter presentation checks passed\n";
  } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
