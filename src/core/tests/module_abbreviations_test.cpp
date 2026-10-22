// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/module_abbreviations.hpp"
#include <cstring>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>
namespace frozen {
#include "module_abbreviations_legacy.inc"
}
namespace actual {
static xiphos::core::ModuleAbbreviations module_abbreviations;
#include "legacy_module_abbreviations_actual.inc"
}
void check(bool ok, const char *message) { if (!ok) throw std::runtime_error(message); }
bool same(const char *a, const char *b) { return (!a && !b) || (a && b && !std::strcmp(a,b)); }
int main() { try {
  xiphos::core::ModuleAbbreviations shared;
  const std::vector<std::string> probes = {"", "Text", "Other", "Third", "T", "t", "T2", "(bad)", "bad)", "café α", "文字", "Missing"};
  auto compare = [&] {
    for (const auto &key : probes) {
      check(same(frozen::main_name_to_abbrev(key.c_str()), shared.abbreviation_for(key.c_str())) &&
            same(frozen::main_abbrev_to_name(key.c_str()), shared.name_for(key.c_str())), "Shared maps differ from retained registration/lookup");
      check(same(frozen::main_name_to_abbrev(key.c_str()), actual::main_name_to_abbrev(key.c_str())) &&
            same(frozen::main_abbrev_to_name(key.c_str()), actual::main_abbrev_to_name(key.c_str())), "Actual GTK adapter differs from frozen baseline");
    }
    check(!shared.name_for(nullptr) && !shared.abbreviation_for(nullptr) &&
          !actual::main_name_to_abbrev(nullptr) && !actual::main_abbrev_to_name(nullptr), "Null queries preserve absence");
  };
  compare();
  const std::vector<std::pair<std::string,std::string>> operations = {
    {"Text","T"},{"Other","T"},{"Text","T2"},{"Third","(bad)"},
    {"Text","(bad)"},{"Third","bad)"},{"café α","文字"},{"Third",""},{"","t"},{"Text","t"}
  };
  for (const auto &item : operations) {
    frozen::main_add_abbreviation(item.first.c_str(),item.second.c_str());
    actual::main_add_abbreviation(item.first.c_str(),item.second.c_str());
    std::string name=item.first, alias=item.second;
    shared.add(name.c_str(),alias.c_str()); name.clear(); alias.clear(); compare();
  }
  check(std::string(shared.name_for("T"))=="Other" && std::string(shared.name_for("T2"))=="Text", "Reverse collisions and previous inverse aliases remain defined");
  check(shared.abbreviation_for("Third") && !*shared.abbreviation_for("Third"), "Empty alias differs from missing alias");
  const auto copied=shared;
  shared.clear(); frozen::main_clear_abbreviations(); actual::main_clear_abbreviations(); compare();
  check(std::string(copied.name_for("文字"))=="café α", "Owned registry copy survives source clearing");
  std::cout << "C++11 shared/actual/frozen abbreviation registration, collisions, Unicode and lifetime checks passed\n";
  return 0;
} catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; } }
