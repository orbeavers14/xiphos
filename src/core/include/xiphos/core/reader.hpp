// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "render.hpp"
#include "study_note.hpp"
#include "engine_options.hpp"
#include "content.hpp"
#include "link.hpp"
#include "navigation.hpp"
#include <memory>
#include <string>
#include <vector>
namespace xiphos::core {
struct Module {
  std::string id, description, type, language;
  ModuleOptions options;
  bool transliteration_available = false;
};


class Reader {
public:
  // Empty root uses SWORD's standard discovery. Explicit root disables home
  // augmentation, permitting deterministic, isolated tests and previews.
  explicit Reader(const std::string &root);
  ~Reader();
  Reader(const Reader &) = delete;
  Reader &operator=(const Reader &) = delete;
  const std::vector<Module> &modules() const;
  ReferenceChoices reference_choices(const std::string &module, const std::string &reference);
  PassageLink resolve_link(const std::string &current_module, const std::string &url);
  ReferenceList read_reference_list(const std::string &module, const std::string &expression,
                                    const std::string &context);
  ReferenceList read_reference_list(const std::string &module, const std::string &expression,
                                    const std::string &context, const EngineOptions &options);
  StudyNote study_note(const std::string &module, const std::string &reference,
                       const std::string &identifier);
  StudyNote study_note(const std::string &module, const std::string &reference,
                       const std::string &identifier, const EngineOptions &options);
  Entry read(const std::string &module, const std::string &reference);
  Entry read(const std::string &module, const std::string &reference, const EngineOptions &options);
  Chapter read_chapter(const std::string &module, const std::string &reference,
                       const DisplayOptions &display, const EngineOptions &engine);
  Chapter read_chapter(const std::string &module, const std::string &reference,
                       const DisplayOptions &options = {});
  std::string adjacent_chapter(const std::string &module, const std::string &reference,
                               int direction);
private:
  ReferenceList reference_list_impl(const std::string &module, const std::string &expression,
                                    const std::string &context, const EngineOptions *options);
  StudyNote note_impl(const std::string &module, const std::string &reference,
                      const std::string &identifier, const EngineOptions *options);
  Entry read_impl(const std::string &module, const std::string &reference, const EngineOptions *options);
  Chapter chapter_impl(const std::string &module, const std::string &reference,
                       const DisplayOptions &display, const EngineOptions *engine);
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
}
