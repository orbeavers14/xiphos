// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/c_api.h"
#include "xiphos/core/reader.hpp"
#include <rawtext.h>
#include <rawld.h>
#include <swmgr.h>
#include <swmodule.h>
#include <markupfiltmgr.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <cstring>
extern "C" int c_header_test(void);
void check(bool condition, const char *message) {
  if (!condition) throw std::runtime_error(message);
}
int main(int argc, char **argv) {
  try {
    check(argc == 2, "fixture path required");
    const auto root = std::filesystem::path(argv[1]);
    // Use a generated module with original test prose; no network, proprietary
    // Bible content, or dependence on the developer's installed library.
    std::filesystem::create_directories(root / "mods.d");
    std::filesystem::create_directories(root / "modules/texts/rawtext/test");
    auto data = (root / "modules/texts/rawtext/test").string();
    check(sword::RawText::createModule(data.c_str()) == 0, "create fixture");
    {
      sword::RawText writer(data.c_str());
      writer.setKey("John 3:16");
      writer.setEntry("<verse osisID=\"John.3.16\">Original fixture: café &amp; peace.</verse>");
      writer.setKey("John 3:17");
      writer.setEntry("<verse osisID=\"John.3.17\">Second fixture verse.</verse>");
    }
    {
      std::ofstream config(root / "mods.d/test.conf");
      config << "[ParityTest]\nDataPath=./modules/texts/rawtext/test/\n"
                "ModDrv=RawText\nSourceType=OSIS\nEncoding=UTF-8\n"
                "Lang=en\nDescription=Original parity fixture\nVersification=KJV\n";
    }
    check(c_header_test(), "C header and symbols");
    xc_reader *r = nullptr;
    check(xc_open(root.string().c_str(), &r) == XC_OK && r, "open fixture");
    check(xc_module_count(r) == 1, "explicit root must not augment home modules");
    check(std::strcmp(xc_module_id(r, 0), "ParityTest") == 0, "module id");
    check(std::strcmp(xc_module_language(r, 0), "en") == 0, "language");
    check(std::strcmp(xc_module_type(r, 0), "Biblical Texts") == 0, "type");
    check(xc_module_description(r, 1) == nullptr, "bounds");
    check(xc_module_count(nullptr) == 0 && xc_module_id(nullptr, 0) == nullptr, "null reader");
    xc_entry *entry = nullptr;
    check(xc_read(r, "ParityTest", "John 3:16", &entry) == XC_OK, "read verse");
    const std::string actual = xc_entry_html(entry);
    check(actual.find("café") != std::string::npos && actual.find("peace") != std::string::npos, "UTF-8 text");
    check(actual.find("<verse") == std::string::npos, "OSIS rendered to XHTML");
    check(std::string(xc_entry_reference(entry)).find("3:16") != std::string::npos, "canonical reference");
    // Compare against the old backend's exact setKey/renderText sequence.
    sword::SWMgr legacy(root.string().c_str(), true,
                        new sword::MarkupFilterMgr(sword::FMT_XHTML), false, false);
    auto *module = legacy.getModule("ParityTest");
    check(module != nullptr, "legacy module");
    module->setKey("John 3:16");
    check(actual == module->renderText().c_str(), "legacy rendering equivalence");
    check(xiphos::core::render_entry(*module, "John 3:17").find("Second fixture") != std::string::npos, "shared legacy seam");
    check(std::string(module->getKeyText()).find("3:17") != std::string::npos, "legacy key mutation preserved");
    xc_entry *second = nullptr;
    check(xc_read(r, "ParityTest", "John 3:17", &second) == XC_OK, "second read");
    check(std::string(xc_entry_html(entry)) == actual, "result survives later read");
    xc_entry_release(second);
    check(xc_read(r, "Missing", "John 3:16", &second) == XC_MODULE_NOT_FOUND && !second, "missing module");
    check(xc_read(r, "ParityTest", "", &second) == XC_INVALID_ARGUMENT && !second, "empty reference");
    check(xc_read(nullptr, "ParityTest", "John 3:16", &second) == XC_INVALID_ARGUMENT, "null read");
    check(xc_read(r, "ParityTest", "John 3:16", nullptr) == XC_INVALID_ARGUMENT, "null out");
    xc_close(r);
    const auto dictroot = root / "dictionary";
    std::filesystem::create_directories(dictroot / "mods.d");
    std::filesystem::create_directories(dictroot / "data");
    check(sword::RawLD::createModule((dictroot / "data").string().c_str()) == 0, "create dictionary");
    {
      std::ofstream config(dictroot / "mods.d/dict.conf");
      config << "[DictionaryTest]\nDataPath=./data/\nModDrv=RawLD\nDescription=Test dictionary\n";
    }
    check(xc_open(dictroot.string().c_str(), &r) == XC_OK, "open dictionary");
    check(xc_module_count(r) == 1, "dictionary discovered");
    check(xc_read(r, "DictionaryTest", "peace", &second) == XC_INVALID_ARGUMENT && !second,
          "unsupported key type returns error");
    xc_close(r);
    check(std::string(xc_entry_html(entry)) == actual, "result survives reader close");
    xc_entry_release(entry);
    xc_entry_release(nullptr);
    xc_close(nullptr);
    check(xc_open((root / "missing").string().c_str(), &r) == XC_INVALID_ARGUMENT && !r, "bad root");
    std::filesystem::create_directories(root / "empty/mods.d");
    check(xc_open((root / "empty").string().c_str(), &r) == XC_OK, "empty library opens");
    check(xc_module_count(r) == 0, "empty library");
    xc_close(r);
    std::cout << "Core/C ABI/legacy rendering checks passed\n";
    return 0;
  } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
