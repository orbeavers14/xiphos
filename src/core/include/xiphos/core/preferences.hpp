// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "engine_options.hpp"
namespace xiphos::core {
// Explicit shared-core save. Existing unrelated sections/keys are retained via
// SWORD's parser/writer (comments/layout normalize as in legacy saves). Prepare
// and verify a same-directory temporary file before replacing the destination.
// Errors leave the destination untouched before replacement; no home discovery.
// Existing symlinks resolve to their target, preserving the alias.
void save_module_engine_preferences(const std::string &path, const ModuleEnginePreferences &settings);
}
