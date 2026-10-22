// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/profile.hpp"
#include "preference_file.hpp"
#include "search_validation.hpp"
#include <libxml/parser.h>
#include <libxml/encoding.h>
#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <array>
#include <cerrno>
#include <cstdlib>
#include <limits>
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
xmlNode *find_value_node(xmlDoc *document, const char *section, const char *key) {
  auto *root = xmlDocGetRootElement(document);
  for (auto *group = root->children; group; group = group->next) {
    if (xmlStrcmp(group->name, BAD_CAST section)) continue;
    for (auto *item = group->children; item; item = item->next) {
      if (xmlStrcmp(item->name, BAD_CAST key)) continue;
      return item;
    }
  }
  return nullptr;
}
std::optional<std::string> value(xmlDoc *document, const char *section, const char *key) {
  auto *item = find_value_node(document, section, key);
  if (!item) return std::nullopt;
  auto *text = xmlNodeListGetString(document, item->children, 1);
  if (!text) return std::nullopt;
  std::unique_ptr<xmlChar, decltype(xmlFree)> owned(text, xmlFree);
  return std::string(reinterpret_cast<const char *>(text));
}
std::string read_profile_source(const std::string &path) {
  if (path.empty() || path.find('\0') != std::string::npos)
    throw std::invalid_argument("Xiphos profile path is invalid");
  const auto file = std::filesystem::u8path(path);
  if (!std::filesystem::is_regular_file(file))
    throw std::invalid_argument("Xiphos profile must be a regular file");
  return detail::read_preference_file(file, maximum_profile_bytes);
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
  return parse_study_profile(read_profile_source(path));
}
SavedSearchLists parse_saved_search_lists(const std::string &source) {
  auto document = parse_document(source);
  auto attribute = [](xmlNode *node, const char *name) -> std::optional<std::string> {
    const std::unique_ptr<xmlChar, decltype(xmlFree)> text(xmlGetProp(node, BAD_CAST name), xmlFree);
    if (!text) return std::nullopt;
    return std::string(reinterpret_cast<const char *>(text.get()));
  };
  auto lists = [&](const char *section, const char *item) {
    std::vector<SavedSearchList> result;
    for (auto *group = xmlDocGetRootElement(document.get())->children; group; group = group->next) {
      if (xmlStrcmp(group->name, BAD_CAST section)) continue;
      for (auto *node = group->children; node; node = node->next) {
        if (xmlStrcmp(node->name, BAD_CAST item)) continue;
        if (result.size() == maximum_saved_search_lists)
          throw std::invalid_argument("Too many saved search lists");
        result.push_back({attribute(node, "label"), attribute(node, "list")});
      }
      break; // First section, even if empty; unlike scalar profile key traversal.
    }
    return result;
  };
  return {lists("modlists", "modlist"), lists("ranges", "range")};
}
SavedSearchLists read_saved_search_lists(const std::string &path) {
  return parse_saved_search_lists(read_profile_source(path));
}
std::optional<std::string> saved_search_list_value(const std::vector<SavedSearchList> &items,
                                                 const std::optional<std::string> &label) {
  const auto index = saved_search_list_index(items, label);
  return index ? items[*index].value : std::nullopt;
}
std::optional<std::size_t> saved_search_list_index(const std::vector<SavedSearchList> &items,
                                                const std::optional<std::string> &label) {
  for (std::size_t index = 0; index < items.size(); ++index)
    if (items[index].label == label) return index;
  return std::nullopt;
}
std::vector<std::string> saved_search_module_names(const std::string &text) {
  // Reject hidden suffixes even when they occur in otherwise empty fields.
  if (text.size() > maximum_saved_search_text_bytes || text.find('\0') != std::string::npos)
    throw std::invalid_argument("Saved module list exceeds input bounds");
  std::vector<std::string> result;
  std::size_t start = 0;
  while (start < text.size()) {
    const auto end = text.find(',', start);
    const auto count = (end == std::string::npos ? text.size() : end) - start;
    if (count) {
      if (result.size() == maximum_search_batch_modules)
        throw std::invalid_argument("Saved module list exceeds search batch limit");
      if (count > 4096) throw std::invalid_argument("Saved module name is too long");
      auto module = text.substr(start, count);
      detail::validate_search_module_name(module);
      result.push_back(std::move(module));
    }
    if (end == std::string::npos) break;
    start = end + 1;
  }
  return result;
}
SavedBookSelection parse_saved_book_selection(const std::string &source) {
  auto document = parse_document(source);
  return {value(document.get(), "modules", "book"), value(document.get(), "keys", "book"),
          value(document.get(), "keys", "offset")};
}
SavedBookSelection read_saved_book_selection(const std::string &path) {
  return parse_saved_book_selection(read_profile_source(path));
}
StudyPreviewPreferences parse_study_preview_preferences(const std::string &source) {
  auto document = parse_document(source);
  StudyPreviewPreferences result;
  const auto configured = value(document.get(), "misc", "xrefsinverselist");
  if (configured) {
    // Match defined atoi prefix behavior without integer overflow. Do not
    // enforce a new true/false grammar or normalize unrelated XML values.
    errno = 0;
    const auto number = std::strtol(configured->c_str(), nullptr, 10);
    if (errno == ERANGE || number < std::numeric_limits<int>::min() || number > std::numeric_limits<int>::max())
      throw std::invalid_argument("Xiphos preview preference integer is out of range");
    result.cross_references_in_verse_list = number != 0;
  }
  return result;
}
StudyPreviewPreferences read_study_preview_preferences(const std::string &path) {
  return parse_study_preview_preferences(read_profile_source(path));
}
std::string serialize_lexicon_profile(const std::string &source, const LexiconPreferences &preferences) {
  auto document = parse_document(source);
  struct Setting { const char *key; std::optional<std::string> LexiconPreferences::*member; };
  const std::array<Setting, 5> settings{{{"greek", &LexiconPreferences::greek},
      {"hebrew", &LexiconPreferences::hebrew}, {"greek_morph_nt", &LexiconPreferences::greek_morph_nt},
      {"greek_morph_ot", &LexiconPreferences::greek_morph_ot}, {"hebrew_morph", &LexiconPreferences::hebrew_morph}}};
  auto *root = xmlDocGetRootElement(document.get());
  xmlNode *section = nullptr;
  for (auto *item = root->children; item; item = item->next)
    if (!xmlStrcmp(item->name, BAD_CAST "lexicons")) { section = item; break; }
  if (!section) section = xmlNewChild(root, nullptr, BAD_CAST "lexicons", nullptr);
  if (!section) throw std::bad_alloc();
  for (const auto &setting : settings) {
    const auto &configured = preferences.*setting.member;
    if (!configured || configured->empty() || configured->size() > maximum_profile_bytes || configured->find('\0') != std::string::npos ||
        !xmlCheckUTF8(BAD_CAST configured->c_str()))
      throw std::invalid_argument("Lexicon choice cannot round trip through the legacy XML profile");
    auto *item = find_value_node(document.get(), "lexicons", setting.key);
    if (!item) item = xmlNewChild(section, nullptr, BAD_CAST setting.key, nullptr);
    if (!item) throw std::bad_alloc();
    auto *text = xmlNewText(BAD_CAST configured->c_str());
    if (!text) throw std::bad_alloc();
    xmlNodeSetContent(item, nullptr);
    if (!xmlAddChild(item, text)) { xmlFreeNode(text); throw std::bad_alloc(); }
  }
  xmlChar *serialized = nullptr;
  int length = 0;
  xmlDocDumpFormatMemoryEnc(document.get(), &serialized, &length, "UTF-8", 0);
  std::unique_ptr<xmlChar, decltype(xmlFree)> owned(serialized, xmlFree);
  if (!serialized || length <= 0) throw std::runtime_error("Xiphos profile serialization failed");
  std::string result(reinterpret_cast<const char *>(serialized), static_cast<std::size_t>(length));
  const auto verified = parse_study_profile(result);
  for (const auto &setting : settings)
    if (verified.lexicons.*setting.member != preferences.*setting.member)
      throw std::runtime_error("Serialized lexicon profile failed verification");
  return result;
}
void save_lexicon_profile(const std::string &path, const LexiconPreferences &preferences) {
  detail::replace_preference_file(path, [&](const std::filesystem::path &file, const std::string &original, bool existed) {
    const auto serialized = serialize_lexicon_profile(existed ? original : "<Xiphos/>", preferences);
    {
      std::ofstream output(file, std::ios::binary);
      output.exceptions(std::ios::badbit | std::ios::failbit);
      output.write(serialized.data(), static_cast<std::streamsize>(serialized.size()));
      output.close();
    }
    if (detail::read_preference_file(file, maximum_profile_bytes) != serialized)
      throw std::runtime_error("Written Xiphos profile failed verification");
    (void)read_study_profile(file.u8string());
  }, maximum_profile_bytes);
}
}
