// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/heading_notes.hpp"
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace xiphos::core;
void check(bool ok, const char *message) { if (!ok) throw std::runtime_error(message); }
int main() {
  try {
    HeadingNoteTarget target;
    unsigned cases = 0;
    for (const auto fragment : {std::size_t(0), std::size_t(1), std::size_t(19), std::numeric_limits<std::size_t>::max()})
      for (const auto &local : {std::string("1"), std::string("01"), std::string("original-note"), std::string("β"), std::string(" space "), std::string(256, 'a')}) {
        const auto id = heading_note_identifier(fragment, local);
        check(parse_heading_note_identifier(id, target) && target.fragment == fragment && target.local_id == local,
              "canonical scoped identifiers retain exact fragment and opaque local ID");
        check(id.find_first_not_of("xiphos-heading0123456789abcdef-") == std::string::npos,
              "generated heading IDs use only URL-safe ASCII"); ++cases;
      }
    check(heading_note_identifier(0, "1") != heading_note_identifier(1, "1"), "different fragments cannot collide");
    for (const auto *body : {"1", "original", "xiphos-heading"}) {
      target.fragment = 7; target.local_id = "stale";
      check(!parse_heading_note_identifier(body, target) && !target.fragment && target.local_id.empty(),
            "ordinary body identifiers remain outside the reserved namespace and clear output");
    }
    for (const auto &bad : {std::string("xiphos-heading-"), std::string("xiphos-heading-01-31"), std::string("xiphos-heading-+1-31"),
         std::string("xiphos-heading--1-31"), std::string("xiphos-heading-184467440737095516160-31"),
         std::string("xiphos-heading-0-"), std::string("xiphos-heading-0-3"), std::string("xiphos-heading-0-3A"),
         std::string("xiphos-heading-0-00"), std::string("xiphos-heading-0-0a"), std::string("xiphos-heading-0-7f"),
         std::string("xiphos-heading-0-31/"), std::string("xiphos-heading-0-") + std::string(514, '1')}) {
      target.fragment = 7; target.local_id = "stale"; bool rejected = false;
      try { parse_heading_note_identifier(bad, target); } catch (const std::invalid_argument &) { rejected = true; }
      check(rejected && !target.fragment && target.local_id.empty(), "malformed reserved IDs reject with cleared output");
    }
    for (const auto &bad : {std::string(), std::string("a\0b", 3), std::string("\n"), std::string(257, 'a')}) {
      bool rejected = false;
      try { heading_note_identifier(0, bad); } catch (const std::invalid_argument &) { rejected = true; }
      check(rejected, "invalid local IDs cannot enter the URL namespace");
    }
    const auto seed = heading_note_identifier(7, "Original");
    for (std::size_t at = 0; at <= seed.size(); ++at) for (char byte : std::string("-+xgA0%/\"")) {
      auto input = seed; input.insert(at, 1, byte);
      try {
        if (parse_heading_note_identifier(input, target))
          check(heading_note_identifier(target.fragment, target.local_id) == input, "accepted mutations remain canonical and reversible");
      } catch (const std::invalid_argument &) {}
    }
    std::cout << "Heading ID canonical round trips, boundaries, namespace separation and mutations passed: " << cases << " base cases\n";
  } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
