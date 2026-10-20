// SPDX-License-Identifier: GPL-2.0-or-later
#include "verse_image_source.hpp"
#include "module_resource.hpp"
#include "search_source.hpp"
#include "xiphos/core/link.hpp"
#include <swmodule.h>
#include <glib.h>
#include <optional>
#include <stdexcept>
namespace xiphos::core::detail {
VerseImageResource capture_verse_image_resource(sword::SWModule &module,const std::string &source,std::size_t maximum_bytes){
 if(!supports_verse_search(module))throw std::invalid_argument("Image source requires a supported Bible or commentary module");
 if(!maximum_bytes||maximum_bytes>maximum_verse_image_bytes||source.empty()||source.size()>32768||
    source.find('\0')!=std::string::npos||!g_utf8_validate(source.data(),source.size(),nullptr))
  throw std::invalid_argument("Invalid bounded verse image source");
 const auto *value=module.getConfigEntry("AbsoluteDataPath");
 const std::optional<std::string> directory=value?std::make_optional(std::string(value)):std::nullopt;
 std::string uri=source;
 // OSIS emits a raw native SDK directory followed by the source URI. Decode
 // only that URI suffix; decoding the native prefix can change literal % paths.
 if(module.getMarkup()==sword::FMT_OSIS&&directory){
  const auto prefix="file:"+*directory;
  if(uri.compare(0,prefix.size(),prefix)==0)uri.erase(0,prefix.size());
 }
 const auto command=book_inline_image_link(uri);
 const auto path=module_resource_path(directory,normalize_module_image_filename(command.filename),false);
 return {module.getName(),source,command.media_type,read_module_resource_bytes(path,maximum_bytes,"Verse image")};
}
}
