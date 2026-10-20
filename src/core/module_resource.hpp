// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <filesystem>
#include <optional>
#include <string>
namespace xiphos::core::detail {
// Local file URI slash/drive normalization shared by image source consumers.
std::string normalize_module_image_filename(std::string filename);
// Filesystem policy only: callers supply exact SDK directory context and decode
// source URIs separately. Missing-is-absent applies only to absent path entries.
std::filesystem::path module_resource_path(const std::optional<std::string> &directory,
    const std::string &filename, bool missing_is_absent);
// Bounded copied bytes with observed size/time/canonical-path change detection.
// Not an interprocess lock or a filesystem snapshot against concurrent mutation.
std::string read_module_resource_bytes(const std::filesystem::path &path,
    std::size_t maximum_bytes, const char *kind);
}
