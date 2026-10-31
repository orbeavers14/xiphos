// SPDX-License-Identifier: GPL-2.0-or-later
#include "xml_document.hpp"
#include <libxml/encoding.h>
#include <libxml/chvalid.h>
#include <algorithm>
#include <mutex>
#include <stdexcept>
namespace xiphos::core::detail {
using Parser = std::unique_ptr<xmlParserCtxt, decltype(&xmlFreeParserCtxt)>;
// Stop at the declaration before parsing/loading any DTD. Do not change the
// process-wide entity loader, error handler or substitution defaults.
void reject_doctype(void *data, const xmlChar *, const xmlChar *, const xmlChar *) {
  auto *context = static_cast<xmlParserCtxt *>(data);
  *static_cast<bool *>(context->_private) = true;
  xmlStopParser(context);
}
XmlDocument parse_xml_document(const std::string &source, const char *expected_root) {
  if (source.empty() || source.size() > maximum_xml_document_bytes)
    throw std::invalid_argument("Xiphos profile size is invalid");
  static std::once_flag initialized;
  std::call_once(initialized, [] { xmlInitParser(); });
  // Older libxml2 accepts a NUL after a complete document as end-of-input,
  // concealing trailing bytes. Check code units while retaining UTF-16/UCS-4.
  const auto encoding = xmlDetectCharEncoding(reinterpret_cast<const unsigned char *>(source.data()),
      static_cast<int>(source.size() < 4 ? source.size() : 4));
  std::size_t unit = 1;
  if (encoding == XML_CHAR_ENCODING_UTF16LE || encoding == XML_CHAR_ENCODING_UTF16BE ||
      encoding == XML_CHAR_ENCODING_UCS2) unit = 2;
  else if (encoding == XML_CHAR_ENCODING_UCS4LE || encoding == XML_CHAR_ENCODING_UCS4BE ||
      encoding == XML_CHAR_ENCODING_UCS4_2143 || encoding == XML_CHAR_ENCODING_UCS4_3412) unit = 4;
  if (source.size() % unit) throw std::invalid_argument("Xiphos profile encoding is incomplete");
  for (std::size_t offset = 0; offset < source.size(); offset += unit) {
    bool zero = true;
    for (std::size_t byte = 0; byte < unit; ++byte) zero = zero && source[offset + byte] == '\0';
    if (zero) throw std::invalid_argument("Xiphos profile contains an invalid NUL character");
  }
  Parser parser(xmlNewParserCtxt(), xmlFreeParserCtxt);
  if (!parser) throw std::bad_alloc();
  bool has_doctype = false;
  parser->_private = &has_doctype;
  parser->sax->internalSubset = reject_doctype;
  XmlDocument document(xmlCtxtReadMemory(parser.get(), source.data(), static_cast<int>(source.size()),
      nullptr, nullptr, XML_PARSE_NONET | XML_PARSE_NOERROR | XML_PARSE_NOWARNING), xmlFreeDoc);
  if (has_doctype) throw std::invalid_argument("Xiphos profiles with DTD declarations are unsupported");
  if (!document || !parser->wellFormed)
    throw std::invalid_argument("Xiphos profile XML is malformed");
  auto *root = xmlDocGetRootElement(document.get());
  if (!root || xmlStrcmp(root->name, BAD_CAST expected_root))
    throw std::invalid_argument("Xiphos profile root is invalid");
  return document;
}
void validate_xml_text(const std::string &text,std::size_t maximum) {
  if(text.size()>maximum||text.find('\0')!=std::string::npos||!xmlCheckUTF8(BAD_CAST text.c_str()))
    throw std::invalid_argument("Invalid XML text");
  for(std::size_t at=0;at<text.size();) {
    int length=static_cast<int>(std::min<std::size_t>(4,text.size()-at));
    const int codepoint=xmlGetUTF8Char(BAD_CAST text.data()+at,&length);
    if(codepoint<0||length<=0||!xmlIsCharQ(codepoint))throw std::invalid_argument("Invalid XML text character");
    at+=static_cast<std::size_t>(length);
  }
}
}
