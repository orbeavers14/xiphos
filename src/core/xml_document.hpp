// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <libxml/parser.h>
#include <cstddef>
#include <memory>
#include <string>
namespace xiphos::core::detail {
inline constexpr std::size_t maximum_xml_document_bytes=16*1024*1024;
using XmlDocument=std::unique_ptr<xmlDoc,decltype(&xmlFreeDoc)>;
// No DTD/entity loading or process-wide parser policy changes. Reject hidden
// NUL code units while retaining declared UTF-16/UCS-4 handling and root checks.
XmlDocument parse_xml_document(const std::string &source,const char *expected_root);
void validate_xml_text(const std::string &text,std::size_t maximum);
}
