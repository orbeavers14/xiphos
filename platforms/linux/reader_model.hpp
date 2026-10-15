// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "xiphos/core/c_api.h"
#include <cstdint>
#include <string>
#include <vector>

namespace xiphos::linux_preview {
struct Module { std::string id, description, type; };
struct Passage { std::string bible, commentary, reference, bible_html, commentary_html; };
// UI-thread serial submissions and nonblocking polling only. All engine policy
// and history stay on the core's one FIFO worker. No user-profile discovery.
class ReaderModel {
public:
  explicit ReaderModel(const char *root);
  ~ReaderModel();
  ReaderModel(const ReaderModel &) = delete;
  ReaderModel &operator=(const ReaderModel &) = delete;
  void tick();
  bool visit(const std::string &bible, const std::string &commentary, const std::string &reference);
  bool navigate(int direction);
  bool follow(const std::string &source, const std::string &uri);
  // Stop submissions/drop observation, then transfer shutdown ownership. The
  // caller MUST close the returned worker on a background thread (drain/join).
  xc_worker *detach();
  bool busy() const { return request_ != nullptr; }
  bool ready() const { return ready_ && !closed_; }
  bool can_back() const { return back_; }
  bool can_forward() const { return forward_; }
  const std::vector<Module> &modules() const { return modules_; }
  const Passage &passage() const { return passage_; }
  const std::string &error() const { return error_; }
  unsigned revision() const { return revision_; }
private:
  enum class Operation { catalog, tab, preferences, passage, history, link };
  bool accept(xc_status status, xc_request *request, Operation operation);
  void history();
  xc_worker *worker_ = nullptr;
  xc_request *request_ = nullptr;
  uint64_t tab_ = 0;
  Operation operation_ = Operation::catalog;
  bool ready_ = false, closed_ = false, back_ = false, forward_ = false;
  std::vector<Module> modules_;
  Passage passage_;
  std::string error_;
  unsigned revision_ = 0;
};
}
