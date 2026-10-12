// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/lexical_style.hpp"
#include <atomic>
#include <cstdio>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <vector>
struct GLOBAL_OPS { int strongs, lemmas, morphs, doublespace; };
#include "legacy_lexical_style.inc"
#undef CSS_BLOCK_BOTH
#undef CSS_BLOCK_ONE
#undef DOUBLE_SPACE
#include "legacy_lexical_style_actual.inc"
using xiphos::core::lexical_block_style;
void check(bool ok, const char *message) { if (!ok) throw std::runtime_error(message); }
int main() {
  try {
    unsigned comparisons = 0;
    for (int strongs : {0, 1, -1, 2}) for (int lemmas : {0, 1, -1, 2})
      for (int morphs : {0, 1, -1, 2}) for (int spacing : {0, 1, -1, 2}) {
        GLOBAL_OPS options{strongs, lemmas, morphs, spacing};
        const std::string expected = frozen_lexical_style(strongs, lemmas, morphs, spacing);
        const auto actual = lexical_block_style(strongs, lemmas, morphs, spacing);
        check(actual == expected && actual_style_0(&options) == expected && actual_style_1(&options) == expected && actual_style_2(&options) == expected,
              "owned CSS and all actual renderer call sites retain frozen declarations/selection");
        comparisons += 3;
      }
    const auto held = lexical_block_style(true, false, true, true);
    const auto next = lexical_block_style(false, false, false, true);
    check(held.find("line-height: 3.5em") != std::string::npos && next.find("line-height: 2em") != std::string::npos &&
          held.find("*[dir=rtl]") != std::string::npos, "owned style survives later selection and retains RTL rules");
    check(lexical_block_style(false, true, true, true) == held &&
          lexical_block_style(false, true, false, true).find("line-height: 2.7em") != std::string::npos &&
          lexical_block_style(false, false, false, false).empty(), "lemma row and annotation precedence retain legacy policy");
    std::atomic<bool> failed{false}; std::vector<std::thread> readers;
    for (unsigned thread = 0; thread < 4; ++thread) readers.emplace_back([&] {
      for (unsigned repeat = 0; repeat < 32; ++repeat) for (unsigned bits = 0; bits < 16; ++bits)
        if (lexical_block_style(bits & 1, bits & 2, bits & 4, bits & 8) != frozen_lexical_style(bits & 1, bits & 2, bits & 4, bits & 8)) failed = true;
    });
    for (auto &reader : readers) reader.join();
    check(!failed, "owned constant style data supports concurrent readers");
    std::cout << "Lexical CSS frozen/actual/owned checks passed: " << comparisons << " call-site cases + 2048 concurrent selections\n";
  } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
