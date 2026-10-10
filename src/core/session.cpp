// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/session.hpp"
namespace xiphos::core {
Entry StudySession::visit(const std::string &module, const std::string &reference) {
  auto entry = reader_.read(module, reference);
  history_.visit({entry.reference, module, ""});
  return entry;
}
Entry StudySession::navigate(int direction) {
  const auto &location = history_.destination(direction);
  auto entry = reader_.read(location.module, location.reference);
  history_.move(direction);
  return entry;
}
}
