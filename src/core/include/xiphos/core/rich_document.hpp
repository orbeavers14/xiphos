// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "editable_document.hpp"
#include <vector>
namespace xiphos::core {
enum class RichBlock { normal=0,h1,h2,h3,h4,h5,h6,address,pre,bullet,number,roman,alpha };
struct RichStyle {
  unsigned flags=0; // bold=1, italic=2, underline=4, strike=8
  RichBlock block=RichBlock::normal;
  std::string foreground,background,link;
};
struct RichRun {std::string text;RichStyle style;};
struct RichDocument {std::vector<RichRun> runs;};
enum class RichAction {bold=0,italic,underline,strike,block,foreground,background,link,clear};
struct RichCommand {RichAction action;int value=-1;std::string text;};
RichDocument normalize_rich_document(const RichDocument &document);
void validate_rich_document(const RichDocument &document);
RichDocument parse_rich_document(const std::string &html);
std::string rich_document_text(const RichDocument &document);
std::string serialize_rich_document(const RichDocument &document);
// UTF-8 byte coordinates, checked at character boundaries. Empty selection uses
// the current line. Block/clear commands apply to complete intersected lines.
RichDocument format_rich_document(const RichDocument &document,std::size_t start,std::size_t length,const RichCommand &command);
}
