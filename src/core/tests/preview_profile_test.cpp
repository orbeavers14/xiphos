// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/profile.hpp"
#include <libxml/parser.h>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <stdexcept>
using namespace xiphos::core;
#define _(v) (v)
static xmlDocPtr xml_settings_doc;
#include "legacy_xml_profile_actual.inc"
namespace {
unsigned checks = 0;
void check(bool ok, const char *message) { ++checks; if (!ok) throw std::runtime_error(message); }
void compare(const std::string &xml) {
  xml_settings_doc = xmlReadMemory(xml.data(), static_cast<int>(xml.size()), nullptr, nullptr, XML_PARSE_NONET);
  check(xml_settings_doc != nullptr, "legacy XML getter fixture parses");
  auto *text = xml_get_value("misc", "xrefsinverselist");
  // Defined, bounded values only: actual getter plus settings.c's observed
  // atoi/default expression, not full load_settings_structure execution.
  const bool expected = text ? std::atoi(text) != 0 : true;
  xmlFree(text); xmlFreeDoc(xml_settings_doc);
  check(parse_study_preview_preferences(xml).cross_references_in_verse_list == expected, "getter/default/prefix agreement");
}
void rejected(const std::string &xml) {
  bool failed = false;
  try { (void)parse_study_preview_preferences(xml); } catch (const std::invalid_argument &) { failed = true; }
  check(failed, "invalid or out-of-range preference rejects");
}
}
int main(int argc, char **argv) {
  try {
    check(argc == 2, "disposable fixture path required");
    const auto root = std::filesystem::u8path(argv[1]);
    std::filesystem::create_directories(root);
    for (const auto &xml : {"<Xiphos/>", "<Xiphos><misc/></Xiphos>", "<Xiphos><misc/><misc><xrefsinverselist>0</xrefsinverselist></misc></Xiphos>"}) compare(xml);
    for (const std::string value : {"", " ", "0", "1", "2", "-1", "+1", "-0", "000", "  -2tail", "1.5", "0x1", "true", "false", "+", "2147483647", "-2147483648", "<![CDATA[0]]>", "<part>0</part>", "1<part>0</part>", "&#x31;"}) {
      compare("<Xiphos><misc><xrefsinverselist>" + value + "</xrefsinverselist><xrefsinverselist>1</xrefsinverselist><untouched>keep</untouched></misc><misc><xrefsinverselist>0</xrefsinverselist></misc></Xiphos>");
    }
    for (const auto &value : {"2147483648", "-2147483649", "999999999999999999999999"}) {
      const auto xml = std::string("<Xiphos><misc><xrefsinverselist>") + value + "</xrefsinverselist></misc><lexicons><greek>Keep</greek></lexicons></Xiphos>";
      rejected(xml);
      check(parse_study_profile(xml).lexicons.greek == "Keep", "lexicon-only import ignores invalid unrelated preview value");
    }
    for (const auto &xml : {"", "<Xiphos>", "<Other/>", "<!DOCTYPE Xiphos><Xiphos/>"}) rejected(xml);
    rejected(std::string("<Xiphos/>\0trailing", 18));
    rejected(std::string(16 * 1024 * 1024 + 1, 'x'));
    std::string utf16 = "\xff\xfe";
    for (char16_t c : std::u16string(u"<?xml version='1.0' encoding='UTF-16'?><Xiphos><misc><xrefsinverselist>0</xrefsinverselist></misc></Xiphos>")) { utf16.push_back(static_cast<char>(c & 255)); utf16.push_back(static_cast<char>(c >> 8)); }
    compare(utf16);
    const auto file = root / std::filesystem::u8path("preview-café.xml");
    { std::ofstream output(file, std::ios::binary); output.write(utf16.data(), static_cast<std::streamsize>(utf16.size())); }
    const auto permissions = std::filesystem::status(file).permissions();
    check(!read_study_preview_preferences(file.u8string()).cross_references_in_verse_list, "explicit UTF-8 path reads encoded file");
    std::ifstream input(file, std::ios::binary);
    const std::string retained((std::istreambuf_iterator<char>(input)), {});
    check(retained == utf16 && std::filesystem::status(file).permissions() == permissions, "read preserves bytes and permissions");
    for (const auto &path : {std::string(), root.u8string(), (root / "missing.xml").u8string(), std::string("bad\0path", 8)}) {
      bool failed = false; try { (void)read_study_preview_preferences(path); } catch (const std::exception &) { failed = true; }
      check(failed, "invalid path rejects");
    }
    std::future<StudyPreviewPreferences> jobs[8];
    for (unsigned i = 0; i < 8; ++i) jobs[i] = std::async(std::launch::async, [i] { return parse_study_preview_preferences("<Xiphos><misc><xrefsinverselist>" + std::to_string(i % 2) + "</xrefsinverselist></misc></Xiphos>"); });
    for (unsigned i = 0; i < 8; ++i) check(jobs[i].get().cross_references_in_verse_list == bool(i % 2), "independent copied parser results");
    std::cout << "Preview profile reader: " << checks << " checks passed\n";
  } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
