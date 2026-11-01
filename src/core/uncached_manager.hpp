// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "xiphos/core/engine_manager.hpp"
#include "xiphos/core/editable_document.hpp"
#include "bounded_file.hpp"
#include <swconfig.h>
#include <filesystem>
#include <algorithm>
#include <memory>
#include <stdexcept>
namespace xiphos::core::detail {
// SWMgr::loadConfigDir writes modules-conf.cache even during inspection. Native
// discovery must read current .conf files without caching or auto-install writes.
// Keep SDK SWConfig parsing/augmentation; replace only its directory cache IO.
class UncachedEngineManager final:public EngineManager {
 bool isolated_;
#if XIPHOS_SWORD_CONFIG_CACHE_ARGUMENT
 void loadConfigDir(const char *directory,bool =false) override {
#else
 void loadConfigDir(const char *directory) override {
#endif
  std::vector<std::filesystem::path> paths;std::size_t bytes=0;
  for(const auto &entry:std::filesystem::directory_iterator(std::filesystem::u8path(directory)))if(entry.is_regular_file()&&entry.path().extension()==".conf"){
   if(paths.size()>=4096)throw std::invalid_argument("Too many module configuration files");paths.push_back(entry.path());
  }
  std::sort(paths.begin(),paths.end());auto staged=std::make_unique<sword::SWConfig>();
  for(const auto &path:paths){auto text=read_bounded_file(path,std::min<std::size_t>(4*1024*1024,16*1024*1024-bytes),"Module configuration");bytes+=text.size();if(!editable_document_text(text))throw std::invalid_argument("Module configuration requires bounded UTF-8 text");
   sword::SWConfig loaded(path.u8string().c_str());staged->augment(loaded);
  }
  const auto globals=staged->getSections().find("Globals");if(globals!=staged->getSections().end()&&globals->second.find("AutoInstall")!=globals->second.end())throw std::invalid_argument("Automatic installation directives are unsupported in native discovery");
  if(isolated_)for(const auto &section:staged->getSections())if(section.second.find("ModDrv")!=section.second.end()){
   const auto entry=section.second.find("DataPath");if(entry==section.second.end()||entry->second.size()==0)throw std::invalid_argument("Installation module has no data path");
   const std::string value=entry->second.c_str();auto path=std::filesystem::u8path(value);
   if(path.is_absolute()||value.find('\\')!=value.npos||value.find(':')!=value.npos)throw std::invalid_argument("Installation data must stay inside its source root");
   for(const auto &part:path)if(part=="..")throw std::invalid_argument("Installation data escapes its source root");
  }
  if(config)config->augment(*staged);else config=myconfig=staged.release();
 }
 void InstallScan(const char *) override {throw std::invalid_argument("Module installation must be explicitly requested");}
public:
 UncachedEngineManager(const char *root,bool autoload=true,sword::SWFilterMgr *filters=nullptr,bool multi=false,bool augment=false,bool isolated=false):EngineManager(root,false,filters,multi,augment),isolated_(isolated){if(autoload)load();}
};
}
