// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/dictionary.hpp"
#include <glib.h>
#include <clocale>
#include <iostream>
#include <stdexcept>
#include <vector>
using xiphos::core::dictionary_click_key;
void check(bool value, const char *message) { if (!value) throw std::runtime_error(message); }
template<class Action> void invalid(Action action) {
  bool rejected=false;
  try { action(); } catch (const std::invalid_argument &) { rejected=true; }
  check(rejected,"malformed key rejected");
}
// Frozen main_display_dictionary operation: do not substitute a second Unicode
// implementation or ASCII-only casing oracle for the legacy GLib call.
std::string legacy_click(const std::string &key) {
  gchar *upper=g_utf8_strup(key.c_str(),-1);
  const std::string result=upper; g_free(upper); return result;
}
int main() {
  try {
    const std::string original=std::setlocale(LC_CTYPE,nullptr);
    const std::vector<std::string> keys={"", "alpha", "café", "straße", "ﬃ", "iıİI", "i̇", "j̇́", "λόγος", "ὀδυσσεύς", "ΐ", "և", "привет", "שלום", "神", "😀", "03056", "robinson:N-NSM"};
    std::size_t comparisons=0, locales=0;
    for (const auto *locale : {"C", "en_US.UTF-8", "tr_TR.UTF-8", "lt_LT.UTF-8"}) {
      if (!std::setlocale(LC_CTYPE,locale)) { std::cout<<"Unavailable locale: "<<locale<<'\n'; continue; }
      ++locales;
      for (const auto &key : keys) { check(dictionary_click_key(key)==legacy_click(key),"legacy locale casing differs"); ++comparisons; }
      // Enumerate every valid Unicode scalar, including supplementary scripts.
      for (gunichar scalar=1; scalar<=0x10ffff; ++scalar) {
        if (!g_unichar_validate(scalar)) continue;
        gchar encoded[7]={}; const auto length=g_unichar_to_utf8(scalar,encoded);
        const std::string key(encoded,static_cast<std::size_t>(length));
        check(dictionary_click_key(key)==legacy_click(key),"legacy Unicode scalar casing differs"); ++comparisons;
      }
    }
    check(locales>=1,"C locale required");
    check(std::setlocale(LC_CTYPE,"C")!=nullptr,"C locale selection");
    check(dictionary_click_key("straße ﬃ café")=="STRASSE FFI CAFÉ","Unicode expansion and accents retained");
    invalid([] { (void)dictionary_click_key(std::string("a\0b",3)); });
    for (const auto &key : {std::string("\xff",1),std::string("\xc0\xaf",2),std::string("\xed\xa0\x80",3),std::string("\xe2\x82",2)})
      invalid([&] { (void)dictionary_click_key(key); });
    check(std::setlocale(LC_CTYPE,original.c_str())!=nullptr,"restore process locale");
    std::cout<<comparisons<<" frozen GLib comparisons in "<<locales<<" locales\n";
    return 0;
  } catch (const std::exception &error) { std::cerr<<error.what()<<'\n'; return 1; }
}
