// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/history.hpp"
#include <iostream>
#include <stdexcept>
using xiphos::core::History;
void require(bool ok, const char *why) { if (!ok) throw std::runtime_error(why); }
int main() {
  try {
    History history;
    history.visit({"A", "Text1", "Commentary1"});
    history.visit({"B", "Text2", "Commentary2"});
    history.visit({"C", "Text3", "Commentary3"});
    history.move(-1);
    history.visit({"D", "Text4", "Commentary4"});
    require(history.entries().size() == 4 && history.entries()[2].reference == "C",
            "new visits preserve old forward history, unlike browser history");
    history.visit({"B", "ChangedText", "ChangedCommentary"});
    require(history.cursor() == 1 && history.entries().size() == 4,
            "reference deduplication selects existing index");
    require(history.entries()[1].module == "Text2" && history.entries()[1].commentary == "Commentary2",
            "duplicate preserves original module pair");
    require(history.destination(1).reference == "C", "forward destination");
    history.move(1);
    require(history.destination(-1).reference == "B", "back destination");
    history.clear();
    for (int i = 0; i < 51; ++i) history.visit({std::to_string(i), "Text", ""});
    require(history.entries().size() == 50 && history.entries().front().reference == "1" &&
            history.cursor() == 49, "oldest entry evicted at legacy limit");
    for (int i = 0; i < 49; ++i) history.move(-1);
    require(!history.can_back() && history.can_forward(), "oldest boundary");
    try { history.move(-1); throw std::runtime_error("back at boundary succeeded"); }
    catch (const std::invalid_argument &) {}
    require(history.cursor() == 0, "failed move leaves cursor unchanged");
    history.clear();
    require(history.entries().empty() && !history.can_back() && !history.can_forward(), "clear boundaries");
    std::cout << "Legacy history characterization checks passed\n";
  } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
