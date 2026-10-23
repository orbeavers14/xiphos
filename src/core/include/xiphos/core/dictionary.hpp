// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <string>
#include <vector>
namespace sword { class SWModule; }
namespace xiphos { namespace core {
// Main-pane clicks use GLib's locale-sensitive Unicode uppercase. Hover/raw
// reads do not. Owns the result; rejects malformed UTF-8 and embedded NUL.
std::string dictionary_click_key(const std::string &key);
// Retained main dictionary display routing after installed/available checks.
// An absent current dictionary is initialized to the target and stays dictionary.
// A changed DailyDevotion module uses an exact NN.NN key or asks the host for
// local today. This preserves shape-only validation (including 99.99); no clock,
// SDK, widget or allocation is involved here.
enum class DictionaryDisplayRoute { dictionary, devotional_key, devotional_today };
DictionaryDisplayRoute dictionary_display_route(const char *current_dictionary,
    const char *target_module, const char *feature, const char *key) noexcept;
// Caller positions the module first. Direction -1/+1; owns SDK returned key.
// Legacy nine-key chooser: five steps back, then nine forward snapshots.
std::vector<std::string> dictionary_nearby_keys(sword::SWModule &module);
std::string dictionary_step_key(sword::SWModule &module, int direction);
}}
