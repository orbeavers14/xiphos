// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "xiphos/core/verse_image.hpp"
namespace sword {class SWModule;}
namespace xiphos::core::detail {
enum class VerseImageSourceKind {inline_source,study_command};
VerseImageResource capture_verse_image_resource(sword::SWModule &module,const std::string &source,std::size_t maximum_bytes,VerseImageSourceKind kind=VerseImageSourceKind::inline_source);
}
