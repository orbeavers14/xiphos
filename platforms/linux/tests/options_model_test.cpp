// SPDX-License-Identifier: GPL-2.0-or-later
#include "reader_model.hpp"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <thread>
using xiphos::linux_preview::ReaderModel;
void check(bool value, const char *message) { if (!value) throw std::runtime_error(message); }
void settle(ReaderModel &model) {
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
  while (model.busy()) { model.tick(); check(std::chrono::steady_clock::now() < deadline, "options timeout"); std::this_thread::sleep_for(std::chrono::milliseconds(1)); }
}
std::string read(const std::filesystem::path &path) { std::ifstream input(path, std::ios::binary); return {std::istreambuf_iterator<char>(input), {}}; }
int main(int argc, char **argv) {
  try {
    check(argc == 3, "module fixture and scratch root required");
    const std::filesystem::path scratch(argv[2]); std::filesystem::create_directories(scratch);
    ReaderModel model(argv[1]); settle(model);
    check(model.visit("OptionOther", "", "John 3:16"), "options passage"); settle(model);
    check(model.error().empty() && model.engine_configured(), "core preferences initialized");
    const auto flags = model.engine_flags(); const auto html = model.passage().bible_html;
    check(model.module("OptionOther") && (model.module("OptionOther")->declared_flags & XC_ENGINE_STRONGS), "declared option capability copied");
    check(model.set_filter(XC_ENGINE_STRONGS, !(flags & XC_ENGINE_STRONGS)), "toggle Strong's"); settle(model);
    check(model.error().empty() && model.engine_flags() == (flags ^ XC_ENGINE_STRONGS) && model.passage().bible_html != html, "setting changes real SDK HTML");
    check(!model.can_back() && model.passage().reference == "John 3:16", "filter refresh does not visit history");
    check(!model.set_filter(XC_ENGINE_STRONGS | XC_ENGINE_FOOTNOTES, true), "multi-flag input rejected");
    check(model.select_variant(XIPHOS_VARIANT_ALL), "all variants"); settle(model);
    check(model.error().empty() && (model.engine_flags() & XC_ENGINE_VARIANTS_ALL), "core variant flags");
    const auto saved_flags = model.engine_flags();
    const auto profile = scratch / "modops.conf";
    { std::ofstream output(profile); output << "[Uninstalled]\nCustomValue=preserve original café\n"; }
    check(model.reading_profile(profile.string(), true), "save engine/presentation options"); settle(model);
    check(model.error().empty() && read(profile).find("preserve original café") != std::string::npos, "save preserves unrelated values");
    const auto saved_bytes = read(profile);
    check(model.set_filter(XC_ENGINE_STRONGS, flags & XC_ENGINE_STRONGS), "change after save"); settle(model);
    check(model.reading_profile(profile.string(), false), "import saved preferences"); settle(model);
    check(model.error().empty() && model.engine_flags() == saved_flags && read(profile) == saved_bytes, "import restores flags without writing source");
    const auto retained = model.passage().bible_html;
    check(model.reading_profile((scratch / "missing/preferences.conf").string(), false), "missing import submitted"); settle(model);
    check(!model.error().empty() && model.passage().bible_html == retained && model.engine_flags() == saved_flags, "failed import retains state");
    check(model.reading_profile((scratch / "missing/output.conf").string(), true), "missing save submitted"); settle(model);
    check(!model.error().empty() && read(profile) == saved_bytes, "failed save preserves existing profile");
    auto *worker = model.detach(); std::thread([worker] { xc_worker_close(worker); }).join();
    std::cout << "Linux options/profile checks passed\n";
    return 0;
  } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
