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
    const std::string query = "action=showNote&type=n&value=1&module=Option%26Greek&passage=John+3%3A16";
    const auto note = study_note_link("passagestudy.jsp?" + query, "Current");
    require(note.module == "Option&Greek" && note.reference == "John 3:16" && note.identifier == "1" && note.type == "n",
            "note parameters decode individually without splitting encoded delimiters");
    const auto nativeNote = study_note_link("xiphos-preview://study/passagestudy.jsp?" + query, "Current");
    require(nativeNote.module == note.module && nativeNote.reference == note.reference, "native local note origin");
    const auto fallbackNote = study_note_link("xiphos.url?passage=Ésaïe%203%3A1&type=x&action=showNote&value=2", "Current");
    require(fallbackNote.module == "Current" && fallbackNote.reference == "Ésaïe 3:1" && fallbackNote.type == "x", "note fallback module/UTF-8/reordered parameters");
    for (const auto bad : {"https://example/passagestudy.jsp?", "xiphos-preview://other/passagestudy.jsp?",
                          "file:///passagestudy.jsp?", "passagestudy.jsp#fragment?", "Passagestudy.jsp?"}) {
      bool rejected = false;
      try { study_note_link(std::string(bad) + query, "Current"); } catch (const std::invalid_argument &) { rejected = true; }
      require(rejected, "external or ambiguous note origin rejected");
    }
    for (const auto bad : {"action=showNote&type=n&value=1", "action=showStrongs&type=n&value=1&passage=John3:16",
                          "action=showNote&type=z&value=1&passage=John3:16", "action=showNote&type=n&value=&passage=John3:16",
                          "action=showNote&type=n&value=1&passage=John3:16&value=2", "action=showNote&type=n&value=1&passage=John%003:16",
                          "action=showNote&type=n&value=1&passage=John%2", "action=showNote&type=n&value=1&passage=John%GG",
                          "action=showNote&type=n&value=1&passage=John3:16-17", "action=showNote&type=n&value=1&passage=John3:16&",
                          "action=showNote&type=n&value=1&passage=John3:16&unknown=1"}) {
      bool rejected = false;
      try { study_note_link(std::string("passagestudy.jsp?") + bad, "Current"); } catch (const std::invalid_argument &) { rejected = true; }
      require(rejected, "malformed/unsupported note query rejected");
    }
    const auto reference = study_link("xiphos-preview://study/passagestudy.jsp?action=showRef&type=scripRef&value=John.3.16&module=", "Current");
    require(std::holds_alternative<PassageLink>(reference) && std::get<PassageLink>(reference).module == "Current" &&
            std::get<PassageLink>(reference).reference == "John.3.16", "generated reference command defaults to note source module");
    const auto explicitReference = study_link("xiphos.url?module=Other%26Greek&value=Ésaïe%203%3A1&type=scripRef&action=showRef", "Current");
    require(std::get<PassageLink>(explicitReference).module == "Other&Greek" && std::get<PassageLink>(explicitReference).reference == "Ésaïe 3:1",
            "reference explicit module/Unicode/parameter order");
    require(std::holds_alternative<StudyNoteLink>(study_link("passagestudy.jsp?" + query, "Current")), "shared command dispatch retains notes");
    require(std::holds_alternative<PassageLink>(study_link("sword:///John+3%3A16", "Current")), "shared command dispatch retains passage URIs");
    for (const auto bad : {"action=showRef&type=wrong&value=John.3.16", "action=showRef&type=scripRef&value=",
                          "action=showRef&type=scripRef&value=John.3.16-John.3.17", "action=showRef&type=scripRef&value=John.3.16,John.3.17",
                          "action=showRef&type=scripRef&value=John.3.16&value=John.3.17", "action=showRef&type=scripRef&value=John.3.16&passage=John3:17",
                          "action=showRef&type=scripRef&value=John%00", "action=showRef&type=scripRef&value=John%GG"}) {
      bool rejected = false;
      try { study_link(std::string("passagestudy.jsp?") + bad, "Current"); } catch (const std::invalid_argument &) { rejected = true; }
      require(rejected, "invalid/ambiguous/range reference commands reject");
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
    ReferenceList references;
    references.entries = {{"A \"<&'", "<em>Original body</em>"}, {"A \"<&'", ""}};
    const auto referenceHTML = display_reference_list(references);
    require(referenceHTML.find("A &quot;&lt;&amp;&#39;") != std::string::npos && referenceHTML.find("<em>Original body</em>") != std::string::npos,
            "reference labels escaped and engine body retained");
    require(referenceHTML.find("reference-1") != std::string::npos && referenceHTML.find("reference-2") != std::string::npos,
            "duplicate reference entries retain unique positions");
    require(display_reference_list({}).empty(), "empty reference-list fragment");
    std::cout << "Chapter presentation checks passed\n";
  } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
