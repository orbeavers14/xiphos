// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/preferences.hpp"
#include <swconfig.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace xiphos::core;
void check(bool condition, const char *message) { if (!condition) throw std::runtime_error(message); }
std::string bytes(const std::filesystem::path &path) {
  std::ifstream input(path, std::ios::binary);
  check(bool(input), "open saved fixture");
  return std::string((std::istreambuf_iterator<char>(input)), {});
}
int main(int argc, char **argv) {
  try {
    check(argc == 2, "fixture root required");
    const std::filesystem::path root(argv[1]);
    std::filesystem::create_directories(root);
    const auto file = root / std::filesystem::u8path("preferences-café.conf");
    { std::ofstream output(file, std::ios::binary);
      output << "; Preserve unrelated semantic values.\n[OptionTest]\nHeadings=Off\nCustom=café & peace\nStrong's Numbers=Off\n[Uninstalled]\nCustom=keep\nCustom=duplicate\n"; }
    std::filesystem::permissions(file, std::filesystem::perms::owner_read | std::filesystem::perms::owner_write);
    const auto original_permissions = std::filesystem::status(file).permissions();
    EngineOptions on, off;
    on.strongs = on.words_in_red = on.footnotes = true;
    save_module_engine_preferences(file.u8string(), {{"OptionTest", on}, {"OptionOther", off}});
    check(std::filesystem::status(file).permissions() == original_permissions, "existing file permissions retained");
    sword::SWConfig saved(file.u8string().c_str()); saved.load();
    check(saved["OptionTest"]["Headings"] == "Off" && saved["OptionTest"]["Custom"] == "café & peace", "unrelated display/UTF-8 values retained");
    check(saved["Uninstalled"].count("Custom") == 2, "uninstalled sections and duplicate unknown keys retained");
    const auto decoded = read_module_engine_preferences(file.u8string());
    check(decoded.at("OptionTest").strongs && decoded.at("OptionTest").words_in_red && decoded.at("OptionTest").footnotes, "saved configured fields decoded");
    check(!decoded.at("OptionOther").strongs && decoded.at("OptionOther").variants_primary, "no-variant legacy primary fallback documented");
    for (unsigned mask = 0; mask < 8; ++mask) {
      EngineOptions options;
      options.variants_primary = mask & 1; options.variants_secondary = mask & 2; options.variants_all = mask & 4;
      save_module_engine_preferences(file.u8string(), {{"OptionTest", options}});
      const auto actual = read_module_engine_preferences(file.u8string()).at("OptionTest");
      check(actual.variants_primary == bool(mask & 1 || mask == 0) && actual.variants_secondary == bool(mask & 2) && actual.variants_all == bool(mask & 4), "all variant combinations preserve legacy reload semantics");
    }
    const auto new_file = root / "new-modops.conf";
    std::filesystem::remove(new_file);
    save_module_engine_preferences(new_file.u8string(), {{"OptionTest", on}});
    check(read_module_engine_preferences(new_file.u8string()).at("OptionTest").strongs, "new file save");
    const auto original = bytes(file);
    bool failed = false;
    try { save_module_engine_preferences(file.u8string(), {{"Broken\nSection", on}}); } catch (const std::runtime_error &) { failed = true; }
    check(failed && bytes(file) == original, "verification failure preserves original destination bytes");
    failed = false;
    try { save_module_engine_preferences(root.u8string(), {{"OptionTest", on}}); } catch (const std::invalid_argument &) { failed = true; }
    check(failed && bytes(file) == original, "directory save rejected without changing existing files");
    failed = false;
    try { save_module_engine_preferences((root / "missing-parent/modops.conf").u8string(), {{"OptionTest", on}}); } catch (const std::exception &) { failed = true; }
    check(failed && !std::filesystem::exists(root / "missing-parent"), "missing parent not created silently");
#ifndef _WIN32
    const auto link = root / "linked-modops.conf";
    std::filesystem::remove(link);
    std::filesystem::create_symlink(file.filename(), link);
    save_module_engine_preferences(link.u8string(), {{"OptionTest", on}});
    check(std::filesystem::is_symlink(link) && read_module_engine_preferences(file.u8string()).at("OptionTest").strongs, "existing symlink alias and target retained");
    std::filesystem::permissions(file, std::filesystem::perms::owner_read);
    const auto readonly = bytes(file);
    failed = false;
    try { save_module_engine_preferences(file.u8string(), {{"OptionTest", off}}); } catch (const std::runtime_error &) { failed = true; }
    check(failed && bytes(file) == readonly, "read-only destination preserved");
    std::filesystem::permissions(file, original_permissions);
#endif
    for (const auto &entry : std::filesystem::directory_iterator(root))
      check(entry.path().filename().u8string().find(".xiphos-preferences-") != 0, "owned temporary files cleaned after success/failure");
    std::cout << "Preference save preservation/round-trip/failure checks passed\n";
  } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
