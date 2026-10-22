// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/worker.hpp"
#include "xiphos/core/engine_manager.hpp"
#include "module_alias_catalog.hpp"
#include <markupfiltmgr.h>
#include <rawtext.h>
#include <swmodule.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <cstring>
#include <set>
#include <stdexcept>
using namespace xiphos::core;
namespace oracle {
#include "module_abbreviations_legacy.inc"
std::set<std::string> installed;
int main_is_module(char *name) { return installed.count(name) != 0; }
void register_alias(const char *modname, const char *abbreviation) {
#include "legacy_alias_registration_actual.inc"
}
void load(const std::vector<detail::ModuleAliasEntry> &entries) {
  installed.clear(); main_clear_abbreviations();
  for (const auto &entry : entries) installed.insert(entry.module);
  for (const auto &entry : entries) register_alias(entry.module.c_str(), entry.abbreviation ? entry.abbreviation->c_str() : nullptr);
}
std::optional<std::string> resolve(const std::string &requested) {
  if (installed.count(requested)) return requested;
  if (const auto *name = main_abbrev_to_name(requested.c_str())) return std::string(name);
  return {};
}
}
void check(bool ok, const char *message) { if (!ok) throw std::runtime_error(message); }
template<class F> void rejects(F call) { bool failed=false; try { call(); } catch (const std::exception &) { failed=true; } check(failed,"Expected bounded resolution rejection"); }
void create(const std::filesystem::path &root, const std::vector<detail::ModuleAliasEntry> &entries) {
  std::filesystem::create_directories(root/"mods.d");
  std::filesystem::create_directories(root/"data");
  check(sword::RawText::createModule((root/"data").string().c_str()) == 0, "Create original alias fixture");
  { sword::RawText writer((root/"data").string().c_str()); writer.setKey("John1:1"); writer.setEntry("Original alias fixture prose."); }
  for (const auto &entry : entries) {
    std::ofstream config(root/"mods.d"/(entry.module+".conf"));
    config << "[" << entry.module << "]\nDataPath=./data/\nModDrv=RawText\nSourceType=Plain\nEncoding=UTF-8\nLang=en\nDescription=Original alias fixture\n";
    if (entry.abbreviation) config << "Abbreviation=" << *entry.abbreviation << "\n";
    check(bool(config), "Write alias metadata");
  }
}
int main(int argc,char **argv) { try {
  check(argc==2,"Disposable alias fixture root required");
  const auto root=std::filesystem::u8path(argv[1]);
  std::filesystem::remove_all(root); // Dedicated generated fixture only.
  const std::vector<detail::ModuleAliasEntry> entries = {
    {"AFirst","Shared"},{"BSecond","Shared"},{"Exact","AFirst"},{"Blocked","(alias)"},
    {"Lower","shared"},{"Empty",""},{"Unicode","文字 café"},{"NoAlias",{}}
  };
  oracle::load(entries); detail::ModuleAliasCatalog catalog(entries);
  const std::vector<std::string> probes = {"AFirst","BSecond","Exact","Shared","shared","SHARED","(alias)","文字 café","NoAlias","Missing"};
  for (const auto &token : probes) {
    const auto value=catalog.resolve(token);
    check(value.requested==token && value.module==oracle::resolve(token) && value.via_abbreviation==(value.module && *value.module!=token),
          "Bounded catalog matches actual retained registration block and frozen map rules");
  }
  check(catalog.resolve("Shared").module=="BSecond" && catalog.resolve("AFirst").module=="AFirst", "SDK order wins alias collisions; every installed ID stays protected");
  for (const auto &bad : {std::string(),std::string("bad\0tail",8),std::string(1,char(0xff)),std::string(4097,'x')}) {
    rejects([&] { catalog.resolve(bad); });
    rejects([&] { detail::ModuleAliasCatalog invalid({{bad,"Alias"}}); });
    if (!bad.empty()) rejects([&] { detail::ModuleAliasCatalog invalid({{"Valid",bad}}); });
  }
  rejects([&] { detail::ModuleAliasCatalog invalid({{"Duplicate",{}},{"Duplicate","Alias"}}); });
  rejects([&] { detail::ModuleAliasCatalog invalid(std::vector<detail::ModuleAliasEntry>(detail::maximum_alias_catalog_modules+1)); });
  {
    std::vector<detail::ModuleAliasEntry> bounded;
    for (unsigned i=0;i<4096;++i) { auto name=std::to_string(i); name.resize(4096,'x'); bounded.push_back({std::move(name),{}}); }
    detail::ModuleAliasCatalog exact(bounded);
    check(exact.resolve(bounded.back().module).module==bounded.back().module, "Exact aggregate byte budget remains valid");
    bounded.front().abbreviation="x";
    rejects([&] { detail::ModuleAliasCatalog overflow(bounded); });
  }
  create(root,entries);
  std::vector<detail::ModuleAliasEntry> sdkEntries;
  { EngineManager manager(root.string().c_str(),true,new sword::MarkupFilterMgr(sword::FMT_XHTML),false,false);
    for (const auto &item : manager.Modules) {
      const auto *alias=item.second->getConfigEntry("Abbreviation");
      sdkEntries.push_back({item.second->getName(),alias?std::optional<std::string>(alias):std::nullopt});
    }
  }
  oracle::load(sdkEntries);
  ModuleNameResolution held;
  { Reader reader(root.u8string());
    for (const auto &token:probes) check(reader.resolve_module_name(token).module==oracle::resolve(token), "Reader uses actual SDK module order/config alias metadata");
    held=reader.resolve_module_name("Shared");
    check(reader.read("AFirst","John1:1").html.find("Original alias fixture prose")!=std::string::npos,"Alias capture leaves ordinary reading intact");
  }
  std::future<ModuleNameResolution> draining;
  { StudyWorker worker(root.u8string()); const auto tab=worker.create_tab().get();
    std::string token="Shared"; auto pending=worker.resolve_module_name(tab,token); token="Changed";
    check(pending.get().module==held.module && worker.history(tab).get().entries.empty() && !worker.search_generation(tab).get().id,
          "FIFO resolution copies inputs and applies no history/search state");
    worker.close_tab(tab).get(); rejects([&] { (void)worker.resolve_module_name(tab,"Shared").get(); });
    const auto live=worker.create_tab().get(); draining=worker.resolve_module_name(live,"文字 café");
  }
  held=draining.get();
  check(held.module=="Unicode" && held.requested=="文字 café", "Copied resolution outlives Reader/worker/SDK");
  const auto badRoot=root/"bad-metadata"; create(badRoot,{{"Bad",std::string(4097,'x')}});
  { Reader reader(badRoot.u8string());
    rejects([&] { reader.resolve_module_name("UnknownAlias"); });
    check(reader.resolve_module_name("Bad").module=="Bad", "Exact IDs bypass unrelated invalid alias metadata");
    check(reader.read("Bad","John1:1").html.find("Original alias fixture prose")!=std::string::npos,"Rejected lazy alias metadata never disables ordinary reading");
    rejects([&] { reader.resolve_module_name("UnknownAlias"); });
  }
  std::cout << "Bounded owned alias catalog, retained registration, SDK order, exact-ID protection and FIFO isolation passed\n";
  return 0;
} catch(const std::exception &error) { std::cerr << error.what() << '\n'; return 1; } }
