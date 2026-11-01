// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <string>
namespace xiphos::core {
enum class ModuleMaintenanceAction {remove=0,set_cipher_key,build_index,delete_index};
struct ModuleMaintenanceRequest {
 std::string module,expected_revision;
 ModuleMaintenanceAction action=ModuleMaintenanceAction::remove;
 std::string cipher_key; // explicit SDK configuration, never returned in results
};
struct ModuleMaintenancePreview {std::string module,revision;bool cipher_declared=false,indexed=false;};
struct ModuleMaintenanceResult {std::string module,revision;ModuleMaintenanceAction action;bool indexed=false;};
bool module_maintenance_arguments(const ModuleMaintenanceRequest &request) noexcept;
}
