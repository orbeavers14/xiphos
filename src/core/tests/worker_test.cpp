// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/worker.hpp"
#include <filesystem>
#include <iostream>
#include <stdexcept>
using namespace xiphos::core;
void check(bool ok, const char *message) { if (!ok) throw std::runtime_error(message); }
template<class Error, class T> void fails(std::future<T> future, const char *message) {
  try { (void)future.get(); } catch (const Error &) { return; }
  throw std::runtime_error(message);
}
int main(int argc, char **argv) {
  try {
    check(argc == 2, "fixture path required");
    const std::string root = argv[1];
    std::future<Chapter> surviving;
    std::future<HistorySnapshot> final_history;
    {
      StudyWorker worker(root);
      const auto modules = worker.modules().get();
      check(modules.size() == 1 && modules[0].id == "ParityTest", "worker module snapshot");
      const auto tab = worker.create_tab().get();
      const auto other = worker.create_tab().get();
      check(tab != other, "independent tab identities");
      std::string reference = "John 3:16";
      auto first = worker.visit(tab, "ParityTest", reference);
      reference = "John 3:17"; // submitted work owns its earlier input
      auto second = worker.visit(tab, "ParityTest", reference);
      auto back = worker.navigate(tab, -1);
      auto history = worker.history(tab);
      check(first.get().html.find("café") != std::string::npos, "owned queued input");
      check(second.get().html.find("Second fixture") != std::string::npos, "second queued result");
      check(back.get().reference == "John 3:16", "FIFO navigation after visits");
      const auto h = history.get();
      check(h.entries.size() == 2 && h.cursor == 0 && !h.can_back && h.can_forward, "FIFO history snapshot");
      check(worker.history(other).get().entries.empty(), "tab history isolation");
      auto failed = worker.visit(tab, "Missing", "John 3:16");
      auto forward = worker.navigate(tab, 1);
      fails<std::out_of_range>(std::move(failed), "operation failure propagated");
      check(forward.get().reference == "John 3:17", "queue continues after failed operation");
      check(worker.history(tab).get().entries.size() == 2, "failed visit did not change history");
      const auto choices = worker.reference_choices("ParityTest", "John 3:16").get();
      check(choices.books.size() == 27 && choices.verses.size() == 36, "worker selector snapshot");
      const auto link = worker.resolve_link("ParityTest", "sword:///John+3%3A17").get();
      check(link.module == "ParityTest" && link.reference == "John 3:17", "worker link policy");
      const auto next = worker.navigate_chapter(tab, "ParityTest", "John 21:25", 1).get();
      check(next.reference == "Acts 1:1", "worker chapter navigation");
      // Multiple native hosts may submit to ONE shared worker. These producers
      // never call SWORD directly, and copied results remain independent.
      std::vector<std::future<void>> producers;
      for (int producer = 0; producer < 4; ++producer) {
        producers.push_back(std::async(std::launch::async, [&worker, producer] {
          for (int request = 0; request < 10; ++request) {
            const int verse = producer % 2 ? 17 : 16;
            const auto chapter = worker.read_chapter("ParityTest", "John 3:" + std::to_string(verse), {false, true}).get();
            check(chapter.selected_verse == static_cast<size_t>(verse) && chapter.verses.size() == 36,
                  "concurrent producers retain their own selection/results");
            check(chapter.html.find("<sup>") == std::string::npos, "queued display options");
          }
        }));
      }
      for (auto &producer : producers) producer.get();
      auto closed = worker.close_tab(other);
      auto invalid = worker.visit(other, "ParityTest", "John 3:16");
      closed.get();
      fails<std::invalid_argument>(std::move(invalid), "closed tab rejected");
      const auto replacement = worker.create_tab().get();
      check(replacement != other && worker.history(replacement).get().entries.empty(), "closed IDs not reused");
      worker.clear_history(tab).get();
      check(worker.history(tab).get().entries.empty(), "worker clear history");
      // Leave queued work unawaited: destructor must drain accepted commands.
      worker.visit(tab, "ParityTest", "John 3:16");
      worker.visit(tab, "ParityTest", "John 3:17");
      surviving = worker.read_chapter("ParityTest", "John 3:16");
      final_history = worker.history(tab);
    }
    check(surviving.get().html.find("café") != std::string::npos, "copied result survives worker shutdown");
    check(final_history.get().entries.size() == 2, "shutdown drains accepted work");
    {
      StudyWorker broken((std::filesystem::path(root) / "not-installed").string());
      fails<std::invalid_argument>(broken.modules(), "initialization failure reaches modules future");
      fails<std::invalid_argument>(broken.create_tab(), "initialization failure reaches later requests");
    }
    std::cout << "Shared engine worker checks passed\n";
  } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
