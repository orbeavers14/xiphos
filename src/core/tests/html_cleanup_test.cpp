// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/html_cleanup.hpp"
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>
using gchar = char;
std::string diagnostic;
void log_message(const char *format, ...) {
  va_list args; va_start(args, format);
  char buffer[4096]; vsnprintf(buffer, sizeof(buffer), format, args); va_end(args);
  diagnostic += buffer;
}
#define XI_message(args) log_message args
namespace frozen {
#include "legacy_html_cleanup.inc"
}
#undef CLEAR_FONT_NAME
#undef CLEAR_FONT_NAME_LENGTH
namespace current {
#include "legacy_html_cleanup_actual.inc"
}
#undef XI_message
#undef CLEAR_FONT_NAME
#undef CLEAR_FONT_NAME_LENGTH
void check(bool ok, const char *message) { if (!ok) throw std::runtime_error(message); }
int main() {
  try {
    auto compare = [](const std::string &input, bool image) {
      std::vector<char> old(input.begin(), input.end()), actual(old);
      old.push_back('\0'); actual.push_back('\0');
      diagnostic.clear();
      if (image) frozen::ClearImages(old.data()); else frozen::ClearFontFaces(old.data());
      const auto old_message = diagnostic;
      diagnostic.clear();
      if (image) current::ClearImages(actual.data()); else current::ClearFontFaces(actual.data());
      const auto result = image ? xiphos::core::blank_images(input) : xiphos::core::blank_font_faces(input);
      check(std::string(old.data()) == result.html && old == actual, "owned cleanup and adapted complete function match frozen bytes");
      check(diagnostic == old_message && result.html.size() == input.size(), "legacy diagnostics and byte length retained");
      std::string message;
      if (result.malformed_offset != std::string::npos) {
        check(result.malformed_offset <= result.html.size(), "diagnostic offset in bounds");
        message = std::string(image ? "ClearImages: no img end: " : "ClearFontFaces: no closing double quote: ") +
                  result.html.substr(result.malformed_offset) + "\n";
      }
      check(message == old_message, "owned malformed offset agrees with complete legacy diagnostic");
    };
    const std::vector<std::string> tokens = {
      "", "original café text.", "<img src=\"original.png\" />", "<img src='original.png'>", "<img >",
      "<IMG src=\"upper.png\">", "<img\tsrc=\"tab.png\">", "<image src=\"other.png\">", "<img", "<img ",
      "<img src=\"unfinished", "<img alt=\"a > b\" src=\"original.png\">", "<font face=\"Original\">",
      "<font face=\"café\">", "<font face=\"\">", "<FONT face=\"upper\">", "<font FACE=\"upper\">",
      "<font face='single'>", "<font size=\"2\" face=\"ordered\">", "<font face=\"unfinished",
      "</font>", "\"", ">", "<a href=\"sword:///John%203:16\">original link</a>"
    };
    unsigned cases = 0;
    for (const auto &first : tokens) for (const auto &second : tokens) for (const auto &third : tokens) {
      const auto input = first + second + third;
      compare(input, true); compare(input, false); cases += 2;
    }
    check(xiphos::core::blank_images("before<img src=\"x\">after").html == "before             after", "image blanks the exact tag span");
    check(xiphos::core::blank_font_faces("<font face=\"café\">body</font>").html == "<font face=\".....\">body</font>", "font face masks bytes without changing other markup");
    for (bool image : {false, true}) {
      bool rejected = false;
      try { if (image) xiphos::core::blank_images(std::string("a\0b", 3)); else xiphos::core::blank_font_faces(std::string("a\0b", 3)); }
      catch (const std::invalid_argument &) { rejected = true; }
      check(rejected, "owned HTML rejects embedded NUL rather than silently truncating");
    }
    std::cout << "HTML cleanup frozen/current/owned checks passed: " << cases << " comparisons\n";
  } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
