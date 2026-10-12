// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/history.h"
#include <array>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
// Explicit GTK/type/display shims; use the actual history layout/limits header.
using gchar = char; using gint = int; using gpointer = void *;
struct GtkMenuItem; struct GtkWidget;
#include "main/tab_history.h"
#define TRUE 1
struct PASSAGE_TAB_INFO {
  TAB_HISTORY history_list[TABHISTORYLENGTH]{};
  gint history_items = 0, current_history_item = 0, first_back_click = 0;
  gchar *text_commentary_key = nullptr, *text_mod = nullptr, *commentary_mod = nullptr;
};
static PASSAGE_TAB_INFO *cur_passage_tab;
static std::vector<std::array<int, 3>> callbacks;
extern "C" void main_update_tab_history_menu(gpointer data) {
  const auto &tab = *static_cast<PASSAGE_TAB_INFO *>(data);
  callbacks.push_back({{tab.history_items, tab.current_history_item, tab.first_back_click}});
}
#define main_add_tab_history_item frozen_add
#define main_fake_tab_history_item frozen_fake
#include "legacy_tab_history.inc"
#undef main_add_tab_history_item
#undef main_fake_tab_history_item
#define main_add_tab_history_item actual_add
#define main_fake_tab_history_item actual_fake
#include "legacy_tab_history_actual.inc"
#undef main_add_tab_history_item
#undef main_fake_tab_history_item
namespace {
unsigned comparisons = 0;
void compare(PASSAGE_TAB_INFO &frozen, PASSAGE_TAB_INFO &actual, bool fake, char *key) {
  callbacks.clear(); cur_passage_tab = &frozen;
  if (fake) frozen_fake(key); else frozen_add(&frozen);
  const auto expected = callbacks;
  callbacks.clear(); cur_passage_tab = &actual;
  if (fake) actual_fake(key); else actual_add(&actual);
  ++comparisons;
  if (frozen.history_items != actual.history_items || frozen.current_history_item != actual.current_history_item ||
      frozen.first_back_click != actual.first_back_click || expected != callbacks ||
      std::memcmp(frozen.history_list, actual.history_list, sizeof(frozen.history_list)))
    throw std::runtime_error("complete history state/ordered callback mismatch");
}
}
int main() {
  try {
    static_assert(TABHISTORYLENGTH == XH_HISTORY_LIMIT, "shared and legacy capacities must agree");
    std::array<std::string, 3> texts{{"", "Text", "Texte-café"}}, comments{{"", "Commentary", "Comment-λόγος"}};
    for (int count = 0; count <= TABHISTORYLENGTH; ++count)
      for (int cursor : {0, count / 2, count ? count - 1 : 0})
        for (int first : {0, 1}) for (int key_case = 0; key_case < 5; ++key_case)
          for (int text = -1; text < 3; ++text) for (int comment = -1; comment < 3; ++comment)
            for (bool fake : {false, true}) {
              PASSAGE_TAB_INFO frozen;
              frozen.history_items = count; frozen.current_history_item = cursor; frozen.first_back_click = first;
              for (int i = 0; i < TABHISTORYLENGTH; ++i) {
                std::strcpy(frozen.history_list[i].verseref, ("Ref-" + std::to_string(i)).c_str());
                std::strcpy(frozen.history_list[i].textmod, "OriginalText");
                std::strcpy(frozen.history_list[i].commod, "OriginalCommentary");
              }
              std::string key = key_case == 0 ? "Ref-0" : key_case == 1 ? "Ref-" + std::to_string(count / 2) :
                  key_case == 2 ? "Ref-" + std::to_string(count ? count - 1 : 0) : key_case == 3 ? "New-café" : std::string(TABELEMENTLENGTH - 1, 'x');
              frozen.text_commentary_key = &key[0];
              frozen.text_mod = text < 0 ? nullptr : &texts[static_cast<unsigned>(text)][0];
              frozen.commentary_mod = comment < 0 ? nullptr : &comments[static_cast<unsigned>(comment)][0];
              auto actual = frozen;
              compare(frozen, actual, fake, &key[0]);
            }
    PASSAGE_TAB_INFO frozen, actual;
    for (int i = 0; i < 600; ++i) {
      std::string key = "Sequential-" + std::to_string((i * 17) % 73), text = "Text-" + std::to_string(i % 3);
      frozen.text_commentary_key = actual.text_commentary_key = &key[0];
      frozen.text_mod = actual.text_mod = &text[0];
      if (i % 11 == 0) frozen.first_back_click = actual.first_back_click = 0;
      compare(frozen, actual, i % 7 == 0, &key[0]);
    }
    std::cout << "Complete tab history adapter: " << comparisons << " comparisons passed\n";
  } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
