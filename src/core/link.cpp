// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/link.hpp"
#include <stdexcept>
#include <map>
namespace xiphos::core {
namespace {
int hex(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}
std::string decode(const std::string &url) {
  std::string value;
  for (std::size_t i = 0; i < url.size(); ++i) {
    char c = url[i];
    if (c == '+') c = ' ';
    else if (c == '%' && i + 2 < url.size() && hex(url[i+1]) >= 0 && hex(url[i+2]) >= 0) {
      c = static_cast<char>((hex(url[i+1]) << 4) | hex(url[i+2])); i += 2;
    }
    if (static_cast<unsigned char>(c) < 32 || c == 127)
      throw std::invalid_argument("Control characters in passage link");
    value += c;
  }
  return value;
}
}
PassageLink passage_link(const std::string &url, const std::string &current_module) {
  // Require the whole URI, unlike the legacy handler's substring dispatch.
  // External content must not accidentally become an internal study command.
  const auto decoded = decode(url);
  std::size_t start;
  if (decoded.compare(0, 8, "sword://") == 0) start = 8;
  else if (decoded.compare(0, 8, "bible://") == 0) start = 8;
  else throw std::invalid_argument("Unsupported passage link");
  const auto slash = decoded.find('/', start);
  if (slash == std::string::npos) throw std::invalid_argument("Passage link requires a reference path");
  PassageLink result{decoded.substr(start, slash - start), decoded.substr(slash + 1)};
  if (result.module.empty()) result.module = current_module;
  if (result.module.empty() || result.reference.empty() ||
      result.reference.find_first_of("#?!-;,/") != std::string::npos)
    throw std::invalid_argument("This slice supports unanchored single-reference links");
  return result;
}
namespace {
using CommandParameters = std::map<std::string, std::string>;
CommandParameters command_parameters(const std::string &url) {
  const auto query = url.find('?');
  const auto path = url.substr(0, query);
  // Only local engine commands and the native host's fixed internal origin.
  if (query == std::string::npos || url.find('#') != std::string::npos ||
      (path != "passagestudy.jsp" && path != "xiphos.url" &&
       path != "xiphos-preview://study/passagestudy.jsp" && path != "xiphos-preview://study/xiphos.url"))
    throw std::invalid_argument("Unsupported study-note link");
  std::map<std::string, std::string> parameters;
  auto position = query + 1;
  while (position < url.size()) {
    const auto end = url.find('&', position);
    const auto parameter = url.substr(position, end == std::string::npos ? end : end - position);
    const auto equal = parameter.find('=');
    if (equal == std::string::npos || equal == 0) throw std::invalid_argument("Malformed note parameter");
    for (std::size_t i = 0; i < parameter.size(); ++i) {
      if (parameter[i] == '%') {
        if (i + 2 >= parameter.size() || hex(parameter[i+1]) < 0 || hex(parameter[i+2]) < 0)
          throw std::invalid_argument("Malformed note escape");
        i += 2;
      }
    }
    const auto name = decode(parameter.substr(0, equal));
    if (name != "action" && name != "type" && name != "value" && name != "module" && name != "passage")
      throw std::invalid_argument("Unsupported note parameter");
    if (!parameters.emplace(name, decode(parameter.substr(equal + 1))).second)
      throw std::invalid_argument("Duplicate note parameter");
    if (end == std::string::npos) break;
    position = end + 1;
    if (position == url.size()) throw std::invalid_argument("Empty note parameter");
  }
  return parameters;
}
std::string parameter(const CommandParameters &parameters, const char *name) {
  const auto found = parameters.find(name);
  return found == parameters.end() ? std::string() : found->second;
}
StudyNoteLink note_fields(const CommandParameters &parameters, const std::string &current_module) {
  const auto get = [&parameters](const char *name) {
    const auto found = parameters.find(name);
    return found == parameters.end() ? std::string() : found->second;
  };
  StudyNoteLink result{get("module"), get("passage"), get("value"), get("type")};
  if (result.module.empty()) result.module = current_module;
  if (get("action") != "showNote" || (result.type != "n" && result.type != "x") ||
      result.module.empty() || result.identifier.empty())
    throw std::invalid_argument("Study-note link requires a single passage and identifier");
  return result;
}
StudyNoteLink note_target(const CommandParameters &parameters, const std::string &current_module,
                          const std::string &current_reference = {}) {
  auto result = note_fields(parameters, current_module);
  if (!current_reference.empty() && result.reference.size() < 5) result.reference = current_reference;
  if (result.reference.empty() || result.reference.find_first_of("#?!-;,/") != std::string::npos)
    throw std::invalid_argument("Study-note link requires a single passage and identifier");
  return result;
}
LexicalLink lexical_target(const CommandParameters &parameters, const std::string &current_module) {
  const auto action = parameter(parameters, "action");
  if (action != "showStrongs" && action != "showMorph")
    throw std::invalid_argument("Unsupported lexical action");
  LexicalLink result{action == "showStrongs" ? LexicalLinkKind::strongs : LexicalLinkKind::morphology,
                     parameter(parameters, "module"), parameter(parameters, "type"),
                     parameter(parameters, "value"), parameter(parameters, "passage")};
  if (result.module.empty()) result.module = current_module;
  if (result.value.empty() || (result.kind == LexicalLinkKind::morphology && result.type.empty()))
    throw std::invalid_argument("Lexical link requires a value and morphology type");
  return result;
}
ReferenceListLink reference_list_target(const CommandParameters &parameters, const std::string &current_module) {
  ReferenceListLink result{parameter(parameters, "module"), parameter(parameters, "value")};
  if (result.module.empty()) result.module = current_module;
  if (parameter(parameters, "action") != "showRef" || parameter(parameters, "type") != "scripRef" ||
      parameters.count("passage") || result.module.empty() || result.expression.empty() ||
      result.expression.find_first_of("#?!/") != std::string::npos)
    throw std::invalid_argument("Reference link requires a scripture expression");
  return result;
}
PassageLink reference_target(const CommandParameters &parameters, const std::string &current_module) {
  const auto list = reference_list_target(parameters, current_module);
  if (list.expression.find_first_of("-;,") != std::string::npos)
    throw std::invalid_argument("Reference link requires a single scripture reference");
  return {list.module, list.expression};
}
}
LexicalLink lexical_link(const std::string &url, const std::string &current_module) {
  return lexical_target(command_parameters(url), current_module);
}
StudyNoteLink study_note_link(const std::string &url, const std::string &current_module) {
  return note_target(command_parameters(url), current_module);
}
StudyNoteLink book_note_link(const std::string &url, const std::string &module, const std::string &path) {
  if (module.empty() || module.find('\0') != std::string::npos || path.size() > 4096 ||
      path.find('\0') != std::string::npos || (!path.empty() && path.front() != '/'))
    throw std::invalid_argument("Invalid book-note source context");
  const auto parameters = command_parameters(url);
  const auto result = note_fields(parameters, module);
  // Book titles can contain verse-parser delimiters. Compare the entire decoded
  // SDK path instead of reinterpreting it as a verse or using short-text fallback.
  if (!parameters.count("passage") || result.module != module || result.reference != path)
    throw std::invalid_argument("Book-note command does not match the captured source");
  return result;
}
StudyCommandLink study_link(const std::string &url, const std::string &current_module) {
  if (url.compare(0, 8, "sword://") == 0 || url.compare(0, 8, "bible://") == 0)
    return passage_link(url, current_module);
  const auto parameters = command_parameters(url);
  if (parameter(parameters, "action") == "showRef") return reference_target(parameters, current_module);
  return note_target(parameters, current_module);
}
namespace {
ContextualStudyCommandLink contextual_link(const std::string &url, const std::string &current_module,
                                           const std::string &current_reference) {
  if (url.compare(0, 8, "sword://") == 0 || url.compare(0, 8, "bible://") == 0)
    return passage_link(url, current_module);
  const auto parameters = command_parameters(url);
  if (parameter(parameters, "action") == "showRef") return reference_list_target(parameters, current_module);
  if (parameter(parameters, "action") == "showStrongs" || parameter(parameters, "action") == "showMorph")
    return lexical_target(parameters, current_module);
  return note_target(parameters, current_module, current_reference);
}
}
ContextualStudyCommandLink contextual_study_link(const std::string &url, const std::string &current_module) {
  return contextual_link(url, current_module, {});
}
ContextualStudyCommandLink contextual_study_link(const std::string &url, const std::string &source_module,
                                                 const std::string &navigation_module) {
  if (url.compare(0, 8, "sword://") == 0 || url.compare(0, 8, "bible://") == 0)
    return passage_link(url, navigation_module);
  return contextual_study_link(url, source_module);
}
ContextualStudyCommandLink contextual_hover_link(const std::string &url, const std::string &current_module,
                                                 const std::string &current_reference) {
  if (current_reference.empty() || current_reference.find('\0') != std::string::npos || current_module.find('\0') != std::string::npos)
    throw std::invalid_argument("Hover source context is invalid");
  return contextual_link(url, current_module, current_reference);
}
std::string passage_uri(const std::string &reference) {
  static const char digits[] = "0123456789ABCDEF";
  std::string result = "sword:///";
  for (unsigned char c : reference) {
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
        (c >= '0' && c <= '9') || c == ':' || c == '.' || c == '_') result += c;
    else { result += '%'; result += digits[c >> 4]; result += digits[c & 15]; }
  }
  return result;
}
}
