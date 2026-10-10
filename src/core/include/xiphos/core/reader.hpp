// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "render.hpp"
#include <memory>
#include <string>
#include <vector>
namespace xiphos::core {
struct Module { std::string id, description, type, language; };
struct Entry { std::string reference, html; };
struct Chapter {
  std::string reference, title;
  std::vector<Entry> verses;
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
  Entry read(const std::string &module, const std::string &reference);
  Chapter read_chapter(const std::string &module, const std::string &reference);
  std::string adjacent_chapter(const std::string &module, const std::string &reference,
                               int direction);
private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
}
