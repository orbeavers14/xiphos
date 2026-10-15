// SPDX-License-Identifier: GPL-2.0-or-later
#include "reader_model.hpp"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <thread>
using xiphos::linux_preview::ReaderModel;
using xiphos::linux_preview::Lexicons;
void check(bool value, const char *message) { if (!value) throw std::runtime_error(message); }
void settle(ReaderModel &model) {
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
  while (model.busy() || model.preview_busy()) { model.tick(); check(std::chrono::steady_clock::now() < deadline, "preview timeout"); std::this_thread::sleep_for(std::chrono::milliseconds(1)); }
}
void close(ReaderModel &model) { auto *worker = model.detach(); std::thread([worker] { xc_worker_close(worker); }).join(); }
std::string read(const std::filesystem::path &path) { std::ifstream input(path, std::ios::binary); return {std::istreambuf_iterator<char>(input), {}}; }
int main(int argc, char **argv) {
  try {
    check(argc == 4, "note/dictionary fixtures and scratch root required");
    const std::string body = "passagestudy.jsp?action=showNote&type=n&value=1&module=OptionOther&passage=John3%3A19";
    const std::string xref = "passagestudy.jsp?action=showNote&type=x&value=1&module=OptionOther&passage=John3%3A20";
    {
      ReaderModel model(argv[1]); settle(model); check(model.visit("OptionOther", "", "John 3:23"), "source visit"); settle(model);
      check(model.follow("OptionOther", body), "note click"); settle(model);
      check(model.note().html.find("Original other note") != std::string::npos && !model.can_back(), "owned note without Bible history");
      check(model.follow("OptionOther", xref), "xref click"); settle(model);
      check(!model.note().expression.empty() && model.note().html.size() > 0, "cross reference note");
      check(model.show_references(), "expand note passages"); settle(model);
      check(model.showing_references() && model.references().count == 5, "ordered duplicate references");
      check(model.show_note() && !model.showing_references(), "back to owned note");
      check(model.hover(body), "body hover"); settle(model);
      check(model.information().html.find("Original other note") != std::string::npos, "hover body information");
      const auto retained = model.information().html;
      check(model.hover("https://example.invalid/"), "unsupported hover submitted"); settle(model);
      check(model.information().html == retained && model.error().empty(), "failed hover retains information silently");
      check(model.hover(xref), "xref list hover"); settle(model);
      check(model.references().count == 5 && model.showing_references(), "hover core list routing");
      check(model.set_cross_reference_list(false), "alternate hover mode"); settle(model);
      check(!model.cross_reference_list() && model.hover(xref), "xref information hover"); settle(model);
      check(model.information().html.find("Back to John 3:23") != std::string::npos, "core target-only information links");
      check(model.hover(body), "old hover submitted");
      check(model.visit("OptionOther", "", "John 3:17"), "main action invalidates hover"); settle(model);
      check(model.passage().reference == "John 3:17" && model.information().html.find("Back to John 3:23") != std::string::npos, "late hover cannot replace retained information");
      check(model.hover(body), "pending hover before close"); close(model); model.tick(); check(!model.preview_busy(), "closed hover observation released");
    }
    {
      ReaderModel model(argv[2]); settle(model);
      Lexicons names{std::string("DictionaryOne"), std::string("DictionaryTwo"), std::string("DictionaryOne"), std::string("DictionaryOne"), std::string("DictionaryTwo")};
      check(model.set_lexicons(names), "set copied lexicons"); settle(model);
      check(model.lexicons() == names, "copied lexical preferences");
      check(model.visit("OriginalBible", "", "John 3:16"), "lexical source"); settle(model);
      const std::string lexical = "xiphos.url?action=showStrongs&type=Greek&value=alpha";
      check(model.hover(lexical), "lexical hover"); settle(model);
      check(model.information().html.find("DictionaryOne") != std::string::npos && model.dictionary_history_count() == 0, "lexical hover has no dictionary visit");
      check(model.follow("OriginalBible", lexical), "lexical click"); settle(model);
      check(model.dictionary().module == "DictionaryOne" && model.dictionary_history_count() == 1 && !model.can_back(), "main click uses core dictionary history only");
      check(model.dictionary().source_module == "OriginalBible" && model.dictionary().source_reference == "John 3:16", "lexical dictionary retains source context");
      check(model.visit("OriginalBible", "", "John 3:17"), "new main passage after dictionary"); settle(model);
      check(model.step_dictionary(1), "step older dictionary view"); settle(model);
      check(model.dictionary().source_reference == "John 3:16", "key step preserves older source context");
      check(model.lookup_dictionary("DictionaryOne", "alpha"), "new direct lookup source"); settle(model);
      check(model.dictionary().source_reference == "John 3:17", "new lookup captures live source");
      const std::filesystem::path scratch(argv[3]); std::filesystem::create_directories(scratch); const auto profile = scratch / "settings.xml";
      { std::ofstream output(profile); output << "<Xiphos><misc><saved>preserve café</saved></misc></Xiphos>"; }
      check(model.lexicon_profile(profile.string(), true), "save XML choices"); settle(model);
      check(model.error().empty(), "XML save succeeds"); const auto saved = read(profile);
      check(saved.find("preserve café") != std::string::npos, "XML save preserves unrelated text");
      auto changed = names; changed[0] = "Unavailable"; check(model.set_lexicons(changed), "unavailable lexicon retained"); settle(model);
      check(model.lexicon_profile(profile.string(), false), "import XML choices"); settle(model);
      check(model.error().empty() && model.lexicons() == names && read(profile) == saved, "import restores choices read-only");
      check(model.lexicon_profile((scratch / "missing.xml").string(), false), "missing XML submitted"); settle(model);
      check(!model.error().empty() && model.lexicons() == names, "failed XML import preserves preferences");
      changed[0] = std::nullopt; changed[1] = ""; check(model.set_lexicons(changed), "unset and empty names"); settle(model);
      check(!model.lexicons()[0] && model.lexicons()[1] && model.lexicons()[1]->empty(), "nullable vs empty preservation");
      check(model.lexicon_profile(profile.string(), true), "non-round-trippable choices submitted"); settle(model);
      check(!model.error().empty() && read(profile) == saved, "invalid XML save preserves existing file"); close(model);
    }
    std::cout << "Linux note/reference/lexical hover and XML preference checks passed\n"; return 0;
  } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
