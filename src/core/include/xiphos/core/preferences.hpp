// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "engine_options.hpp"
#include "presentation.hpp"
namespace xiphos::core {
// Explicit shared-core save. Existing unrelated sections/keys are retained via
// SWORD's parser/writer (comments/layout normalize as in legacy saves). Prepare
// and verify a same-directory temporary file before replacing the destination.
// Errors leave the destination untouched before replacement; no home discovery.
// Existing symlinks resolve to their target, preserving the alias.
// Combined explicit save uses the same verified replacement transaction.
// Only supplied presentation sections update Commentary by Chapter.
void save_module_preferences(const std::string &path, const ModuleEnginePreferences &settings,
                              const ModulePresentationPreferences &presentation);
void save_module_engine_preferences(const std::string &path, const ModuleEnginePreferences &settings);
}
