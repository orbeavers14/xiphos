// SPDX-License-Identifier: GPL-2.0-or-later
#include "sdk_render.hpp"
#include "xiphos/core/study_note.hpp"
#include <swmodule.h>
#include <utf8html.h>
#include <stdexcept>
namespace xiphos { namespace core {
namespace {
StudyNote read_note(sword::SWModule &module, const std::string &reference,
                    const std::string &identifier, bool render_body) {
  if (reference.empty() || identifier.empty() || reference.find('\0') != std::string::npos || identifier.find('\0') != std::string::npos)
    throw std::invalid_argument("Note requires a valid passage and identifier");
  module.setKey(reference.c_str());
  detail::render_sdk_entry(module); // Entry attributes belong to this exact verse, not the last rendered chapter verse.
  if (module.getKey()->getError()) throw std::invalid_argument("SWORD could not resolve the note passage");
  const auto &attributes = module.getEntryAttributes();
  const auto group = attributes.find("Footnote");
  if (group == attributes.end()) throw std::invalid_argument("Passage has no study notes");
  const auto found = group->second.find(identifier.c_str());
  if (found == group->second.end()) throw std::invalid_argument("Study note is unavailable");
  const auto &values = found->second;
  const auto value = [&values](const char *key) {
    const auto found = values.find(key);
    return found == values.end() ? std::string() : std::string(found->second.c_str());
  };
  StudyNote result;
  result.module = module.getName();
  result.reference = module.getKeyText(); result.identifier = identifier;
  result.type = value("type"); result.label = value("n"); result.reference_list = value("refList");
  if (render_body) {
    // Copy every required value before this render can replace SDK attributes.
    sword::SWBuf body(value("body").c_str());
    sword::UTF8HTML encoding;
    encoding.processText(body);
    result.html = detail::render_sdk_buffer(module, body.c_str()).c_str();
  }
  return result;
}
}
StudyNote render_study_note(sword::SWModule &module, const std::string &reference,
                            const std::string &identifier) {
  return read_note(module, reference, identifier, true);
}
StudyNote read_study_note_metadata(sword::SWModule &module, const std::string &reference,
                                   const std::string &identifier) {
  return read_note(module, reference, identifier, false);
}
}}
