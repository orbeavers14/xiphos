// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/profile.hpp"
#include <libxml/parser.h>
#include <libxml/encoding.h>
#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <stdexcept>
namespace xiphos::core {
namespace {
constexpr std::size_t maximum_profile_bytes = 16 * 1024 * 1024;
using Document = std::unique_ptr<xmlDoc, decltype(&xmlFreeDoc)>;
using Parser = std::unique_ptr<xmlParserCtxt, decltype(&xmlFreeParserCtxt)>;
// Stop at the declaration before parsing/loading any DTD. Do not change the
// process-wide entity loader, error handler or substitution defaults.
void reject_doctype(void *data, const xmlChar *, const xmlChar *, const xmlChar *) {
  auto *context = static_cast<xmlParserCtxt *>(data);
  *static_cast<bool *>(context->_private) = true;
  xmlStopParser(context);
}
Document parse_document(const std::string &source) {
  if (source.empty() || source.size() > maximum_profile_bytes)
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
  Document document(xmlCtxtReadMemory(parser.get(), source.data(), static_cast<int>(source.size()),
      nullptr, nullptr, XML_PARSE_NONET | XML_PARSE_NOERROR | XML_PARSE_NOWARNING), xmlFreeDoc);
  if (has_doctype) throw std::invalid_argument("Xiphos profiles with DTD declarations are unsupported");
  if (!document || !parser->wellFormed)
    throw std::invalid_argument("Xiphos profile XML is malformed");
  auto *root = xmlDocGetRootElement(document.get());
  if (!root || xmlStrcmp(root->name, BAD_CAST "Xiphos"))
    throw std::invalid_argument("Xiphos profile root is invalid");
  return document;
}
// Match xml_find_prop traversal, including later duplicate sections when
// earlier ones do not contain this key. Empty/child-only values remain null.
std::optional<std::string> value(xmlDoc *document, const char *section, const char *key) {
  auto *root = xmlDocGetRootElement(document);
  for (auto *group = root->children; group; group = group->next) {
    if (xmlStrcmp(group->name, BAD_CAST section)) continue;
    for (auto *item = group->children; item; item = item->next) {
      if (xmlStrcmp(item->name, BAD_CAST key)) continue;
      auto *text = xmlNodeListGetString(document, item->children, 1);
      if (!text) return std::nullopt;
      std::unique_ptr<xmlChar, decltype(xmlFree)> owned(text, xmlFree);
      return std::string(reinterpret_cast<const char *>(text));
    }
  }
  return std::nullopt;
}
}
StudyProfile parse_study_profile(const std::string &source) {
  auto document = parse_document(source);
  StudyProfile result;
  auto configured = value(document.get(), "lexicons", "greek");
  if (configured) result.lexicons.greek = std::move(configured);
  configured = value(document.get(), "lexicons", "hebrew");
  if (configured) result.lexicons.hebrew = std::move(configured);
  configured = value(document.get(), "lexicons", "greek_morph_nt");
  if (configured && !configured->empty()) result.lexicons.greek_morph_nt = std::move(configured);
  configured = value(document.get(), "lexicons", "greek_morph_ot");
  if (configured && !configured->empty()) result.lexicons.greek_morph_ot = std::move(configured);
  configured = value(document.get(), "lexicons", "hebrew_morph");
  if (configured && !configured->empty()) result.lexicons.hebrew_morph = std::move(configured);
  result.saved_locale = value(document.get(), "locale", "special");
  return result;
}
StudyProfile read_study_profile(const std::string &path) {
  if (path.empty() || path.find('\0') != std::string::npos)
    throw std::invalid_argument("Xiphos profile path is invalid");
  const auto file = std::filesystem::u8path(path);
  if (!std::filesystem::is_regular_file(file))
    throw std::invalid_argument("Xiphos profile must be a regular file");
  std::ifstream input(file, std::ios::binary);
  if (!input) throw std::invalid_argument("Xiphos profile cannot be read");
  std::string source;
  char buffer[8192];
  while (input.read(buffer, sizeof buffer) || input.gcount()) {
    const auto length = static_cast<std::size_t>(input.gcount());
    if (source.size() + length > maximum_profile_bytes)
      throw std::invalid_argument("Xiphos profile size is invalid");
    source.append(buffer, length);
  }
  if (input.bad()) throw std::runtime_error("Xiphos profile read failed");
  return parse_study_profile(source);
}
}
