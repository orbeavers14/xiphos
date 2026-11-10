// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "editable_document.hpp"
#include <cstdint>
namespace xiphos::core {
struct ModuleEditTarget {std::string module,reference;bool create_book_entry=false;std::string rename_book_leaf{};}; // Full verse reference or exact book path; creation is explicit and RawGenBook-only.
struct ModuleEditEntry {
  std::string module,reference,driver,text,revision;
  std::uint64_t offset=0;
  std::string source_type;
};
bool module_edit_arguments(const ModuleEditTarget &target) noexcept;
}
