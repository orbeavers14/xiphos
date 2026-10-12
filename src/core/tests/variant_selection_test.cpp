// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/variant.h"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>
using gchar = char;
using gboolean = int;
using gpointer = void *;
struct DIALOG_DATA {};
struct GtkMenuItem {};
#define TRUE 1
#define FALSE 0
#define GPOINTER_TO_INT(value) static_cast<int>(reinterpret_cast<std::intptr_t>(value))
std::vector<std::string> calls;
gchar *g_strdup_printf(const char *format, const char *module, const char *key) {
  const auto size = std::snprintf(nullptr, 0, format, module, key);
  auto *result = static_cast<char *>(std::malloc(static_cast<std::size_t>(size) + 1));
  if (!result) throw std::bad_alloc();
  std::snprintf(result, static_cast<std::size_t>(size) + 1, format, module, key);
  return result;
}
void g_free(void *pointer) { std::free(pointer); }
void g_message(const char *, int value) { calls.push_back("invalid:" + std::to_string(value)); }
void gui_generic_warning(const char *message) { calls.emplace_back(message); }
void main_save_module_options(const char *module, const char *name, int value) {
  calls.push_back(std::string(module) + ':' + name + '=' + std::to_string(value));
}
void main_dialogs_url_handler(DIALOG_DATA *, const char *url, int clicked) {
  calls.push_back(std::string("dialog:") + url + ':' + std::to_string(clicked));
}
void main_url_handler(const char *url, int clicked) {
  calls.push_back(std::string("main:") + url + ':' + std::to_string(clicked));
}
namespace frozen {
#include "legacy_variant_selector.inc"
}
namespace actual {
#include "legacy_variant_adapter.inc"
}
int main() {
  try {
    unsigned count = 0;
    for (const int choice : {0, 1, 2, -1, 3, std::numeric_limits<int>::max()}) {
      for (const bool dialog : {false, true}) {
        char module[] = "Original % café", key[] = "John 3:16";
        DIALOG_DATA context;
        auto *pointer = dialog ? &context : nullptr;
        const auto value = reinterpret_cast<void *>(static_cast<std::intptr_t>(choice));
        calls.clear(); frozen::reading_selector(module, key, pointer, nullptr, value);
        const auto expected = calls;
        calls.clear(); actual::reading_selector(module, key, pointer, nullptr, value);
        if (calls != expected) throw std::runtime_error("Shared variant adapter diverged from frozen legacy selector");
        ++count;
      }
    }
    std::cout << "Frozen/actual variant selector matches (" << count << " cases)\n";
  } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
