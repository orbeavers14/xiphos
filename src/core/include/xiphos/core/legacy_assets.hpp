// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <memory>
namespace xiphos::core {
class TransferControl;
struct LegacyAsset {std::string path,revision;std::uint64_t bytes=0;bool directory=false;};
struct LegacyAssets {std::string source,revision;std::vector<LegacyAsset> entries;std::uint64_t bytes=0;};
// Selected directory only: includes hidden files/empty directories. No symlink
// traversal, external-reference chasing, execution or modification of originals.
// Bounds: 10000 entries, depth64,128MiB/file,1GiB/tree. Portable name aliases reject.
LegacyAssets inspect_legacy_assets(const std::string &source);
// Recheck reviewed source; verified private staging then publish a NEW directory.
// Existing destinations and destinations inside the source reject. No merging.
LegacyAssets copy_legacy_assets(const std::string &source,const std::string &revision,const std::string &destination);
// Additive cancellable forms. A cancellation winning the existing transfer gate
// prevents publication; cancellation loses once commit starts. Sources stay intact.
LegacyAssets inspect_legacy_assets(const std::string &source,std::shared_ptr<TransferControl> control);
LegacyAssets copy_legacy_assets(const std::string &source,const std::string &revision,const std::string &destination,std::shared_ptr<TransferControl> control);
}
