// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/profile.hpp"
#include "preference_file.hpp"
#include "search_validation.hpp"
#include <libxml/parser.h>
#include <libxml/encoding.h>
#include <libxml/chvalid.h>
#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <array>
#include <algorithm>
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
namespace {
void parallel_text(const std::string &text, std::size_t maximum) {
  if (text.size() > maximum || text.find('\0') != std::string::npos || !xmlCheckUTF8(BAD_CAST text.c_str()))
    throw std::invalid_argument("Invalid saved parallel text");
}
std::vector<std::string> parallel_split(const std::string &text, std::size_t maximum_count, std::size_t maximum_token) {
  parallel_text(text, maximum_profile_bytes);
  std::vector<std::string> result;
  if (text.empty()) return result;
  std::size_t start = 0;
  for (;;) {
    const auto end = text.find(',', start);
    const auto size = (end == std::string::npos ? text.size() : end) - start;
    if (result.size() == maximum_count || size > maximum_token)
      throw std::invalid_argument("Saved parallel list exceeds storage bounds");
    result.push_back(text.substr(start, size));
    if (end == std::string::npos) break;
    start = end + 1;
  }
  return result;
}
}
std::vector<std::string> saved_parallel_module_names(const std::string &text) {
  return parallel_split(text, maximum_saved_parallel_modules, 4096);
}
std::string parallel_set_key(const std::string &display_name) {
  parallel_text(display_name, maximum_saved_parallel_key_bytes);
  auto key = display_name;std::replace(key.begin(), key.end(), ' ', '_');return key;
}
std::string parallel_set_display_name(const std::string &key) {
  parallel_text(key, maximum_saved_parallel_key_bytes);
  auto display = key;std::replace(display.begin(), display.end(), '_', ' ');return display;
}
SavedParallelProfile parse_saved_parallel_profile(const std::string &source) {
  auto document = parse_document(source);
  SavedParallelProfile result;
  auto account = [&](const std::string &text) {
    if (text.size() > maximum_saved_parallel_snapshot_bytes - result.snapshot_bytes)
      throw std::invalid_argument("Saved parallel snapshot exceeds byte bounds");
    result.snapshot_bytes += text.size();
  };
  auto scalar = [&](const std::string &key) {
    auto text = value(document.get(), "modules", key.c_str());if (text) account(*text);return text;
  };
  auto modules = [&](const std::optional<std::string> &text) -> std::optional<std::vector<std::string>> {
    if (!text || text->empty()) return std::nullopt;
    auto tokens = saved_parallel_module_names(*text);for (const auto &token : tokens) account(token);return tokens;
  };
  auto definition = [&](const std::string &key) -> std::optional<std::string> {
    parallel_text(key, maximum_saved_parallel_key_bytes);
    return key.empty() ? std::nullopt : value(document.get(), "modules", ("parallel_set_" + key).c_str());
  };
  result.names_text = scalar("parallel_set_names");result.current_key = scalar("parallel_set_current");
  result.active_modules_text = scalar("parallels");
  for (std::size_t index = 0; index < result.legacy_slots.size(); ++index)
    result.legacy_slots[index] = scalar("int" + std::to_string(index + 1));
  if (result.names_text) for (auto &key : parallel_split(*result.names_text, maximum_saved_parallel_sets, maximum_saved_parallel_key_bytes)) {
    account(key);auto display = parallel_set_display_name(key);account(display);
    auto text = definition(key);if (text) account(*text);auto selected = modules(text);
    result.sets.push_back({std::move(key), std::move(display), std::move(text), std::move(selected)});
  }
  auto active = result.active_modules_text;
  if (active && !active->empty()) result.startup_source = ParallelStartupSource::configured;
  else {
    std::string migrated;
    for (const auto &slot : result.legacy_slots) if (slot && !slot->empty()) {
      if (!migrated.empty()) migrated += ',';migrated += *slot;
      if (migrated.size() > maximum_profile_bytes) throw std::invalid_argument("Legacy parallel list exceeds byte bounds");
    }
    if (!migrated.empty()) { active = std::move(migrated);result.startup_source = ParallelStartupSource::legacy_slots; }
  }
  if (result.current_key && !result.current_key->empty()) {
    auto text = definition(*result.current_key);
    if (text && !text->empty()) { active = std::move(text);result.startup_source = ParallelStartupSource::named_set; }
  }
  if (active && !active->empty()) { result.startup_modules_text = std::move(active);account(*result.startup_modules_text); }
  result.startup_modules = modules(result.startup_modules_text);
  return result;
}
SavedParallelProfile read_saved_parallel_profile(const std::string &path) {
  return parse_saved_parallel_profile(read_profile_source(path));
}
namespace {
void parallel_xml_text(const std::string &text, std::size_t maximum) {
  parallel_text(text, maximum);
  for (std::size_t at = 0; at < text.size();) {
    int length = static_cast<int>(std::min<std::size_t>(4, text.size() - at));
    const int codepoint = xmlGetUTF8Char(BAD_CAST text.data() + at, &length);
    if (codepoint < 0 || length <= 0 || !xmlIsCharQ(codepoint))
      throw std::invalid_argument("Invalid XML parallel profile text");
    at += static_cast<std::size_t>(length);
  }
}
void parallel_key(const std::string &key) {
  parallel_xml_text(key, maximum_saved_parallel_key_bytes);
  const auto node = "parallel_set_" + key;
  if (key.empty() || key == "names" || key == "current" || key.find(',') != std::string::npos ||
      xmlValidateNCName(BAD_CAST node.c_str(), 0) != 0)
    throw std::invalid_argument("Parallel set key cannot round trip as a legacy XML field");
}
std::string parallel_join(const std::vector<std::string> &items) {
  if (items.size() > maximum_saved_parallel_modules)
    throw std::invalid_argument("Too many stored parallel modules");
  std::string result;
  for (std::size_t index = 0; index < items.size(); ++index) {
    parallel_xml_text(items[index], 4096);
    if (items[index].find(',') != std::string::npos)
      throw std::invalid_argument("Parallel module token contains an unescaped comma");
    const auto extra = items[index].size() + (index ? 1 : 0);
    if (extra > maximum_profile_bytes - result.size())
      throw std::invalid_argument("Stored parallel modules exceed byte bounds");
    if (index) result += ',';result += items[index];
  }
  return result;
}
void validate_parallel_edit(const ParallelProfileEdit &edit) {
  parallel_key(edit.key);
  switch (edit.action) {
    case ParallelProfileAction::save:
      if (!edit.replacement_key.empty()) throw std::invalid_argument("Save does not rename a parallel set");
      (void)parallel_join(edit.modules);break;
    case ParallelProfileAction::rename:
      parallel_key(edit.replacement_key);
      if (!edit.modules.empty() || edit.make_current) throw std::invalid_argument("Rename has unexpected parallel fields");
      break;
    case ParallelProfileAction::remove:
    case ParallelProfileAction::activate:
      if (!edit.replacement_key.empty() || !edit.modules.empty() || edit.make_current)
        throw std::invalid_argument("Unexpected parallel edit fields");
      break;
    default: throw std::invalid_argument("Unknown parallel profile edit");
  }
}
xmlNode *parallel_first_section(xmlDoc *document) {
  for (auto *node = xmlDocGetRootElement(document)->children; node; node = node->next)
    if (!xmlStrcmp(node->name, BAD_CAST "modules")) return node;
  return nullptr;
}
void set_parallel_value(xmlDoc *document, const std::string &key, const std::string &text) {
  auto *target = find_value_node(document, "modules", key.c_str());
  if (!target) {
    auto *section = parallel_first_section(document);
    if (!section) section = xmlNewChild(xmlDocGetRootElement(document), nullptr, BAD_CAST "modules", nullptr);
    if (!section) throw std::bad_alloc();
    target = xmlNewChild(section, nullptr, BAD_CAST key.c_str(), nullptr);
    if (!target) throw std::bad_alloc();
  }
  auto *content = xmlNewText(BAD_CAST text.c_str());
  if (!content) throw std::bad_alloc();
  xmlNodeSetContent(target, nullptr);
  if (!xmlAddChild(target, content)) { xmlFreeNode(content);throw std::bad_alloc(); }
}
bool parallel_optional_text(const std::optional<std::string> &actual, const std::string &expected) {
  return expected.empty() ? !actual || actual->empty() : actual && *actual == expected;
}
}
std::string serialize_parallel_profile_edit(const std::string &source, const ParallelProfileEdit &edit) {
  validate_parallel_edit(edit);
  const auto before = parse_saved_parallel_profile(source);
  auto document = parse_document(source);
  auto listed = [&](const std::string &key) {
    return std::any_of(before.sets.begin(), before.sets.end(), [&](const auto &set) { return set.key == key; });
  };
  auto definition = [&](const std::string &key) { return value(document.get(), "modules", ("parallel_set_" + key).c_str()); };
  std::optional<std::string> expected_names, expected_current, expected_modules, renamed_definition;
  bool check_renamed_definition = false;
  const auto node_key = "parallel_set_" + edit.key;
  switch (edit.action) {
    case ParallelProfileAction::save: {
      const auto text = parallel_join(edit.modules);
      set_parallel_value(document.get(), node_key, text);
      if (!listed(edit.key)) {
        expected_names = before.names_text && !before.names_text->empty() ? *before.names_text + "," + edit.key : edit.key;
        set_parallel_value(document.get(), "parallel_set_names", *expected_names);
      }
      if (edit.make_current) {
        expected_current = edit.key;set_parallel_value(document.get(), "parallel_set_current", edit.key);
        if (!text.empty()) { expected_modules = text;set_parallel_value(document.get(), "parallels", text); }
      }
      break;
    }
    case ParallelProfileAction::rename: {
      if (!listed(edit.key)) throw std::invalid_argument("Cannot rename an unlisted parallel set");
      if (edit.key == edit.replacement_key) return source;
      const auto replacement = "parallel_set_" + edit.replacement_key;
      renamed_definition = definition(edit.key);check_renamed_definition = true;
      if (listed(edit.replacement_key) || find_value_node(document.get(), "modules", replacement.c_str()))
        throw std::invalid_argument("Parallel rename would overwrite an existing key");
      if (auto *node = find_value_node(document.get(), "modules", node_key.c_str())) {
        xmlNodeSetName(node, BAD_CAST replacement.c_str());
        if (!node->name || xmlStrcmp(node->name, BAD_CAST replacement.c_str())) throw std::bad_alloc();
      }
      std::string names;
      for (std::size_t index = 0; index < before.sets.size(); ++index) {
        if (index) names += ',';names += before.sets[index].key == edit.key ? edit.replacement_key : before.sets[index].key;
      }
      expected_names = std::move(names);set_parallel_value(document.get(), "parallel_set_names", *expected_names);
      if (before.current_key == edit.key) {
        expected_current = edit.replacement_key;set_parallel_value(document.get(), "parallel_set_current", *expected_current);
      }
      break;
    }
    case ParallelProfileAction::remove: {
      if (!listed(edit.key)) return source;
      std::string names;
      for (const auto &set : before.sets) if (set.key != edit.key) {
        // Match the complete retained delete callback's empty-token fold.
        if (!names.empty()) names += ',';names += set.key;
      }
      expected_names = std::move(names);set_parallel_value(document.get(), "parallel_set_names", *expected_names);
      // Retained removal is scoped to the first modules section only.
      if (auto *section = parallel_first_section(document.get())) for (auto *node = section->children; node; node = node->next)
        if (!xmlStrcmp(node->name, BAD_CAST node_key.c_str())) { xmlUnlinkNode(node);xmlFreeNode(node);break; }
      if (before.current_key == edit.key) {
        expected_current = "";set_parallel_value(document.get(), "parallel_set_current", "");
      }
      break;
    }
    case ParallelProfileAction::activate: {
      if (!listed(edit.key)) throw std::invalid_argument("Cannot activate an unlisted parallel set");
      expected_current = edit.key;set_parallel_value(document.get(), "parallel_set_current", edit.key);
      const auto text = definition(edit.key);
      if (text && !text->empty()) { expected_modules = *text;set_parallel_value(document.get(), "parallels", *text); }
      break;
    }
  }
  xmlChar *serialized = nullptr;int length = 0;
  xmlDocDumpFormatMemoryEnc(document.get(), &serialized, &length, "UTF-8", 0);
  const std::unique_ptr<xmlChar, decltype(xmlFree)> owned(serialized, xmlFree);
  if (!serialized || length <= 0) throw std::runtime_error("Parallel profile serialization failed");
  std::string result(reinterpret_cast<const char *>(serialized), static_cast<std::size_t>(length));
  const auto verified = parse_saved_parallel_profile(result);
  if ((expected_names && !parallel_optional_text(verified.names_text, *expected_names)) ||
      (expected_current && !parallel_optional_text(verified.current_key, *expected_current)) ||
      (expected_modules && !parallel_optional_text(verified.active_modules_text, *expected_modules)))
    throw std::runtime_error("Parallel profile metadata failed verification");
  if (check_renamed_definition) {
    const auto found = std::find_if(verified.sets.begin(), verified.sets.end(), [&](const auto &set) { return set.key == edit.replacement_key; });
    if (found == verified.sets.end() || found->module_text != renamed_definition)
      throw std::runtime_error("Renamed parallel definition failed verification");
  }
  if (edit.action == ParallelProfileAction::save) {
    auto expected = parallel_join(edit.modules);
    const auto found = std::find_if(verified.sets.begin(), verified.sets.end(), [&](const auto &set) { return set.key == edit.key; });
    if (found == verified.sets.end() || !parallel_optional_text(found->module_text, expected))
      throw std::runtime_error("Saved parallel set failed verification");
  }
  return result;
}
SavedParallelProfile edit_parallel_profile(const std::string &path, const ParallelProfileEdit &edit) {
  validate_parallel_edit(edit);
  SavedParallelProfile prepared;
  detail::replace_preference_file(path, [&](const std::filesystem::path &file, const std::string &original, bool existed) {
    if (!existed && edit.action != ParallelProfileAction::save)
      throw std::invalid_argument("Cannot edit parallel sets in a missing profile");
    const auto serialized = serialize_parallel_profile_edit(existed ? original : "<Xiphos/>", edit);
    { std::ofstream output(file, std::ios::binary);output.exceptions(std::ios::badbit | std::ios::failbit);
      output.write(serialized.data(), static_cast<std::streamsize>(serialized.size()));output.close(); }
    if (detail::read_preference_file(file, maximum_profile_bytes) != serialized)
      throw std::runtime_error("Written parallel profile failed verification");
    prepared = read_saved_parallel_profile(file.u8string());
  }, maximum_profile_bytes);
  return prepared;
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
std::string serialize_saved_search_list(const std::string &source, const SavedSearchListEdit &edit) {
  const char *section_name = nullptr, *item_name = nullptr;
  switch (edit.kind) {
    case SavedSearchListKind::module_list: section_name = "modlists"; item_name = "modlist"; break;
    case SavedSearchListKind::range: section_name = "ranges"; item_name = "range"; break;
    default: throw std::invalid_argument("Unknown saved search list kind");
  }
  for (const auto *text : {&edit.label, &edit.value})
    if (text->size() > maximum_profile_bytes || text->find('\0') != std::string::npos || !xmlCheckUTF8(BAD_CAST text->c_str()))
      throw std::invalid_argument("Invalid saved search list text");
  auto document = parse_document(source);
  auto *root = xmlDocGetRootElement(document.get());
  xmlNode *section = nullptr, *target = nullptr;
  for (auto *node = root->children; node; node = node->next)
    if (!xmlStrcmp(node->name, BAD_CAST section_name)) { section = node; break; }
  if (!section) section = xmlNewChild(root, nullptr, BAD_CAST section_name, nullptr);
  if (!section) throw std::bad_alloc();
  for (auto *node = section->children; node; node = node->next) {
    if (xmlStrcmp(node->name, BAD_CAST item_name)) continue;
    const std::unique_ptr<xmlChar, decltype(xmlFree)> label(xmlGetProp(node, BAD_CAST "label"), xmlFree);
    if (label && !xmlStrcmp(label.get(), BAD_CAST edit.label.c_str())) { target = node; break; }
  }
  if (!target) {
    target = xmlNewChild(section, nullptr, BAD_CAST item_name, nullptr);
    if (!target || !xmlNewProp(target, BAD_CAST "label", BAD_CAST edit.label.c_str())) throw std::bad_alloc();
  }
  if (!xmlSetProp(target, BAD_CAST "list", BAD_CAST edit.value.c_str())) throw std::bad_alloc();
  xmlChar *serialized = nullptr;
  int length = 0;
  xmlDocDumpFormatMemoryEnc(document.get(), &serialized, &length, "UTF-8", 0);
  const std::unique_ptr<xmlChar, decltype(xmlFree)> owned(serialized, xmlFree);
  if (!serialized || length <= 0) throw std::runtime_error("Xiphos search-list serialization failed");
  std::string result(reinterpret_cast<const char *>(serialized), static_cast<std::size_t>(length));
  const auto verified = parse_saved_search_lists(result);
  const auto &items = edit.kind == SavedSearchListKind::module_list ? verified.module_lists : verified.ranges;
  if (saved_search_list_value(items, edit.label) != edit.value)
    throw std::runtime_error("Serialized search list failed verification");
  return result;
}
SavedSearchLists save_saved_search_list(const std::string &path, const SavedSearchListEdit &edit) {
  SavedSearchLists prepared;
  detail::replace_preference_file(path, [&](const std::filesystem::path &file, const std::string &original, bool existed) {
    const auto serialized = serialize_saved_search_list(existed ? original : "<Xiphos/>", edit);
    {
      std::ofstream output(file, std::ios::binary);
      output.exceptions(std::ios::badbit | std::ios::failbit);
      output.write(serialized.data(), static_cast<std::streamsize>(serialized.size()));
      output.close();
    }
    if (detail::read_preference_file(file, maximum_profile_bytes) != serialized)
      throw std::runtime_error("Written Xiphos search list failed verification");
    prepared = read_saved_search_lists(file.u8string());
  }, maximum_profile_bytes);
  return prepared;
}
std::string serialize_saved_search_list_removal(const std::string &source, SavedSearchListKind kind,
                                                const std::string &label) {
  const char *section_name = nullptr, *item_name = nullptr;
  switch (kind) {
    case SavedSearchListKind::module_list: section_name = "modlists"; item_name = "modlist"; break;
    case SavedSearchListKind::range: section_name = "ranges"; item_name = "range"; break;
    default: throw std::invalid_argument("Unknown saved search list kind");
  }
  if (label.size() > maximum_profile_bytes || label.find('\0') != std::string::npos || !xmlCheckUTF8(BAD_CAST label.c_str()))
    throw std::invalid_argument("Invalid saved search list label");
  for (std::size_t offset = 0; offset < label.size();) {
    int length = static_cast<int>(std::min<std::size_t>(4, label.size() - offset));
    const int codepoint = xmlGetUTF8Char(BAD_CAST label.data() + offset, &length);
    if (codepoint < 0 || length <= 0 || !xmlIsCharQ(codepoint))
      throw std::invalid_argument("Invalid XML saved search list label");
    offset += static_cast<std::size_t>(length);
  }
  // Validate original collection bounds as well as the resulting document.
  const auto before = parse_saved_search_lists(source);
  auto document = parse_document(source);
  auto *root = xmlDocGetRootElement(document.get());
  xmlNode *section = nullptr;
  for (auto *node = root->children; node; node = node->next)
    if (!xmlStrcmp(node->name, BAD_CAST section_name)) { section = node; break; }
  if (section) for (auto *node = section->children; node; node = node->next) {
    if (xmlStrcmp(node->name, BAD_CAST item_name)) continue;
    const std::unique_ptr<xmlChar, decltype(xmlFree)> candidate(xmlGetProp(node, BAD_CAST "label"), xmlFree);
    if (candidate && !xmlStrcmp(candidate.get(), BAD_CAST label.c_str())) {
      xmlUnlinkNode(node);
      xmlFreeNode(node);
      break;
    }
  }
  xmlChar *serialized = nullptr;
  int length = 0;
  xmlDocDumpFormatMemoryEnc(document.get(), &serialized, &length, "UTF-8", 0);
  const std::unique_ptr<xmlChar, decltype(xmlFree)> owned(serialized, xmlFree);
  if (!serialized || length <= 0) throw std::runtime_error("Xiphos search-list serialization failed");
  std::string result(reinterpret_cast<const char *>(serialized), static_cast<std::size_t>(length));
  const auto verified = parse_saved_search_lists(result);
  auto expected = kind == SavedSearchListKind::module_list ? before.module_lists : before.ranges;
  if (const auto index = saved_search_list_index(expected, label)) expected.erase(expected.begin() + *index);
  const auto &actual = kind == SavedSearchListKind::module_list ? verified.module_lists : verified.ranges;
  if (actual.size() != expected.size()) throw std::runtime_error("Removed search list failed verification");
  for (std::size_t i = 0; i < actual.size(); ++i)
    if (actual[i].label != expected[i].label || actual[i].value != expected[i].value)
      throw std::runtime_error("Removed search list failed verification");
  return result;
}
SavedSearchLists remove_saved_search_list(const std::string &path, SavedSearchListKind kind, const std::string &label) {
  SavedSearchLists prepared;
  detail::replace_preference_file(path, [&](const std::filesystem::path &file, const std::string &original, bool existed) {
    if (!existed) throw std::invalid_argument("Cannot remove a search list from a missing profile");
    const auto serialized = serialize_saved_search_list_removal(original, kind, label);
    {
      std::ofstream output(file, std::ios::binary);
      output.exceptions(std::ios::badbit | std::ios::failbit);
      output.write(serialized.data(), static_cast<std::streamsize>(serialized.size()));
      output.close();
    }
    if (detail::read_preference_file(file, maximum_profile_bytes) != serialized)
      throw std::runtime_error("Written Xiphos search list failed verification");
    prepared = read_saved_search_lists(file.u8string());
  }, maximum_profile_bytes);
  return prepared;
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
