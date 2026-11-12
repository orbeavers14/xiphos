// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "xiphos/core/c_api.h"
#include <gtk/gtk.h>
#include <deque>
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace xiphos::linux_preview {
class SyncUI {
public:
  SyncUI(GtkApplication *application, xc_worker &worker);
  ~SyncUI();
  void present();
  void tick();
  std::function<std::pair<std::string, std::string>()> current_passage;
  std::function<bool(const std::string &, const std::string &, int, bool)> receive;
private:
  struct Navigation { std::string speaker, module, reference, group, domain; };
  enum class Operation { connect, disconnect, poll, send, plan };
  bool submit(xc_status status, xc_request *request, Operation operation);
  void configure(bool enabled);
  void send();
  void review();
  void update_review();
  void set_status(const char *message);

  xc_worker &worker_;
  xc_request *request_ = nullptr;
  Operation operation_ = Operation::poll;
  bool available_, active_ = false, configured_ = false;
  unsigned mode_applied_ = 0;
  std::string group_applied_;
  gint64 next_poll_ = 0;
  std::deque<Navigation> incoming_;
  std::vector<Navigation> review_;
  GtkApplication *application_;
  GtkWindow *window_;
  GtkWidget *mode_, *listen_, *user_, *phrase_, *group_, *allowed_, *consent_, *privacy_, *direct_, *group_windows_;
  GtkWidget *status_, *connect_, *disconnect_, *send_, *review_list_, *review_button_;
};
}
